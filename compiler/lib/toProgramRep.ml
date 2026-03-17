open Absyn
open ProgramRep
open Exceptions
open Field_props
open Builtins
open Resources
open Helpers

(*** Compiling functions ***)


(* TODO: 
- Cleanup
- More helper functions, especially for using 'find_expression_location'
*)

type location = 
  | ComputeStack of {
    typ: typ;
    instrs: instruction list;
  }
  | StorageStack of { 
    typ: typ;
    instrs: instruction list;
    load: instruction;
    store: instruction;
  }

let label_name name = 
  string_of_int(label_context ()) ^ "_" ^ name

let label name = Label(label_name name)

let label_ref name = LabelRef(label_name name)



let get_expr (Expr(expr,_)) = expr

let rec can_assign target_type value_type = match target_type, value_type with
  | T_Func _, T_Null
  | T_Null, T_Func _ -> true
  | T_Array(st1,_), T_Array(st2,_) -> can_assign st1 st2
  | T_Tuple(ts1), T_Tuple(ts2) -> 
    if List.length ts1 != List.length ts2 then false
    else List.combine ts1 ts2 |> List.for_all (fun ((st1,_),(st2,_)) -> can_assign st1 st2)
  | T_Func(ret1,params1), T_Func(ret2,params2) -> 
    if List.length params1 != List.length params2 then false
    else can_assign ret1 ret2 && List.combine params1 params2 |> List.for_all (uncurry can_assign)
  | t1, t2 -> type_eq t1 t2

let rec can_declare typ = match typ with
  | T_Null -> false
  | T_Array(st,_) -> can_declare st
  | T_Tuple(ts) -> ts |> List.map fst |> List.for_all can_declare
  | T_Func(ret,params) -> can_declare ret && List.for_all can_declare params
  | _ -> true

let identifier_name id =  match id with
  | Var(_,n)
  | Const(_,n,_) -> n

let scope_size ids = List.fold_left (fun acc id -> match id with
    | Const _ -> acc
    | Var(t,_) -> acc + type_size t
  ) 0 ids

let lookup_identifier name scopes =
  let rec aux vars = match vars with
    | [] -> None
    | h::t -> if identifier_name h = name then Some(h, scope_size t) else aux t 
  in
  match aux scopes.local with
  | Some id -> Some(LocalScope, id)
  | None -> (
    match scopes.global |> Option.map (fun scope -> aux scope) |> Option.join with
    | Some id -> Some(GlobalScope, id)
    | None -> None
  )

let rec is_constant state (Expr(expr, _)) = match expr with
  | Int _
  | Direction _
  | Prop _
  | Resource _ -> true
  | IdentifierAccess name -> (match lookup_identifier name state.scopes with
    | Some(_, (Const(_,_,_), _)) -> true
    | _ -> false
  )
  | StructureLiteral exprs -> List.for_all (is_constant state) exprs
  | _ -> false

let is_true i = i > 0

let rec reduce_expression state (Expr(expr, ln)) = Expr(reduce_expr state expr, ln)

