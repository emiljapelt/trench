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
  | T_Array(st1,size1), T_Array(st2,size2) -> size1 = size2 && can_assign st1 st2
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
  | StructureLiteral exprs -> exprs |> List.map snd |> List.for_all (is_constant state) 
  | Func _ -> true
  | _ -> false

let is_true i = i > 0

let rec reduce_expression state (Expr(expr, ln)) = Expr(reduce_expr state expr, ln)

and reduce_expr state expr = match expr with
  | Int i -> Int i
  | Null -> Null
  | Direction d -> Direction d
  | Prop p -> Prop p
  | Resource r -> Resource r
  | Binary_op(op, e1, e2) -> (
    let e1 = reduce_expression state e1 in
    let e2 = reduce_expression state e2 in
    match op, get_expr e1, get_expr e2 with
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
    | Plus, StructureLiteral entries1, StructureLiteral entries2 -> StructureLiteral(entries1 @ entries2)
    | Times, StructureLiteral entries, Int i
    | Times, Int i, StructureLiteral entries -> StructureLiteral(List.init i (fun _ -> entries) |> List.flatten)
    | _ -> Binary_op(op, e1, e2)
  )
  | Unary_op(op, e) -> (
    let e = reduce_expression state e in  
    match op, get_expr e with 
    | Negate, Int i -> Int (if is_true i then 0 else 1)
    | _ -> Unary_op(op, e)
  )
  | IdentifierAccess name when not(is_bound name state.scopes) -> expr
  | IdentifierAccess name -> (match lookup_identifier name state.scopes with
    | Some(_, (Const(_,_, Expr(expr,_)), _)) -> expr
    | _ -> expr
  )
  | IndexAccess(target, Index index) -> (match reduce_expression state target, reduce_expression state index with
    | Expr(StructureLiteral entries, _) as target, Expr(Int i,_) when is_constant state target && i >= 0 && i < List.length entries -> 
      List.nth entries i |> snd |> get_expr
    | target, index  -> IndexAccess(target, Index index)
  )
  | IndexAccess(target, Range(fst, snd)) -> (match reduce_expression state target, Option.map (reduce_expression state) fst, Option.map (reduce_expression state) snd with
    | Expr(StructureLiteral entries, _), None, Some(Expr(Int until,_)) -> StructureLiteral(List.take (until+1) entries)
    | Expr(StructureLiteral entries, _), Some(Expr(Int from,_)), None -> StructureLiteral(List.drop from entries)
    | Expr(StructureLiteral entries, _), Some(Expr(Int from,_)), Some(Expr(Int len,_)) -> StructureLiteral(entries |> List.drop from |> List.take len)
    | target, fst, snd -> IndexAccess(target, Range(fst, snd))
  )
  | TupleAccess(target, name) -> (match reduce_expression state target with
    | Expr(StructureLiteral entries,_) as e -> (match List.find_index (fst >> Option.fold ~none:false ~some:((=) name)) entries with
      | Some i -> List.nth entries i |> snd |> get_expr
      | None -> TupleAccess(e, name)
    )
    | e -> TupleAccess(e, name)
  )
  | Call(f,args) -> Call(reduce_expression state f, List.map (reduce_expression state) args)
  | RandomAccess expr -> RandomAccess(reduce_expression state expr)
  | Ternary(c,a,b) -> (match reduce_expression state c with
    | Expr(Int i, _) -> reduce_expression state (if is_true i then a else b ) |> get_expr
    | c -> Ternary(c, reduce_expression state a, reduce_expression state b)
  )
  | StructureLiteral entries -> StructureLiteral(List.map (fun (name, expr) -> (name, reduce_expression state expr)) entries)
  | SizeOf expr -> (
    let e = reduce_expression state expr in
    match get_expr e with
    | StructureLiteral entries -> Int(List.length entries)
    | IdentifierAccess name -> (match lookup_identifier name state.scopes with
      | Some(_, ((Const(T_Array(_,s),_,_)), _)) -> Int(s)
      | Some(_, ((Const(T_Tuple(entries),_,_)), _)) -> Int(List.length entries)
      | Some(_, ((Var(T_Array(_,s),_)), _)) -> Int(s)
      | Some(_, ((Var(T_Tuple(entries),_)), _)) -> Int(List.length entries)
      | _ -> SizeOf(e)
    )
    | _ -> SizeOf(e)
  )
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
  | TE_Tuple entries -> T_Tuple(List.map (fun (t,n) -> (eval_type_expr state t, n)) entries)
  | TE_Func(ret, params) -> T_Func(eval_type_expr state ret, List.map (eval_type_expr state) params)

