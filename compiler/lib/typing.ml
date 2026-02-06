open Absyn
open ProgramRep
open Exceptions
open Builtins
open Helpers

let require req_typ expr_t res = 
  if type_eq req_typ expr_t then res ()
  else raise_failure ("required type: '" ^type_string req_typ^ "' but got: '" ^type_string expr_t^ "'")

let var_type scopes name = 
  match List.find_opt (fun (Var(_,vn)) -> vn = name) scopes.local with
  | Some Var(t,_) -> t
  | None -> (
    match Option.map (List.find_opt (fun (Var(_,vn)) -> vn = name)) scopes.global |> Option.join with
    | Some Var(t,_) -> t
    | None -> raise_failure ("No such variable: "^name)
  )

let assignable (Expr(expr,_)) = match expr with
  | VarAccess _ 
  | ArrayAccess _ -> true
  | _ -> false

let get_type (Expr(_, (_,t))) = t
let get_line (Expr(_, (ln,_))) = ln

let rec type_expr (e : int expr) state : ((int * typ) expr * typ) = match e with
  | VarAccess name when not (is_var name state.scopes) -> (
    match lookup_builtin_info name with
    | Some builtin -> (VarAccess name, builtin_canonical_type builtin)
    | _ -> raise_failure ("Unknown variable: "^name)
    )
  | VarAccess name -> 
    let typ = var_type state.scopes name in
    (VarAccess name, typ)
  | ArrayAccess(target, index) -> (
    let index = type_expression index state in
    let index_type = get_type index in
    let target = type_expression target state in
    let target_type = get_type target in
    match target_type, index_type with
    | T_Array(t,_), T_Int -> (ArrayAccess(target, index), t)
    | T_Array(_,_), _ -> raise_failure "Indexing expression must be of type 'int'"
    | _, _ -> raise_failure "Indexing must be done on an array"
  )
  | Binary_op(op,left,right) -> 
    let left = type_expression left state in
    let right = type_expression right state in
    let result_type = match op, get_type left, get_type right with
    | "+", T_Int, T_Int 
    | "-", T_Int, T_Int 
    | "*", T_Int, T_Int 
    | "&", T_Int, T_Int
    | "|", T_Int, T_Int 
    | "=", T_Int, T_Int
    | "=", T_Dir, T_Dir
    | "=", T_Resource, T_Resource
    | "=", T_Field, T_Field
    | "=", T_Func _, T_Null
    | "=", T_Null, T_Func _
    | "!=", T_Int, T_Int
    | "!=", T_Dir, T_Dir
    | "!=", T_Resource, T_Resource
    | "!=", T_Field, T_Field
    | "!=", T_Func _, T_Null
    | "!=", T_Null, T_Func _
    | "<", T_Int, T_Int 
    | ">", T_Int, T_Int 
    | "<=", T_Int, T_Int
    | ">=", T_Int, T_Int 
    | "/", T_Int, T_Int
    | "%", T_Int, T_Int
    | "any", T_Field, T_Field
    | "is", T_Field, T_Field -> T_Int
    | "<<", T_Dir, T_Int 
    | ">>", T_Dir, T_Int -> T_Dir
    | "+", T_Field, T_Field -> T_Field
    | "-", T_Field, T_Field -> T_Field
    | _,t0,t1 -> raise_failure ("Unknown binary operation: "^type_string t0^" "^op^" "^type_string t1)
    in
    (Binary_op(op,left,right),result_type)
  | Unary_op(op, e) -> 
    let e = type_expression e state in
    let result_type = match op, get_type e with
    | "!", T_Int -> T_Int
    | _, t -> raise_failure ("Unknown unary operation: " ^ op ^ type_string t)
    in
    (Unary_op(op, e), result_type)
  | Random -> (Random, T_Int)
  | Int i -> (Int i, T_Int)
  | Prop p -> (Prop p, T_Field)
  | Resource r -> (Resource r, T_Resource)
  | Decrement(target, post) -> 
    if not(assignable target) then raise_failure ("Cannot decrement expression") else
    let target = type_expression target state in
    let target_type = get_type target in
    (match target_type with 
    | T_Int -> (Decrement(target, post), T_Int)
    | _ -> raise_failure ("Only int can be decremented")
  )
  | Increment(target, post) -> 
    if not(assignable target) then raise_failure ("Cannot increment expression") else
    let target = type_expression target state in
    let target_type = get_type target in
    (match target_type with 
    | T_Int -> (Increment(target, post), T_Int)
    | _ -> raise_failure ("Only int can be incremented")
  )
  | Direction d -> (Direction d, T_Dir)
  | RandomSet exprs -> (
    let exprs = List.map (fun e -> type_expression e state) exprs in
    match exprs with 
    | [] -> raise_failure "Empty random set"
    | h::t -> 
      let h_type = get_type h in
      if not(t |> List.map get_type |> List.for_all (type_eq h_type)) then raise_failure "Random set of differing types"
      else (RandomSet exprs, h_type)
    )
  | Func(ret,args,body) -> (
    let typ = T_Func(ret, List.map fst args) in
    let func_scope = { 
      local = List.map (fun (t,n) -> Var(t,n)) args @ [Var(typ, "this")] ; 
      global = if state.scopes.global = None then Some(state.scopes.local) else state.scopes.global
    } in
    let (body,_) = type_statement body ({scopes = func_scope; ret_type = Some ret; continue = None; break = None; labels = StringSet.empty})in
    (Func(ret, args, body), typ)
  )
  | Call(Expr(VarAccess name, ln), args) when not (is_var name state.scopes) -> (
    let args = List.map (fun a -> type_expression a state) args in
    let arg_types = List.map get_type args in
    match lookup_builtin_info name with
    | Some builtin -> (match builtin.info with
    | BuiltinVar _ -> raise_failure ("Unknown function: "^name^"("^String.concat "," (List.map type_string arg_types) ^")")
    | BuiltinFunc bf -> (
        if 
          (bf.canonical :: (List.map fst bf.short_hands))
          |> List.exists (type_list_eq arg_types) 
        then (Call(Expr(VarAccess name, (ln, bf.ret)), args), bf.ret)
        else raise_failure ("No version of builtin function '"^name^"' takes: "^ String.concat "," (List.map type_string arg_types))
      )
    )
    | _ -> raise_failure ("Unknown function: "^name^"("^String.concat "," (List.map type_string arg_types) ^")")
  )
  | Call(f,args) -> (
    let f = type_expression f state in
    let f_type = get_type f in
    let args = List.map (fun a -> type_expression a state) args in
    let arg_types = List.map get_type args in
    match f_type with
    | T_Func(ret, params) -> (
      let raise_arg_type_failure = fun () -> raise_failure ("Expected arguments of types: ("^ String.concat "," (List.map type_string params) ^"), but got: (" ^ String.concat "," (List.map type_string arg_types) ^ ")") in
      if List.length params != List.length args then raise_arg_type_failure ()
      else if not (
        List.combine params arg_types
        |> List.for_all (fun (p, a) -> type_eq p a)
      )
      then raise_arg_type_failure ()
      else (Call(f,args), ret)
    )
    | _ -> raise_failure ("Not a callable type: '" ^ type_string f_type ^ "'")
  )
  | Ternary(c,a,b) -> 
    let c = type_expression c state in
    let a = type_expression a state in
    let b = type_expression b state in
    if not(type_eq T_Int (get_type c)) then raise_failure "Condition must be of type 'int'" else
    if not(type_eq (get_type a) (get_type b)) then raise_failure "Ternary of differing types" else
    (Ternary(c,a,b), get_type a)
  | Null -> (Null, T_Null)