and reduce_expr state expr = match expr with
  | Int i -> Int i
  | Direction d -> Direction d
  | Prop p -> Prop p
  | Resource r -> Resource r
  | Binary_op(op, e1, e2) -> (match op, reduce_expression state e1 |> get_expr, reduce_expression state e2 |> get_expr with
    | Plus, Int a, Int b -> Int (a + b)
    | Minus, Int a, Int b -> Int (a - b)
    | Times, Int a, Int b -> Int (a * b)
    | And, Int a, Int b -> Int (if is_true a && is_true b then 1 else 0)
    | Or, Int a, Int b -> Int (if is_true a || is_true b then 1 else 0)
    | Equal, Int a, Int b -> Int (if a = b then 1 else 0)
    | Equal, Direction a, Direction b -> Int (if a = b then 1 else 0)
    | Equal, Resource a, Resource b -> Int (if a = b then 1 else 0) 
    | Equal, Prop a, Prop b -> Int (if a = b then 1 else 0) 
    | NotEqual, Int a, Int b -> Int (if a != b then 1 else 0)
    | NotEqual, Direction a, Direction b -> Int (if a != b then 1 else 0)
    | NotEqual, Resource a, Resource b -> Int (if a != b then 1 else 0) 
    | NotEqual, Prop a, Prop b -> Int (if a != b then 1 else 0)
    | Less, Int a, Int b -> Int (if a < b then 1 else 0)
    | LessOrEqual, Int a, Int b -> Int (if a <= b then 1 else 0)
    | Greater, Int a, Int b -> Int (if a > b then 1 else 0)
    | GreaterOrEqual, Int a, Int b -> Int (if a >= b then 1 else 0)
    | Divide, Int a, Int b -> Int (a / b)
    | Remainder, Int a, Int b -> Int (((a mod b) + b) mod b)
    | _ -> expr
  )
  | Unary_op(op, e) -> (match op, reduce_expression state e |> get_expr with 
    | Negate, Int i -> Int (if is_true i then 0 else 1)
    | _ -> expr
  )
  | IdentifierAccess name when not(is_bound name state.scopes) -> expr
  | IdentifierAccess name -> (match lookup_identifier name state.scopes with
    | Some(_, (Const(_,_, Expr(expr,_)), _)) -> expr
    | _ -> expr
  )
  | ArrayAccess(target, index) -> ArrayAccess(reduce_expression state target, reduce_expression state index) (* Can likely be a little better on constants *)
  | Call(f,args) -> Call(reduce_expression state f, List.map (reduce_expression state) args) (* Can likely be a little better on constants *)
  | _ -> expr

let rec eval_type_expr state te = match te with
  | TE_Int -> T_Int
  | TE_Dir -> T_Dir
  | TE_Field -> T_Field
  | TE_Resource -> T_Resource
  | TE_Array(sub, size_expr) -> (match reduce_expression state size_expr with
    | Expr(Int i,_) when i > 0 -> T_Array(eval_type_expr state sub, i)
    | Expr(Int _,_) -> raise_failure "Array size must be positive"
    | _ -> raise_failure "Array size must be of type 'int'"
  )
  | TE_Tuple _ -> raise_failure "Not implemented"
  | TE_Func(ret, params) -> T_Func(eval_type_expr state ret, List.map (eval_type_expr state) params)