let rec compile_expr (state:compile_state) (Expr(expr, ln) as expression) : (typ * instruction list) =
  try match expr with
  | IdentifierAccess name when not(is_bound name state.scopes) -> (
    match lookup_builtin_info name with
    | Some builtin -> (match builtin.info with
      | BuiltinVar bv -> (bv.typ, bv.comp)
      | BuiltinFunc bf -> (T_Func(bf.ret, bf.canonical), [Instr_Place ; I(bf.addr)])
    )
    | _ -> raise_failure ("Unknown variable: "^name)
  )
  | IdentifierAccess _ 
  | IndexAccess _ 
  | TupleAccess _ -> (match find_expr_location expression state with
    | StorageStack loc -> (loc.typ, loc.instrs @ [loc.load ; I(type_size loc.typ)])
    | ComputeStack loc -> (loc.typ, loc.instrs)
  )
  | Int i -> (T_Int, [Instr_Place ; I(i)])
  | Prop fp -> (T_Field, [Instr_Place ; I(prop_index fp)])
  | Resource r -> (T_Resource, [Instr_Place ; I(resource_value r)])
  | Random -> (T_Int, [Instr_Random])
  | RandomAccess expr -> (match compile_expr state expr with
    | (T_Array(typ, size), instrs) -> 
      let elem_size = type_size typ in
      (typ, instrs @ [Instr_Place ; I(size) ; Instr_Random ; Instr_Mod ; Instr_Place; I(elem_size) ; Instr_Mul ; Instr_Extract ; I(size * elem_size) ; I(elem_size) ])
    | (typ,_) -> raise_failure ("Cannot do random access on value type: "^type_string typ)
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
    | Plus, T_Array(t1,s1), T_Array(t2,s2) when type_eq t1 t2 -> (T_Array(t1, s1+s2), instrs1 @ instrs2)
    | Plus, T_Tuple(entries1), T_Tuple(entries2) -> (T_Tuple(entries1 @ entries2), instrs1 @ instrs2)
    | Times, T_Int, T_Array(t,s) -> (match e1 with
      | Expr(Int i, _) -> (T_Array(t, s*i), instrs2 |> return |> List.init i |> List.flatten)
      | _ -> raise_failure "Array multiplication requires a constant factor"
    ) 
    | Times, T_Array(t,s), T_Int -> (match e2 with
      | Expr(Int i, _) -> (T_Array(t, s*i), instrs1 |> return |> List.init i |> List.flatten)
      | _ -> raise_failure "Array multiplication requires a constant factor"
    )
    | Times, T_Int, T_Tuple(entries) -> (match e1 with
      | Expr(Int i, _) -> (T_Tuple(entries |> return |> List.init i |> List.flatten), instrs2 |> return |> List.init i |> List.flatten)
      | _ -> raise_failure "Tuple multiplication requires a constant factor"
    ) 
    | Times, T_Tuple(entries), T_Int -> (match e2 with
      | Expr(Int i, _) -> (T_Tuple(entries |> return |> List.init i |> List.flatten), instrs1 |> return |> List.init i |> List.flatten)
      | _ -> raise_failure "Tuple multiplication requires a constant factor"
    ) 
    | _ -> raise_failure ("Unknown binary operation between '" ^type_string typ1^ "' and '" ^type_string typ2^ "'")
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
  | Func f -> (match f.cache with
    | Some(typ,label) -> (typ, [Instr_Place ; LabelRef label])
    | None ->
      let (ret,args,body) = f.data in
      new_label_context () ;
      let start_label = label_name "func" in
      let end_label = label_name "func_end" in
      let ret = eval_type_expr state ret in
      let args = List.map (fun (t,n) -> (eval_type_expr state t, n)) args  in
      let typ = T_Func(ret, List.map fst args) in
      f.cache <- Some(typ, start_label) ;
      let func_scope = {
        local = List.fold_left (fun acc (t,n) -> Var(t,n)::acc) [Const(typ, "this", expression)] args ; 
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
      (typ, [Instr_GoTo ; LabelRef end_label ; Label start_label ; Instr_Declare ; I(state'.size)] @ body @ [Instr_Declare ; I(type_size ret) ; Instr_Return ; I(type_size ret) ; Label end_label ; Instr_Place ; LabelRef start_label])
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
        else match List.find_opt (fst >> type_list_eq arg_types) bf.short_hands with
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
          if can_assign param arg_typ then arg_instrs
          else raise_failure ("Incorrect argument for function of type: '"^type_string f_typ ^"'")
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
  | StructureLiteral exprs -> (
    let info = List.map (fun (name, expr) -> (name, compile_expr state expr)) exprs in
    let names = List.map fst info in 
    let typs = List.map (snd >> fst) info in
    let instrs = List.map (snd >> snd) info in
    match List.find_opt can_declare typs with
    | None -> raise_failure "Could not infere structure type"
    | Some typ -> (
      if List.for_all (can_assign typ) typs && List.for_all Option.is_none names then (T_Array(typ,List.length typs), List.flatten instrs)
      else if List.for_all can_declare typs then (T_Tuple(List.combine typs names), List.flatten instrs)
      else raise_failure "Could not infere structure type"
    )
  )
  | SizeOf expr -> (match compile_expr state expr with
    | (T_Array(_,s), _) -> (T_Int, [Instr_Place ; I(s)])
    | (T_Tuple(entries), _) -> (T_Int, [Instr_Place ; I(List.length entries)])
    | (t, _) -> raise_failure ("Cannot get size of type: " ^ type_string t)
  )
  with
  | Failure(p,None,msg) -> raise (Failure(p,Some ln, msg))
  | a -> raise a

and find_identifier_location name state =
  match lookup_identifier name state.scopes with
  | Some(GlobalScope, (Var(typ,_), addr)) -> Some(StorageStack { typ = typ; instrs = [Instr_Place ; I(addr)]; load = Instr_LoadGlobal; store = Instr_StoreGlobal })
  | Some(LocalScope, (Var(typ,_), addr)) -> Some(StorageStack { typ = typ; instrs = [Instr_Place ; I(addr)]; load = Instr_LoadLocal; store = Instr_StoreLocal })
  | Some(_, (Const(_,_,expr), _)) -> 
    let (expr_type, expr_instrs) = compile_expr state expr in
    Some(ComputeStack { typ = expr_type; instrs = expr_instrs})
  | None -> None

(* New function name ??? *)
and find_expr_location (Expr(e,_) as expr) state = match e with
  | IdentifierAccess name -> (match find_identifier_location name state with
    | Some loc -> loc
    | None -> raise_failure ("Unknown identifier: " ^ name)
  )
  | IndexAccess (target, range) -> (match find_expr_location target state with
    | ComputeStack loc -> (match loc.typ with
      | T_Array(elem_t, array_size) -> (
        let elem_size = type_size elem_t in
        match range with 
        | Index index ->
          let (index_typ, index_instrs) = compile_expr state index in
          if (index_typ != T_Int) then raise_failure "Index must be of type 'int'" else
          ComputeStack { typ = elem_t; instrs = loc.instrs @ index_instrs @ [ Instr_Place ; I(elem_size) ; Instr_Mul ; Instr_Extract ; I(array_size * elem_size) ; I(elem_size)] }
        | Range(Some index, Some Expr(Int len,_)) -> 
          if abs len > array_size then raise_failure "Length of the range is greater than the length of the array" else
          let (index_typ, index_instrs) = compile_expr state index in
          if not(type_eq T_Int index_typ) then raise_failure "Index must be of type 'int'" else
          let offset = if len < 0 then [Instr_Place ; I(len + 1) ; Instr_Add] else [] in
          ComputeStack { typ = T_Array(elem_t, abs len) ; instrs = loc.instrs @ index_instrs @ offset @ [Instr_Place ; I(elem_size) ; Instr_Mul ; Instr_Extract ; I(array_size * elem_size) ; I(abs len * elem_size)] }
        | Range(None, Some Expr(Int index,_)) -> (* a[..2] *)
          if (index < 0 || index >= array_size) then raise_failure "Out of bounds" else
          let len = index + 1 in
          ComputeStack { typ = T_Array(elem_t, len); instrs = loc.instrs @ [Instr_Place ; I(0) ; Instr_Extract ; I(array_size * elem_size) ; I(len * elem_size)] }
        | Range(Some Expr(Int index,_), None) -> (* a[1..] *)
          if (index < 0 || index >= array_size) then raise_failure "Out of bounds" else
          let len = array_size - index in
          ComputeStack { typ = T_Array(elem_t, len); instrs = loc.instrs @ [Instr_Place ; I(index * elem_size) ; Instr_Extract ; I(array_size * elem_size) ; I(len * elem_size)] }
        | _ -> raise_failure "Not implemented: 2"
      )
      | T_Tuple(entries) -> (match range with
        | Index Expr(Int i,_) when i >= 0 && i < List.length entries -> (
          let (t,_) = List.nth entries i in
          let sizes = List.map (fst >> type_size) entries in
          let i = sizes |> List.take i |> List.fold_left (+) 0 in
          let tuple_size = sizes |> List.fold_left (+) 0 in
          ComputeStack { typ = t; instrs = loc.instrs @ [Instr_Place ; I(i) ; Instr_Extract ; I(tuple_size) ; I(type_size t)] }
        )
        | _ -> raise_failure "Not a valid index"
      )
      | _ -> raise_failure "Cannot index type"
    )
    | StorageStack loc -> (match loc.typ with
      | T_Array(elem_t, array_size) -> (
        let elem_size = type_size elem_t in
        match range with
        | Index index ->
          let (index_typ, index_instrs) = compile_expr state index in
          if (index_typ != T_Int) then raise_failure "Index must be of type 'int'" else
          StorageStack { loc with typ = elem_t; instrs = loc.instrs @ index_instrs @ [Instr_Place ; I(elem_size) ; Instr_Mul ; Instr_Index ; I(array_size * elem_size) ; I(elem_size)] }
        | Range(Some index, Some Expr(Int len,_)) ->
          if abs len > array_size then raise_failure "Length of the range is greater than the length of the array" else
          let (index_typ, index_instrs) = compile_expr state index in
          if not(type_eq T_Int index_typ) then raise_failure "Index must be of type 'int'" else
          let offset = if len < 0 then [Instr_Place ; I(len + 1) ; Instr_Add] else [] in
          StorageStack { loc with typ = T_Array(elem_t, abs len) ; instrs = loc.instrs @ index_instrs @ offset @ [Instr_Place ; I(elem_size) ; Instr_Mul ; Instr_Index ; I(array_size * elem_size) ; I(abs len * elem_size)] }
        | Range(None, Some Expr(Int index,_)) -> (* a[..2] *)
          if (index < 0 || index >= array_size) then raise_failure "Out of bounds" else
          let len = index + 1 in
          StorageStack { loc with typ = T_Array(elem_t, len); instrs = loc.instrs }
        | Range(Some Expr(Int index,_), None) -> (* a[1..] *)
          if (index < 0 || index >= array_size) then raise_failure "Out of bounds" else
          let len = array_size - index in
          StorageStack { loc with typ = T_Array(elem_t, len); instrs = loc.instrs @ [Instr_Place ; I(index * elem_size) ; Instr_Add]}
        | _ -> raise_failure "Not implemented: 3"
      )
      | T_Tuple(entries) -> (match range with
        | Index Expr(Int i,_) when i >= 0 && i < List.length entries -> (
          let (elem_t,_) = List.nth entries i in
          let sizes = List.map (fst >> type_size) entries in
          let i = sizes |> List.take i |> List.fold_left (+) 0 in
          StorageStack { loc with typ = elem_t; instrs = loc.instrs @ [Instr_Place ; I(i); Instr_Add] }
        )
        | _ -> raise_failure "Not a valid index"
      )
      | _ -> raise_failure "Cannot index type"
    )
  )
  | TupleAccess (target, name) -> (match find_expr_location target state with
    | ComputeStack loc -> (match loc.typ with
      | T_Tuple(entries) -> (
        match List.find_index (snd >> Option.fold ~none:false ~some:((=) name)) entries with
        | None -> raise_failure ("No such entry: "^name)
        | Some index -> 
          let (t,_) = List.nth entries index in
          let sizes = List.map (fst >> type_size) entries in
          let i = sizes |> List.take index |> List.fold_left (+) 0 in
          let tuple_size = sizes |> List.fold_left (+) 0 in
          ComputeStack {typ = t; instrs = loc.instrs @ [Instr_Place ; I(i) ; Instr_Extract ; I(tuple_size) ; I(type_size t)]}
        )
      | _ -> raise_failure ("No such entry: '" ^name^ "' in a value of type: " ^type_string loc.typ^ "'")
    )
    | StorageStack loc -> (match loc.typ with
      | T_Tuple(entries) -> (
        match List.find_index (snd >> Option.fold ~none:false ~some:((=) name)) entries with
        | None -> raise_failure ("No such entry: "^name)
        | Some index -> 
          let (elem_t,_) = List.nth entries index in
          let sizes = List.map (fst >> type_size) entries in
          let i = sizes |> List.take index |> List.fold_left (+) 0 in
          StorageStack { loc with typ = elem_t; instrs = loc.instrs @ [Instr_Place ; I(i); Instr_Add] }
      )
    | _ -> raise_failure ("No such entry: '" ^name^ "' in a value of type: " ^type_string loc.typ^ "'")
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
        (state, expr_instrs @ loc.instrs @ [loc.store ; I(type_size loc.typ)])
      else 
        raise_failure ("Cannot assign a value of type '"^type_string expr_typ^"' to a target of type '"^type_string loc.typ^"'")
  )
  | Declare(typ,name) -> 
    let typ = eval_type_expr state typ in
    ({state with scopes = { local = Var(typ,name)::state.scopes.local; global = state.scopes.global }; size = state.size + type_size typ }, [])
  | DeclareAssign (typ_opt, name, expr) -> (match typ_opt with
    | None -> (
      let (typ, expr_instrs) = expr |> reduce_expression state |> compile_expr state in
      if not(can_declare typ) then  raise_failure ("Cannot declare a variable of type '"^type_string typ^"'")
      else let state' = {state with scopes = { local = Var(typ,name)::state.scopes.local; global = state.scopes.global }; size = state.size + type_size typ} in 
      match find_identifier_location name state' with
      | Some StorageStack loc -> (state', expr_instrs @ loc.instrs @ [loc.store ; I(type_size loc.typ)])
      | _ -> raise_failure "Could not assign"
    )
    | Some typ -> (
      let typ = eval_type_expr state typ in
      let (expr_typ, expr_instrs) = expr |> reduce_expression state |> compile_expr state in
      if not(can_assign typ expr_typ) then raise_failure ("Cannot assign a value of type '" ^type_string expr_typ^ "' to a variable of type '" ^type_string typ^ "'") else
      let state' = {state with scopes = { local = Var(typ,name)::state.scopes.local; global = state.scopes.global }; size = state.size + type_size typ} in 
      match find_identifier_location name state' with
      | Some StorageStack loc -> (state', expr_instrs @ loc.instrs @ [loc.store ; I(type_size loc.typ)])
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
      (state, expr_instrs @ [Instr_Return ; I(type_size typ)])
    | Some typ -> raise_failure ("Cannot return a value of type '"^type_string expr_typ^"' from a function returning '"^type_string typ^"'")
    | None -> raise_failure "Not in a function"
  )
  | Repeat(times, stmt) -> (
    new_label_context () ;
    let times = Option.map (reduce_expression state) times in
    match times with
    | None -> 
      let start = label_name "repeat_start" in
      let stop = label_name "repeat_stop" in 
      let state' = {state with break = Some stop; continue = Some start } in
      let (_, instrs) = compile_stmt stmt state' in
      (state, Label start :: instrs @ [Instr_GoTo ; LabelRef start ; Label stop])
    | Some Expr(Int i, _) -> 
      let stop = label_name "repeat_stop" in
      let continues = List.init i (fun i -> label_name ("repeat_"^string_of_int i)) in
      let instrs = List.map (fun cont -> 
        let (_,instrs) = compile_stmt stmt {state with break = Some stop; continue = Some cont} in
        instrs @ [Label cont]
      ) continues in
      (state, List.flatten instrs @ [Label stop])
    | _ -> raise_failure "Not supported"
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
