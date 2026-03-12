open Absyn
open ProgramRep
open Exceptions
(*open Typing*)
open Field_props
(*open Transform*)
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
    expr: expression;
  }
  | StorageStack of { 
    typ: typ;
    address: instruction list;
    load: instruction;
    store: instruction;
  }

(*let size_of_vars (vars : identifier list) = 
  let rec aux vars acc = match vars with
    | [] -> acc
    | Var(typ,_)::t -> aux t (acc + type_size typ)
    | Const(_)::t -> aux t acc 
  in
  aux vars 0*)

let get_expr (Expr(expr,_)) = expr

let can_assign target_type value_type = match target_type, value_type with
      | T_Func _, T_Null
      | T_Null, T_Func _ -> true
      | t1, t2 -> type_eq t1 t2

let find_variable_location name scopes =
  let rec aux vars load store i : location option = match vars with
    | [] -> None
    | Var(ty,n)::t ->
      if n = name 
      then Some(StorageStack {typ = ty; address = [Instr_Place ; I(i)]; load = Instr_LoadLocal; store = Instr_StoreLocal }) 
      else aux t load store (i+type_size ty)
    | Const(ty,n,expr)::t -> 
      if n = name 
      then Some(ComputeStack {typ = ty; expr = expr}) 
      else aux t load store i
  in
  match aux scopes.local Instr_LoadLocal Instr_StoreLocal 0 with
  | Some loc -> loc
  | None -> (
    match scopes.global |> Option.map (fun scope -> aux scope Instr_LoadGlobal Instr_StoreGlobal 0) |> Option.join with
    | Some loc -> loc
    | None -> raise_failure ("No such variable: "^name)
  )


let is_constant state (Expr(expr, _)) = match expr with
  | Int _
  | Direction _
  | Prop _
  | Resource _ -> true
  | IdentifierAccess name -> (match find_variable_location name state.scopes with
    | ComputeStack _ -> true
    | _ -> false
  )
  | _ -> false

let rec reduce_expression state (Expr(expr, ln)) = Expr(reduce_expr state expr, ln)