and type_expression (Expr(e,ln)) state : (int * typ) expression = 
  try
    let (e,t) = type_expr e state in
    Expr(e, (ln, t))
  with
  | Failure(p, None, msg) -> raise (Failure(p, Some ln, msg))
  | e -> raise e

and can_assign target_type value_type = match target_type, value_type with
      | T_Func _, T_Null
      | T_Null, T_Func _ -> true
      | t1, t2 -> type_eq t1 t2

and type_check_stmt_inner stmt state : ((int * typ) stmt * compile_state) = match stmt with
  | If(c,a,b) -> 
    let c = type_expression c state in
    let (a,_) = type_statement a state in
    let (b,_) = type_statement b state in
    if not(type_eq T_Int (get_type c)) then raise_failure "Condition must be of type 'int'" else
    (If(c,a,b), state)
  | IfIs(c,alts,else_opt) -> 
    let c = type_expression c state in
    let c_type = get_type c in
    let (alt_expressions, alt_statements) = List.split alts in
    let alt_expressions = List.map (fun e -> type_expression e state) alt_expressions in
    let alt_statements = List.map (fun s -> type_statement s state |> fst) alt_statements in
    let else_statement = Option.map (fun s -> type_statement s state |> fst) else_opt in
    let match_failures = alt_expressions |> List.filter (fun e -> not(type_eq c_type (get_type e))) in (
    match match_failures with
      | [] -> (IfIs(c, List.combine alt_expressions alt_statements, else_statement), state)
      | h::_ -> raise(Failure(None, Some(get_line h), "A case did not match the condition type, expected '"^type_string c_type^"' but got '"^type_string(get_type h)^"'"))
    )
  | Block stmts -> (
    let (stmts', state') = type_statements stmts state in
    (Block stmts', state')
  )
  | While(c,s,iter_opt) -> 
    let c = type_expression c state in
    let (s,_) = type_statement s state in
    let iter_opt = Option.map (fun iter -> type_statement iter state |> fst) iter_opt in
    if not(type_eq T_Int (get_type c)) then raise_failure "The condition of a while loop must be of type 'int'" else
    (While(c,s,iter_opt), state)
  | Assign(target,e) -> (* Needs a seperate impl. for arrays ??? *)
    let target = type_expression target state in
    let target_type = get_type target in
    let e = type_expression e state in
    let e_type = get_type e in
    if can_assign target_type e_type 
    then (Assign(target, e), state)
    else raise_failure ("Cannot assign a value of type '" ^ type_string e_type ^ "' to a target of type '" ^ type_string target_type ^ "'")
  | DeclareAssign(Some typ, name, e) ->
    let e = type_expression e state in
    let e_type = get_type e in
    if can_assign typ e_type 
    then (DeclareAssign(Some typ, name, e), {state with scopes = { local = Var(typ,name)::state.scopes.local; global = state.scopes.global } })
    else raise_failure ("Cannot assign a value of type '" ^ type_string e_type ^ "' to a variable of type '" ^ type_string typ ^ "'")
  | DeclareAssign(None, name, e) -> (
    let e = type_expression e state in
    let e_type = get_type e in
    match e_type with
    | T_Null -> raise_failure "Cannot infere a type from null"
    | typ -> (DeclareAssign(Some typ, name, e), {state with scopes = { local = Var(typ,name)::state.scopes.local; global = state.scopes.global } })
  )
  | Declare(t,n) -> (Declare(t,n), {state with scopes = { local = Var(t,n)::state.scopes.local; global = state.scopes.global } })
  | GoTo l -> (GoTo l, state)
  | Label l -> (Label l, state)
  | Continue -> (Continue, state)
  | Break -> (Break, state)
  | Return e -> ( match state.ret_type with
      | None -> raise_failure "Return statement outside function"
      | Some ret_type -> 
        let e = type_expression e state in
        let e_type = get_type e in
        if not(type_eq ret_type e_type) then raise_failure ("Return type mismatch: expected '" ^type_string ret_type^ "', but got '" ^type_string e_type^ "'")
        else (Return e, state)
  )
  | ExprStmt e -> (ExprStmt(type_expression e state), state)

and type_statement (Stmt(stmt,ln)) state = 
  try 
    let (stmt', state') = type_check_stmt_inner stmt state in
    (Stmt(stmt', (ln, T_Null)), state')
  with 
  | Failure(p, None, msg) -> raise (Failure(p, Some ln, msg))
  | e -> raise e

and type_statements stmts state  =
  let (stmts, state) = List.fold_left (fun (stmts, state) stmt -> 
    let (stmt', state') = type_statement stmt state in 
    (stmt'::stmts, state')
  ) ([], state) stmts
  in
  (List.rev stmts, state)

let type_program (File(prog,i)) =
  let (prog',_) = type_statements prog {scopes = { local = []; global = None}; labels = available_labels (Stmt(Block prog,i)); break = None; continue = None; ret_type = None;} 
  in File(prog', (i,T_Null))