let rec compile_expr (state:compile_state) (Expr(expr, ln)) : (typ * instruction list) =
  try match expr with
  | IdentifierAccess name when not(is_bound name state.scopes) -> (
    match lookup_builtin_info name with
    | Some builtin -> (match builtin.info with
      | BuiltinVar bv -> (bv.typ, bv.comp)
      | BuiltinFunc bf -> (T_Func(bf.ret, bf.canonical), [Instr_Place ; I(bf.addr)])
    )
    | _ -> raise_failure ("Unknown variable: "^name)
  )
  | IdentifierAccess name -> (match lookup_identifier name state.scopes with
    | Some(GlobalScope, (Var(typ,_), addr)) -> (typ, [Instr_Place ; I(addr) ; Instr_LoadGlobal ; I(type_size typ)])
    | Some(LocalScope, (Var(typ,_), addr)) -> (typ, [Instr_Place ; I(addr) ; Instr_LoadLocal ; I(type_size typ)])
    | Some(_, (Const(_,_,expr), _)) -> compile_expr state expr
    | None -> raise_failure ("Unknown identifier: "^name)
  ) 
  | ArrayAccess(target, index) -> (
    let (index_typ, index_instrs) = compile_expr state index in
    if not(type_eq T_Int index_typ) then raise_failure "Index must be of type 'int'" else
    match find_expr_location target state with    (* Not working? *)
    | ComputeStack loc -> (match loc.typ with
      | T_Array(elem_t, size) -> (loc.typ, loc.instrs @ index_instrs @ [Instr_Extract ; I(size * type_size elem_t) ; I(type_size elem_t)])
      | T_Tuple(entries) -> (match index with
        | Expr(Int i,_) when i >= 0 && i < List.length entries-> (match List.nth_opt entries i with
          | Some(elem_t,_) -> (* One off somehow... *)
            let size = List.fold_left (fun acc (t,_) -> acc + type_size t) 0 entries in
            (elem_t, loc.instrs @ index_instrs @ [Instr_Extract ; I(size) ; I(type_size elem_t)])
          | None -> raise_failure "Invalid tuple access"
          )
        | _ -> raise_failure "Invalid tuple access"
      )
      | _ -> raise_failure "Dont know how to access array" 
    )
    | StorageStack loc -> (match loc.typ with
      | T_Array(elem_t, size) -> (elem_t, loc.instrs @ index_instrs @ [Instr_Index ; I(size) ; I(type_size elem_t) ; loc.load ; I(type_size elem_t)])
      | T_Tuple(entries) -> (match index with
        | Expr(Int i,_) when i >= 0 && i < List.length entries-> (match List.nth_opt entries i with
          | Some(elem_t,_) -> 
            let size = List.fold_left (fun acc (t,_) -> acc + type_size t) 0 entries in
            (elem_t, loc.instrs @ index_instrs @ [Instr_Index ; I(size) ; I(type_size elem_t) ; loc.load ; I(type_size elem_t)])
          | None -> raise_failure "Invalid tuple access"
          )
        | _ -> raise_failure "Invalid tuple access"
      )
      | _ -> raise_failure "Dont know how to access array"
    )
  )
  | Int i -> (T_Int, [Instr_Place ; I(i)])
  | Prop fp -> (T_Field, [Instr_Place ; I(prop_index fp)])
  | Resource r -> (T_Resource, [Instr_Place ; I(resource_value r)])
  | Random -> (T_Int, [Instr_Random])
  | RandomSet exprs -> (
    let (types, instrs) = exprs |> List.map (compile_expr state) |> List.split in
    match types with
    | [] -> raise_failure "Empty random set"
    | h::t -> 
      if List.for_all (type_eq h) t 
      then (h, List.flatten instrs @ [Instr_RandomSet ; I(List.length types)])
      else raise_failure "Random set of differing types"
  )
  | Direction d -> (T_Dir, [Instr_Place ; I(int_of_dir d)])
  | Binary_op (op, e1, e2) -> (
    let (typ1, instrs1) = compile_expr state e1 in
    let (typ2, instrs2) = compile_expr state e2 in
    match op, typ1, typ2 with
    | Plus, T_Int, T_Int -> (T_Int, instrs1 @ instrs2 @ [Instr_Add])
    | Minus, T_Int, T_Int -> (T_Int, instrs1 @ instrs2 @ [Instr_Sub])
    | Times, T_Int, T_Int -> (T_Int, instrs1 @ instrs2 @ [Instr_Mul])
    | And, T_Int, T_Int -> (T_Int, instrs1 @ instrs2 @ [Instr_And])
    | Or, T_Int, T_Int -> (T_Int, instrs1 @ instrs2 @ [Instr_Or])
    | Equal, T_Int, T_Int 
    | Equal, T_Dir, T_Dir 
    | Equal, T_Resource, T_Resource
    | Equal, T_Field, T_Field
    | Equal, T_Func _, T_Func _
    | Equal, T_Null, T_Func _
    | Equal, T_Func _, T_Null -> (T_Int, instrs1 @ instrs2 @ [Instr_Eq])
    | NotEqual, T_Int, T_Int 
    | NotEqual, T_Dir, T_Dir 
    | NotEqual, T_Resource, T_Resource 
    | NotEqual, T_Field, T_Field
    | NotEqual, T_Func _, T_Func _
    | NotEqual, T_Null, T_Func _
    | NotEqual, T_Func _, T_Null -> (T_Int, instrs1 @ instrs2 @ [Instr_Eq ; Instr_Not])
    | Less, T_Int, T_Int -> (T_Int, instrs2 @ instrs1 @ [Instr_Lt])
    | Greater, T_Int, T_Int -> (T_Int, instrs1 @ instrs2 @ [Instr_Lt])
    | LessOrEqual, T_Int, T_Int -> (T_Int, instrs1 @ instrs2 @ [Instr_Lt ; Instr_Not])
    | GreaterOrEqual, T_Int, T_Int -> (T_Int, instrs2 @ instrs1 @ [Instr_Lt ; Instr_Not])
    | Divide, T_Int, T_Int ->  (T_Int, instrs2 @ instrs1 @ [Instr_Div])
    | Remainder, T_Int, T_Int -> (T_Int, instrs2 @ instrs1 @ [Instr_Mod])
    | RightShift, T_Dir, T_Int -> (T_Dir, [Instr_Place ; I(4)] @ instrs1 @ instrs2 @ [Instr_Add ; Instr_Mod])
    | LeftShift, T_Dir, T_Int -> (T_Dir, [Instr_Place ; I(4)] @ instrs1 @ instrs2 @ [Instr_Sub ; Instr_Mod])
    | Plus, T_Field, T_Field ->  (T_Field, instrs1 @ instrs2 @ [Instr_BinOr])
    | Minus, T_Field, T_Field -> (T_Field, instrs1 @ instrs2 @ [Instr_BinNot ; Instr_BinAnd]) 
    | IsCompare, T_Field, T_Field -> (T_Int, instrs2 @ [Instr_Copy] @ instrs1 @ [Instr_BinAnd ; Instr_Eq])
    | AnyCompare, T_Field, T_Field -> (T_Int, instrs1 @ instrs2 @ [Instr_BinAnd])
    | _ -> raise_failure "Unknown binary operation"
  )
  | Unary_op (op, e) -> ( 
    let (t, instrs) = compile_expr state e in 
    match op, t with 
      | Negate, T_Int -> (T_Int, instrs @ [Instr_Not])
      | Negate, T_Field -> (T_Field, [Instr_Place ; I(prop_index All_Prop)] @ instrs @ [Instr_BinNot ; Instr_BinAnd])
      | _ -> raise_failure "Unknown unary operation"
  )
  (* true = pre *)
  | Increment(target, true) -> (match find_expr_location target state with
  | StorageStack loc -> (loc.typ, loc.instrs @ [Instr_Copy ; Instr_Copy ; loc.load ; I(type_size loc.typ) ; Instr_Place ; I(1) ; Instr_Add ; Instr_Swap ; loc.store ; I(type_size loc.typ) ; loc.load ; I(type_size loc.typ)])
    | ComputeStack _ -> raise_failure "Could not increment"
  )
  | Decrement(target, true)  -> (match find_expr_location target state with
    | StorageStack loc -> (loc.typ, loc.instrs @ [Instr_Copy ; Instr_Copy ; loc.load ; I(type_size loc.typ) ; Instr_Place ; I(1) ; Instr_Sub ; Instr_Swap ; loc.store ; I(type_size loc.typ) ; loc.load ; I(type_size loc.typ)])
    | ComputeStack _ -> raise_failure "Could not decrement"
  )
  | Increment(target, false) -> (match find_expr_location target state with
    | StorageStack loc -> (loc.typ, loc.instrs @ [Instr_Copy ; loc.load ; I(type_size loc.typ) ; Instr_Swap ; Instr_Copy ; loc.load ; I(type_size loc.typ) ; Instr_Place ; I(1) ; Instr_Add ; Instr_Swap ; loc.store ; I(type_size loc.typ)]) 
    | ComputeStack _ -> raise_failure "Could not increment"
  )
  | Decrement(target, false) -> (match find_expr_location target state with
    | StorageStack loc -> (loc.typ, loc.instrs @ [Instr_Copy ; loc.load ; I(type_size loc.typ) ; Instr_Swap ; Instr_Copy ; loc.load ; I(type_size loc.typ) ; Instr_Place ; I(1) ; Instr_Sub ; Instr_Swap ; loc.store ; I(type_size loc.typ)])
    | ComputeStack _ -> raise_failure "Could not decrement"
  )
  | Func(ret,args,body) -> (
    new_label_context () ;
    let ret = eval_type_expr state ret in
    let args = args |> List.rev |> List.map (fun (t,n) -> (eval_type_expr state t, n)) in
    let func_scope = {  (* Could "this" be a constant ??? *)
      local = List.map (fun (t,n) -> Var(t,n)) args @ [Var(T_Func(ret, List.map fst args), "this")] ; 
      global =  if state.scopes.global = None then Some(state.scopes.local) else state.scopes.global ;
    } in
    let new_state = {
      scopes = func_scope;  
      labels = available_labels body; 
      break = None; 
      continue = None; 
      ret_type = Some ret;
      size = 0;
    } in
    let (state', body) = compile_stmt body {new_state with scopes = ({ local = new_state.scopes.local; global = new_state.scopes.global })} in
    (T_Func(ret, List.map fst args), [Instr_GoTo ; label_ref "func_end" ; label "func" ; Instr_Declare ; I(state'.size)] @ body @ [Instr_Declare ; I(type_size ret) ; Instr_Return ; label "func_end" ; Instr_Place ; label_ref "func"])
  )
  | Call(Expr(IdentifierAccess name,_), args) when not(is_bound name state.scopes) -> (
    let (arg_types, arg_instrs) = args |> List.map (compile_expr state) |> List.split in
    match lookup_builtin_info name with
    | Some builtin -> (match builtin.info with
      | BuiltinVar _ -> raise_failure ("Unknown function: "^name)
      | BuiltinFunc bf -> (
        if (type_list_eq arg_types bf.canonical)
        then 
          (bf.ret, List.flatten arg_instrs @ [Instr_Place ; I(bf.addr) ; Instr_Place ; I(List.length args) ; Instr_Call])
        else match List.find_opt (fun (params, _) -> type_list_eq arg_types params) bf.short_hands with
        | Some(_, comp) -> 
          (bf.ret, arg_instrs |> List.flatten |> comp)
        | None -> raise_failure ("Unknown function: "^name)
      )
    )
    | _ -> raise_failure ("Unknown function: "^name)
  )
  | Call(f,args) -> ( 
    let (f_typ, f_instrs) = compile_expr state f in
    match f_typ with
    | T_Func(ret, params) ->
      (*type check?*)
      let total_params_size = params |> List.map type_size |> List.fold_left (+) 0 in
      if List.length params != List.length args then raise_failure "Incorrect amount of arguments" else
      let comped_args = args 
        |> List.combine params 
        |> List.map (fun (param, arg) -> 
          let (arg_typ, arg_instrs) = compile_expr state arg in
          if can_assign param arg_typ then arg_instrs @ fix_size param arg_typ
          else raise_failure "Incorrect argument type"
        ) 
        |> List.flatten 
      in
      (ret, comped_args @ (f_instrs @ [Instr_Place ; I(total_params_size) ; Instr_Call]))
    | _ -> raise_failure "Calling non-function"
  )
  | Ternary(c,a,b) -> (
    new_label_context ();
    let (c_typ, c_instrs) = compile_expr state c in
    let (a_typ, a_instrs) = compile_expr state a in
    let (b_typ, b_instrs) = compile_expr state b in
    if c_typ != T_Int then raise_failure ("Condition must be of type 'int', but was: " ^ type_string c_typ) else
    if not(type_eq a_typ b_typ) then raise_failure "Ternary type mismatch" else
    (a_typ, c_instrs @ [Instr_GoToIf ; label_ref "ternary_true"] @ b_instrs @ [Instr_GoTo ; label_ref "ternary_stop" ; label "ternary_true"] @ a_instrs @ [label "ternary_stop"])
  )
  | Null -> (T_Null, [Instr_Place ; I(0)])
  | StructureLiteral exprs -> 
    let (typs, instrs) = exprs |> List.map (compile_expr state) |> List.split in
    match List.find_opt can_declare typs with
    | None -> raise_failure "Could not infere structure type"
    | Some typ -> (
      if List.for_all (can_assign typ) typs then (T_Array(typ,List.length typs), List.flatten instrs)
      else if List.for_all can_declare typs then (T_Tuple(typs |> List.map (fun t -> (t, None))), List.flatten instrs)
      else raise_failure "Could not infere structure type"
    )
  with
  | Failure(p,None,msg) -> raise (Failure(p,Some ln, msg))
  | a -> raise a

and fix_size target_typ expr_typ : instruction list = 
  let size_diff = type_size target_typ - type_size expr_typ in
  match size_diff with
  | 0 -> []
  | x when x < 0 -> [Instr_MoveSP ; I(x)] (*arg is too large*)
  | x -> [Instr_Declare ; I(x)] (*arg is too small*)

and find_variable_location name state =
  match lookup_identifier name state.scopes with
  | Some(GlobalScope, (Var(typ,_), addr)) -> Some(StorageStack { typ = typ; instrs = [Instr_Place ; I(addr)]; load = Instr_LoadGlobal; store = Instr_StoreGlobal })
  | Some(LocalScope, (Var(typ,_), addr)) -> Some(StorageStack { typ = typ; instrs = [Instr_Place ; I(addr)]; load = Instr_LoadLocal; store = Instr_StoreLocal })
  | Some(_, (Const(_,_,expr), _)) -> 
    let (expr_type, expr_instrs) = compile_expr state expr in
    Some(ComputeStack { typ = expr_type; instrs = expr_instrs})
  | None -> raise_failure ""

(* This function is crazy, please fix *)
(* Need to take another look at extract and index, to see if they are used correctly *)
(* - Index instruction is using element size, this will not work for tuples in general *)
and find_expr_location (Expr(e,_) as expr) state = match e with
  | IdentifierAccess name -> (match find_variable_location name state with
    | Some loc -> loc
    | None -> raise_failure ""
  )
  | ArrayAccess (target, index) -> (match find_expr_location target state with
    | ComputeStack loc -> (match loc.typ with
      | T_Array(elem_t, array_size) ->
        let (index_typ, index_instrs) = compile_expr state index in
        if (index_typ != T_Int) then raise_failure "Index must be of type 'int'" else
        let elem_size = type_size elem_t in
        ComputeStack { typ = elem_t; instrs = loc.instrs @ index_instrs @ [ Instr_Place ; I(elem_size) ; Instr_Mul ; Instr_Extract ; I(array_size * elem_size) ; I(elem_size)] }
      | T_Tuple(entries) -> (match index with
        | Expr(Int i,_) when i >= 0 && i < List.length entries -> (match List.nth_opt entries i with
          | Some (t,_) ->
            let i = entries |> List.mapi (fun idx (typ,_) -> if idx > i then type_size typ else 0) |> List.fold_left (+) 0 in
            let tuple_size = List.fold_left (fun acc (t,_) -> acc + type_size t) 0 entries in
            ComputeStack { typ = t; instrs = loc.instrs @ [Instr_Place ; I(i) ; Instr_Extract ; I(tuple_size) ; I(type_size t)] }
          | None -> raise_failure ":("
        )
        | _ -> raise_failure ":("
      )
      | _ -> raise_failure ":("
    )
    | StorageStack loc -> (match loc.typ with
      | T_Array(elem_t, array_size) ->
        let (index_typ, index_instrs) = compile_expr state index in
        if (index_typ != T_Int) then raise_failure "Index must be of type 'int'" else
        let elem_size = type_size elem_t in
        StorageStack { loc with typ = elem_t; instrs = loc.instrs @ index_instrs @ [Instr_Place ; I(elem_size) ; Instr_Mul ; Instr_Index ; I(array_size * elem_size) ; I(elem_size)] }
      | T_Tuple(entries) -> (match index with
        | Expr(Int i,_) when i >= 0 && i < List.length entries -> (match List.nth_opt entries i with
          | Some (t,_) ->
            let i = entries |> List.mapi (fun idx (typ,_) -> if idx > i then type_size typ else 0) |> List.fold_left (+) 0 in
            let tuple_size = List.fold_left (fun acc (t,_) -> acc + type_size t) 0 entries in
            StorageStack { loc with typ = t; instrs = loc.instrs @ [Instr_Place ; I(i); Instr_Index ; I(tuple_size) ; I(type_size t)] }
          | None -> raise_failure ":("
        )
        | _ -> raise_failure ":("
      )
      | _ -> raise_failure ":("
    )
  )
  | _ -> 
    let (expr_type, expr_instrs) = compile_expr state expr in
    ComputeStack { typ = expr_type ; instrs = expr_instrs }


and compile_stmts state stmts =
  let rec aux stmts state acc = match stmts with
    | [] -> (state, acc |> List.rev |> List.flatten)
    | h::t -> 
      let (state', instrs) = compile_stmt h state in
      aux t state' (instrs :: acc)
  in
  aux stmts state []

and compile_stmt (Stmt(stmt,ln)) state : (compile_state * instruction list)  =
  try match stmt with
  | If (c, a, b) -> (
    new_label_context () ;
    let (c_typ, c_instrs) = c |> reduce_expression state |> compile_expr state in
    let (_, a_instrs) = compile_stmt a state in
    let (_, b_instrs) = compile_stmt b state in
    match c_typ with
    | T_Int -> (state, c_instrs @ [Instr_GoToIf ; label_ref "if_true"] @ b_instrs @ [Instr_GoTo ; label_ref "if_stop" ; label "if_true"] @ a_instrs @ [label "if_stop"])
    | _ -> raise_failure "Condition must be of type 'int'"
  )
  | IfIs(_) -> raise_failure "IfIs not implemented"(*
    let label_end = Helpers.new_label "ifis_end" in
    let (c_typ, c_instrs) = compile_expr state c in

    let rec comp_alts alts next acc = match alts with
      | [] -> acc
      | (alt_v,alt_s)::t -> Instr_Copy :: compile_expr alt_v state (Instr_Eq :: Instr_Not :: Instr_GoToIf :: LabelRef(next) :: compile_stmt alt_s state (Instr_GoTo :: LabelRef(label_end) :: Label(next) :: (comp_alts t (Helpers.new_label "ifis_alt") acc)))
    in
    match opt with
    | None -> compile_expr v state (comp_alts alts (Helpers.new_label "ifis_alt") (Label(label_end) :: acc))
    | Some other -> compile_expr v state (comp_alts alts (Helpers.new_label "ifis_alt") (Instr_MoveSP :: I(-1) :: compile_stmt other state (Label(label_end) :: acc)))
  *)
  | Block stmts -> (
    let (state', instrs) = compile_stmts state stmts in
    ({state with size = state.size + state'.size}, instrs)
  )
  | While(c,s,None) -> 
    new_label_context () ;
    let (c_typ, c_instrs) = c |> reduce_expression state |> compile_expr state in
    if c_typ != T_Int then raise_failure "Conditon must be of type 'int'" else
    let (_, s_instrs) = compile_stmt s {state with break = Some(label_name "while_stop"); continue = Some(label_name "while_cond") } in
    (state, [Instr_GoTo ; label_ref "while_cond" ; label "while_start"] @ s_instrs @ [label "while_cond"] @ c_instrs @ [Instr_GoToIf ; label_ref "while_start" ; label "while_stop"])
  | While(c,s,Some si) ->
    new_label_context () ; 
    let (c_typ, c_instrs) = c |> reduce_expression state |> compile_expr state in
    if c_typ != T_Int then raise_failure "Conditon must be of type 'int'" else
    let (_, si_instrs) = compile_stmt si state in
    let (_, s_instrs) = compile_stmt s {state with break = Some(label_name "while_stop"); continue = Some(label_name "while_iter") } in
    (state, [Instr_GoTo ; label_ref "while_cond" ; label "while_start"] @ s_instrs @ [label "while_iter"] @ si_instrs @ [label "while_cond"] @ c_instrs @ [Instr_GoToIf ; label_ref "while_start" ; label "while_stop"])
  | Continue -> (match state.continue with
    | Some label -> (state, [Instr_GoTo ; LabelRef(label)])
    | None -> raise_failure "Nothing to continue"
  )
  | Break -> (match state.break with
    | Some label -> (state, [Instr_GoTo ; LabelRef(label)])
    | None -> raise_failure "Nothing to break out of"
  )
  | Assign (target, expr) -> (
    match find_expr_location target state with
    | ComputeStack _ -> raise_failure "Not an assignable target"
    | StorageStack loc -> 
      let (expr_typ, expr_instrs) = expr |> reduce_expression state |> compile_expr state in
      if can_assign loc.typ expr_typ then
        (state, expr_instrs @ fix_size loc.typ expr_typ @ loc.instrs @ [loc.store ; I(type_size loc.typ)])
      else 
        raise_failure ("Cannot assign a value of type '"^type_string expr_typ^"' to a target of type '"^type_string loc.typ^"'")
  )
  | Declare(typ,name) -> 
    let typ = eval_type_expr state typ in
    ({state with scopes = { local = Var(typ,name)::state.scopes.local; global = state.scopes.global }; size = state.size + type_size typ }, [])
  | DeclareAssign (typ_opt, name, expr) -> ( match typ_opt with
    | None -> (
      let (typ, expr_instrs) = expr |> reduce_expression state |> compile_expr state in
      if not(can_declare typ) then  raise_failure ("Cannot declare a variable of type '"^type_string typ^"'")
      else let state' = {state with scopes = { local = Var(typ,name)::state.scopes.local; global = state.scopes.global }; size = state.size + type_size typ} in 
      match find_variable_location name state' with
      | Some StorageStack loc -> (state', expr_instrs @ loc.instrs @ [loc.store ; I(type_size loc.typ)])
      | _ -> raise_failure "Could not assign"
    )
    | Some typ -> (
      let typ = eval_type_expr state typ in
      let (expr_typ, expr_instrs) = expr |> reduce_expression state |> compile_expr state in
      if not(can_assign typ expr_typ) then raise_failure ("Cannot assign a value of type '" ^type_string expr_typ^ "' to a variable of type '" ^type_string typ^ "'") else
      let state' = {state with scopes = { local = Var(typ,name)::state.scopes.local; global = state.scopes.global }; size = state.size + type_size typ} in 
      match find_variable_location name state' with
      | Some StorageStack loc -> (state', expr_instrs @ fix_size typ expr_typ @ loc.instrs @ [loc.store ; I(type_size loc.typ)])
      | _ -> raise_failure "Could not assign"
    )
  )
  | DeclareConst(name,expr) -> 
    let expr = reduce_expression state expr in
    if not(is_constant state expr) then raise_failure "Could not reduce to a constant" else
    ({state with scopes = { local = Const(T_Null,name,expr)::state.scopes.local; global = state.scopes.global } }, [])
  | Label name -> (state, [Label name])
  | GoTo n -> 
    if StringSet.mem n state.labels
    then (state, [Instr_GoTo ; LabelRef n])
    else raise_failure ("Unavailable label: "^n)
  | Return expr -> (
    let (expr_typ, expr_instrs) = expr |> reduce_expression state |> compile_expr state in
    match state.ret_type with
    | Some typ when can_assign typ expr_typ -> 
      (state, expr_instrs @ fix_size typ expr_typ  @ [Instr_Return ; I(type_size typ)])
    | Some typ -> raise_failure ("Cannot return a value of type '"^type_string expr_typ^"' from a function returning '"^type_string typ^"'")
    | None -> raise_failure "Not in a function"
  )
  | ExprStmt expr -> 
    let (typ, instrs) = expr |> reduce_expression state |> compile_expr state in
    (state, instrs @ [Instr_MoveSP ; I(-(type_size typ))])
  with 
  | Failure(p,None,msg) -> raise (Failure(p,Some ln, msg))
  | a -> raise a

let compile_player (File(program,i)) =
  let program = Stmt(Block program, i) in
  let labels = available_labels program in
  let state = {scopes = { local = [] ; global = None }; size = 0; labels = labels; break = None; continue = None; ret_type = None;} in
  let (state, instrs) = compile_stmt program state in
  Instr_Declare :: I(state.size) :: instrs |> Optimize.optimize_instruction_list