and reduce_expr state expr = match expr with
  | Int i -> Int i
  | Direction d -> Direction d
  | Prop p -> Prop p
  | Resource r -> Resource r
  | Binary_op(op, e1, e2) -> (match op, reduce_expression state e1 |> get_expr, reduce_expression state e2 |> get_expr with
    | Plus, Int i1, Int i2 -> Int (i1 + i2)
    | Minus, Int i1, Int i2 -> Int (i1 - i2)
    | Times, Int i1, Int i2 -> Int (i1 * i2)
    | Divide, Int i1, Int i2 -> Int (i1 / i2)
    | _ -> expr
  )
  | IdentifierAccess name -> (match find_variable_location name state.scopes with
    | ComputeStack loc -> get_expr loc.expr
    | _ -> expr
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
  | TE_Tuple _ -> raise_failure "Not implemented"
  | TE_Func(ret, params) -> T_Func(eval_type_expr state ret, List.map (eval_type_expr state) params)

let rec compile_expr (state:compile_state) (Expr(expr, ln) as e) : (typ * instruction list) =
  try match expr with
  | IdentifierAccess name when not(is_bound name state.scopes) -> (
    match lookup_builtin_info name with
    | Some builtin -> (match builtin.info with
      | BuiltinVar bv -> (bv.typ, bv.comp)
      | BuiltinFunc bf -> (T_Func(bf.ret, bf.canonical), [Instr_Place ; I(bf.addr)])
    )
    | _ -> raise_failure ("Unknown variable: "^name)
  )
  | IdentifierAccess _ -> ( match find_expr_location e state with
    | StorageStack loc -> (loc.typ, loc.address @ [loc.load ; I(type_size loc.typ)])
    | ComputeStack loc -> compile_expr state loc.expr
  ) 
  | ArrayAccess(target, index) -> (
    let (index_typ, index_instrs) = compile_expr state index in
    if not(type_eq T_Int index_typ) then raise_failure "Index must be of type 'int'" else
    match find_expr_location target state with
    | ComputeStack loc -> (match loc.typ with
      | T_Array(elem_t, size) -> 
        let (_,target_instrs) = compile_expr state target in 
        (elem_t, target_instrs @ index_instrs @ [Instr_Extract ; I(size * type_size elem_t) ; I(type_size elem_t)])
      | _ -> raise_failure "Dont know how to access array"
    )
    | StorageStack loc -> (match loc.typ with
      | T_Array(elem_t, size) -> (elem_t, loc.address @ (index_instrs @ (Instr_Index :: I(size) :: I(type_size elem_t) :: loc.load :: I(type_size elem_t) :: [])))
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
    (*List.fold_left (fun acc v -> compile_expr v state acc) (Instr_RandomSet :: I(List.length vals) :: acc) vals*)
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
    | Equal, T_Null, T_Func _
    | Equal, T_Func _, T_Null -> (T_Int, instrs1 @ instrs2 @ [Instr_Eq])
    | NotEqual, T_Int, T_Int 
    | NotEqual, T_Dir, T_Dir 
    | NotEqual, T_Resource, T_Resource 
    | NotEqual, T_Field, T_Field
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
  | StorageStack loc -> (loc.typ, loc.address @ [Instr_Copy ; Instr_Copy ; loc.load ; I(type_size loc.typ) ; Instr_Place ; I(1) ; Instr_Add ; Instr_Swap ; loc.store ; I(type_size loc.typ) ; loc.load ; I(type_size loc.typ)])
    | ComputeStack _ -> raise_failure "Could not increment"
  )
  | Decrement(target, true)  -> (match find_expr_location target state with
    | StorageStack loc -> (loc.typ, loc.address @ [Instr_Copy ; Instr_Copy ; loc.load ; I(type_size loc.typ) ; Instr_Place ; I(1) ; Instr_Sub ; Instr_Swap ; loc.store ; I(type_size loc.typ) ; loc.load ; I(type_size loc.typ)])
    | ComputeStack _ -> raise_failure "Could not decrement"
  )
  | Increment(target, false) -> (match find_expr_location target state with
    | StorageStack loc -> (loc.typ, loc.address @ [Instr_Copy ; loc.load ; I(type_size loc.typ) ; Instr_Swap ; Instr_Copy ; loc.load ; I(type_size loc.typ) ; Instr_Place ; I(1) ; Instr_Add ; Instr_Swap ; loc.store ; I(type_size loc.typ)]) 
    | ComputeStack _ -> raise_failure "Could not increment"
  )
  | Decrement(target, false) -> (match find_expr_location target state with
    | StorageStack loc -> (loc.typ, loc.address @ [Instr_Copy ; loc.load ; I(type_size loc.typ) ; Instr_Swap ; Instr_Copy ; loc.load ; I(type_size loc.typ) ; Instr_Place ; I(1) ; Instr_Sub ; Instr_Swap ; loc.store ; I(type_size loc.typ)])
    | ComputeStack _ -> raise_failure "Could not decrement"
  )
  | Func(ret,args,body) -> (
    let func_label = Helpers.new_label "func" in
    let end_label = Helpers.new_label "func_end" in
    let ret = eval_type_expr state ret in
    let args = List.map (fun (t,n) -> (eval_type_expr state t, n)) args in
    let func_scope = {  (* Could "this" be a constant ??? *)
      local = (Var(T_Func(ret, List.map fst args), "this") :: List.map (fun (t,n) -> Var(t,n)) args) ; 
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
    (*let vars = extract_declarations body |> List.rev in*)
    let (state', body) = compile_stmt body {new_state with scopes = ({ local = new_state.scopes.local; global = new_state.scopes.global })} in
    (T_Func(ret, List.map fst args), [Instr_GoTo ; LabelRef(end_label) ; Label(func_label) ; Instr_Declare ; I(state'.size)] @ body @ [Instr_Declare ; I(type_size ret) ; Instr_Return ; Label(end_label) ; Instr_Place ; LabelRef(func_label)])
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
      let comped_args = args 
        |> List.combine params 
        |> List.map (fun (param, arg) -> 
          let (arg_typ, arg_instrs) = compile_expr state arg in
          arg_instrs @ fix_size param arg_typ) 
        |> List.flatten 
      in
      (ret, comped_args @ (f_instrs @ [Instr_Place ; I(total_params_size) ; Instr_Call]))
    | _ -> raise_failure "Calling non-function"
  )
  | Ternary(c,a,b) -> (
    let label_true = Helpers.new_label "ternary_true" in
    let label_stop = Helpers.new_label "ternary_stop" in
    let (c_typ, c_instrs) = compile_expr state c in
    let (a_typ, a_instrs) = compile_expr state a in
    let (b_typ, b_instrs) = compile_expr state b in
    if c_typ != T_Int then raise_failure ("Condition must be of type 'int', but was: " ^ type_string c_typ) else
    if not(type_eq a_typ b_typ) then raise_failure "Ternary type mismatch" else
    (a_typ, c_instrs @ [Instr_GoToIf ; LabelRef(label_true)] @ b_instrs @ [Instr_GoTo ; LabelRef(label_stop) ; Label(label_true)] @ a_instrs @ [Label(label_stop)])
  )
  | Null -> (T_Null, [Instr_Place ; I(0)])
  | ArrayLiteral exprs -> 
    let (typs, instrs) = exprs |> List.map (compile_expr state) |> List.split in
    match typs with
    | [] -> raise_failure "Empty structure literal"
    | h::t -> 
      if List.for_all (type_eq h) t 
      then (T_Array(h,List.length typs), List.flatten instrs)
      else raise_failure "TODO: Tuples!"
  with
  | Failure(p,None,msg) -> raise (Failure(p,Some ln, msg))
  | a -> raise a

and fix_size target_typ expr_typ : instruction list = 
  let size_diff = type_size target_typ - type_size expr_typ in
  match size_diff with
  | 0 -> []
  | x when x < 0 -> [Instr_MoveSP ; I(x)] (*arg is too large*)
  | x -> [Instr_Declare ; I(x)] (*arg is too small*)

(*and find_expr_location e state = match e with
  | Expr(e,_) -> find_expression_location e state*)

and find_expr_location (Expr(e,_) as expr) state = match e with
  | IdentifierAccess name -> find_variable_location name state.scopes
  | ArrayAccess (target, index) -> (match compile_expr state target, find_expr_location target state with
    | _, ComputeStack info -> ComputeStack info
    | (T_Array(elem_t, array_size), _), StorageStack loc -> 
      let (index_typ, index_instrs) = compile_expr state index in
      if (index_typ != T_Int) then raise_failure "Index must be od type 'int'" else
      StorageStack { loc with address = loc.address @ index_instrs @ [Instr_Index ; I(array_size) ; I(type_size elem_t)] }
    | _ -> raise_failure ""
  )
  | _ -> ComputeStack { typ = T_Null (* NOPE, is this even needed? *) ; expr = expr }


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
    let label_true = Helpers.new_label "if_true" in
    let label_stop = Helpers.new_label "if_stop" in
    let (c_typ, c_instrs) = compile_expr state c in
    let (_, a_instrs) = compile_stmt a state in
    let (_, b_instrs) = compile_stmt b state in
    match c_typ with
    | T_Int -> (state, c_instrs @ [Instr_GoToIf ; LabelRef(label_true)] @ b_instrs @ [Instr_GoTo ; LabelRef(label_stop) ; Label(label_true)] @ a_instrs @ [Label(label_stop)])
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
    let (c_typ, c_instrs) = compile_expr state c in
    if c_typ != T_Int then raise_failure "Conditon must be of type 'int'" else
    let label_cond = Helpers.new_label "while_cond" in
    let label_start = Helpers.new_label "while_start" in
    let label_stop = Helpers.new_label "while_stop" in
    let (_, s_instrs) = compile_stmt s {state with break = Some label_stop; continue = Some label_cond } in
    (state, [Instr_GoTo ; LabelRef(label_cond) ; Label(label_start)] @ s_instrs @ [Label(label_cond)] @ c_instrs @ [Instr_GoToIf ; LabelRef(label_start) ; Label(label_stop)])
  | While(c,s,Some si) -> 
    let (c_typ, c_instrs) = compile_expr state c in
    if c_typ != T_Int then raise_failure "Conditon must be of type 'int'" else
    let label_cond = Helpers.new_label "while_iter_cond" in
    let label_start = Helpers.new_label "while_iter_start" in
    let label_stop = Helpers.new_label "while_iter_stop" in
    let label_iter = Helpers.new_label "while_iter_iter" in
    let (_, si_instrs) = compile_stmt si state in
    let (_, s_instrs) = compile_stmt s {state with break = Some label_stop; continue = Some label_iter } in
    (state, [Instr_GoTo ; LabelRef(label_cond) ; Label(label_start)] @ s_instrs @ [Label(label_iter)] @ si_instrs @ [Label(label_cond)] @ c_instrs @ [Instr_GoToIf ; LabelRef(label_start) ; Label(label_stop)])
  | Continue -> (match state.continue with
    | Some label -> (state, [Instr_GoTo ; LabelRef(label)])
    | None -> raise_failure "Nothing to continue"
  )
  | Break -> (match state.break with
    | Some label -> (state, [Instr_GoTo ; LabelRef(label)])
    | None -> raise_failure "Nothing to break out of"
  )
  | Assign (target, expr) -> ( (*type check*)
    match find_expr_location target state with
    | ComputeStack _ -> raise_failure "Could not assign"
    | StorageStack loc -> 
      let (expr_typ, expr_instrs) = compile_expr state expr in
      (state, expr_instrs @ fix_size loc.typ expr_typ @ loc.address @ [loc.store ; I(type_size loc.typ)])
  )
  | Declare(typ,name) -> 
    let typ = eval_type_expr state typ in
    ({state with scopes = { local = Var(typ,name)::state.scopes.local; global = state.scopes.global }; size = state.size + type_size typ }, [])
  | DeclareAssign (typ_opt, name, expr) -> ( match typ_opt with
    | None -> (
      let (typ, expr_instrs) = compile_expr state expr in
      if typ == T_Null then raise_failure "Cannot infere a type from null" else
      let state' = {state with scopes = { local = Var(typ,name)::state.scopes.local; global = state.scopes.global }; size = state.size + type_size typ} in 
      match find_variable_location name state'.scopes with
      | ComputeStack _ -> raise_failure "Could not assign"
      | StorageStack loc -> (state', expr_instrs @ loc.address @ [loc.store ; I(type_size loc.typ)])
    )
    | Some typ -> (
      let typ = eval_type_expr state typ in
      let (expr_typ, expr_instrs) = compile_expr state expr in
      if not(can_assign typ expr_typ) then raise_failure ("Cannot assign a value of type '" ^type_string expr_typ^ "' to a variable of type '" ^type_string typ^ "'") else
      let state' = {state with scopes = { local = Var(typ,name)::state.scopes.local; global = state.scopes.global }; size = state.size + type_size typ} in 
      match find_variable_location name state'.scopes with
      | ComputeStack _ -> raise_failure "Could not assign"
      | StorageStack loc -> (state', expr_instrs @ fix_size typ expr_typ @ loc.address @ [loc.store ; I(type_size loc.typ)])
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
  | Return expr -> (match state.ret_type with  (*type check*)
    | Some t -> 
      let (expr_typ, expr_instrs) = compile_expr state expr in
      (state, expr_instrs @ fix_size t expr_typ  @ [Instr_Return ; I(type_size t)])
    | None -> raise_failure ""
  )
  | ExprStmt e -> 
    let (typ, instrs) = compile_expr state e in
    (state, instrs @ [Instr_MoveSP ; I(-(type_size typ))])
  with 
  | Failure(p,None,msg) -> raise (Failure(p,Some ln, msg))
  | a -> raise a

(* TODO: Maybe global scope could just be empty ??? *)

let compile_player (File(program,i)) =
  let program = Stmt(Block program, i) in
  (* let vars = extract_declarations program |> List.rev in *)
  (* Does the returned state hold this information ??? *)
  let labels = available_labels program in
  let state = {scopes = { local = [] ; global = None }; size = 0; labels = labels; break = None; continue = None; ret_type = None;} in
  let (state, instrs) = compile_stmt program state in
  Instr_Declare :: I(state.size) :: instrs
