open Absyn
open ProgramRep
open Exceptions
open Typing
open Field_props
open Transform
open Builtins
open Resources
open Helpers

(*** Compiling functions ***)

let target_scope (Expr(target,_)) scopes : scope = 
  let rec find_name t = match t with
    | VarAccess n -> n
    | ArrayAccess(Expr(t,_),_) -> find_name t
    | _ -> raise_failure "No containing scope"
  in
  let name = find_name target in
  let rec aux vars = match vars with
    | [] -> false
    | Var(_,n)::t ->
      if n = name then true else aux t
  in
  match aux scopes.local with
  | true -> LocalScope
  | false -> (
    match Option.map (fun scope -> aux scope) scopes.global with
    | Some true -> GlobalScope
    | _ -> raise_failure ("No such variable: "^name)
  )

let instr_access target scopes = match target_scope target scopes with
  | LocalScope -> Instr_Access
  | GlobalScope -> Instr_GlobalAccess

let instr_assign target scopes = match target_scope target scopes with
  | LocalScope -> Instr_Assign
  | GlobalScope -> Instr_GlobalAssign


let fetch_var_index name scopes = 
  let rec aux vars i = match vars with
    | [] -> None
    | Var(ty,n)::t ->
      if n = name then Some i else aux t (i+type_size ty)
  in
  match aux scopes.local 0 with
  | Some i -> i
  | None -> (
    match Option.map (fun scope -> aux scope 0) scopes.global |> Option.join with
    | Some i -> i
    | None -> raise_failure ("No such register: "^name)
  )

let size_of_vars (vars : variable list) =
  List.fold_left (fun acc (Var(t,_)) -> acc + type_size t) 0 vars

let rec compile_expr (Expr(expr, (ln, _)) as e) (state:compile_state) acc =
  try match expr with
  | VarAccess name when not(is_var name state.scopes) -> (
    match lookup_builtin_info name with
    | Some builtin -> (match builtin.info with
      | BuiltinVar bv ->  bv.comp @ acc
      | BuiltinFunc bf -> Instr_Place :: I(bf.addr) :: acc
    )
    | _ -> raise_failure ("Unknown variable: "^name)
  )
  | VarAccess _ -> compile_addr e state (instr_access e state.scopes :: acc)
  | ArrayAccess _ -> compile_addr e state (instr_access e state.scopes :: acc)
  | Int i -> Instr_Place :: I(i) :: acc
  | Prop fp -> Instr_Place :: I(prop_index fp) :: acc
  | Resource r -> Instr_Place :: I(resource_value r) :: acc
  | Random -> Instr_Random :: acc
  | RandomSet vals -> 
    List.fold_left (fun acc v -> compile_expr v state acc) (Instr_RandomSet :: I(List.length vals) :: acc) vals
  | Direction d -> Instr_Place :: I(int_of_dir d) :: acc
  | Binary_op (op, e1, e2) -> (match op, get_type e1, get_type e2 with
    | "+", T_Int, T_Int -> compile_expr e1 state (compile_expr e2 state (Instr_Add :: acc))
    | "-", T_Int, T_Int -> compile_expr e1 state (compile_expr e2 state (Instr_Sub :: acc))
    | "*", T_Int, T_Int -> compile_expr e1 state (compile_expr e2 state (Instr_Mul :: acc))
    | "&", T_Int, T_Int -> compile_expr e1 state (compile_expr e2 state (Instr_And :: acc))
    | "|", T_Int, T_Int -> compile_expr e1 state (compile_expr e2 state (Instr_Or :: acc))
    | "=", T_Int, T_Int 
    | "=", T_Dir, T_Dir 
    | "=", T_Resource, T_Resource
    | "=", T_Field, T_Field
    | "=", T_Null, T_Func _
    | "=", T_Func _, T_Null -> compile_expr e1 state (compile_expr e2 state (Instr_Eq :: acc))
    | "!=", T_Int, T_Int 
    | "!=", T_Dir, T_Dir 
    | "!=", T_Resource, T_Resource 
    | "!=", T_Field, T_Field
    | "!=", T_Null, T_Func _
    | "!=", T_Func _, T_Null -> compile_expr e1 state (compile_expr e2 state (Instr_Eq :: Instr_Not :: acc))
    | "<", T_Int, T_Int -> compile_expr e2 state (compile_expr e1 state (Instr_Lt :: acc))
    | ">", T_Int, T_Int -> compile_expr e1 state (compile_expr e2 state (Instr_Lt :: acc))
    | "<=", T_Int, T_Int -> compile_expr e1 state (compile_expr e2 state (Instr_Lt :: Instr_Not :: acc))
    | ">=", T_Int, T_Int -> compile_expr e2 state (compile_expr e1 state (Instr_Lt :: Instr_Not :: acc))
    | "/", T_Int, T_Int -> compile_expr e2 state (compile_expr e1 state (Instr_Div :: acc))
    | "%", T_Int, T_Int -> compile_expr e2 state (compile_expr e1 state (Instr_Mod :: acc))
    | ">>", T_Dir, T_Int -> Instr_Place :: I(4) :: (compile_expr e1 state (compile_expr e2 state (Instr_Add :: Instr_Mod :: acc)))
    | "<<", T_Dir, T_Int -> Instr_Place :: I(4) :: (compile_expr e1 state (compile_expr e2 state (Instr_Sub :: Instr_Mod :: acc)))
    | "+", T_Field, T_Field -> compile_expr e1 state (compile_expr e2 state (Instr_BinOr :: acc))
    | "-", T_Field, T_Field -> compile_expr e1 state (compile_expr e2 state (Instr_BinNot :: Instr_BinAnd :: acc))
    | "is", T_Field, T_Field -> compile_expr e2 state (Instr_Copy :: compile_expr e1 state (Instr_BinAnd :: Instr_Eq :: acc))
    | "any", T_Field, T_Field -> compile_expr e1 state (compile_expr e2 state (Instr_BinAnd :: acc))
    | _ -> raise_failure "Unknown binary operation"
  )
  | Unary_op (op, e) -> ( match op with 
      | "!" -> compile_expr e state (Instr_Not :: acc)
      | _ -> raise_failure "Unknown unary operation"
  )
  (* true = pre *)
  | Increment(target, true)  -> compile_addr target state (Instr_Copy :: Instr_Copy :: instr_access target state.scopes :: Instr_Place :: I(1) :: Instr_Add :: instr_assign target state.scopes :: instr_access target state.scopes :: acc)
  | Increment(target, false) -> compile_addr target state (Instr_Copy :: instr_access target state.scopes :: Instr_Swap :: Instr_Copy :: instr_access target state.scopes :: Instr_Place :: I(1) :: Instr_Add :: instr_assign target state.scopes :: acc)
  | Decrement(target, true)  -> compile_addr target state (Instr_Copy :: Instr_Copy :: instr_access target state.scopes :: Instr_Place :: I(1) :: Instr_Sub :: instr_assign target state.scopes :: instr_access target state.scopes :: acc)
  | Decrement(target, false) -> compile_addr target state (Instr_Copy :: instr_access target state.scopes :: Instr_Swap :: Instr_Copy :: instr_access target state.scopes :: Instr_Place :: I(1) :: Instr_Sub :: instr_assign target state.scopes :: acc)
  | Func(ret,args,body) -> (
    let func_label = Helpers.new_label () in
    let end_label = Helpers.new_label () in
    let func_scope = { 
      local = (Var(T_Func(ret, List.map fst args), "this") :: List.map (fun (t,n) -> Var(t,n)) args) ; 
      global =  if state.scopes.global = None then Some(state.scopes.local) else state.scopes.global ;
    } in
    let new_state = {
      scopes = func_scope;  
      labels = available_labels body; 
      break = None; 
      continue = None; 
      ret_type = Some ret;
    } in
    let vars = extract_declarations body |> List.rev in
    Instr_GoTo :: LabelRef(end_label) :: Label(func_label) :: Instr_Declare :: I(size_of_vars vars) :: compile_stmt body {new_state with scopes = ({ local = new_state.scopes.local @ vars; global = new_state.scopes.global })} (Instr_Place :: I(0) :: Instr_Return :: Label(end_label) :: Instr_Place :: LabelRef(func_label) :: acc)
  )
  | Call(Expr(VarAccess name,_), args) when not(is_var name state.scopes) -> (
    let arg_types = List.map get_type args in  
    match lookup_builtin_info name with
    | Some builtin -> (match builtin.info with
      | BuiltinVar _ -> raise_failure ("Unknown function: "^name)
      | BuiltinFunc bf -> (
        if (type_list_eq arg_types bf.canonical)
        then 
          let comped_args = List.map (fun arg -> compile_expr arg state []) args |> List.flatten in
          comped_args @ ((Instr_Place :: I(bf.addr) :: Instr_Place :: I(List.length args) :: Instr_Call :: acc))
        else match List.find_opt (fun (params, _) -> type_list_eq arg_types params) bf.short_hands with
        | Some(_, comp) -> 
          let comped_args = List.map (fun arg -> compile_expr arg state []) args |> List.flatten in
          (comp comped_args) @ acc
        | None -> raise_failure ("Unknown function: "^name)
      )
    )
    | _ -> raise_failure ("Unknown function: "^name)
  )
  | Call(f,args) -> (
    let comped_args = List.map (fun arg -> compile_expr arg state []) args |> List.flatten in
    comped_args @ (compile_expr f state (Instr_Place :: I(List.length args) :: Instr_Call :: acc))
  )
  | Ternary(c,a,b) -> (
    let label_true = Helpers.new_label () in
    let label_stop = Helpers.new_label () in
    compile_expr c state (Instr_GoToIf :: LabelRef(label_true) :: (compile_expr b state (Instr_GoTo :: LabelRef(label_stop) :: Label(label_true) :: (compile_expr a state (Label(label_stop) :: acc)))))
  )
  | Null -> Instr_Place :: I(0) :: acc
  with
  | Failure(p,None,msg) -> raise (Failure(p,Some ln, msg))
  | a -> raise a


and compile_var_addr name state acc =
  Instr_Place :: I(fetch_var_index name state.scopes) :: acc

and compile_addr (Expr(expr, _)) state acc = match expr with
  | VarAccess name -> Instr_Place :: I(fetch_var_index name state.scopes) :: acc
  | ArrayAccess(t,i) -> (
    match get_type t with
    | T_Array(typ,array_size) ->
      compile_addr t state (compile_expr i state (Instr_Index :: I(array_size) :: I(type_size typ)  :: acc))
    | _ -> raise_failure "Not an array"
  )
  | _ -> raise_failure "Expression does not have a stack address"


and compile_assignment target expr state acc =
    (*let expr_type = get_type expr in
    match target_type, expr_type, expr with
    | T_Array(_,st), T_Array(_,se), Expr(Reference(t),_) -> (
      let num = min st se in
      List.init num (fun i -> i)
      |> List.fold_left (fun acc i -> compile_assignment (Target(Array(target, Int(i)), )) (Reference(Array(e,Int(i)))) state acc) acc
    )
    | _,_,_ -> compile_target_index target state (compile_expr expr state (instr_assign target state.scopes :: acc))*)
    compile_addr target state (compile_expr expr state (instr_assign target state.scopes :: acc))

and compile_stmt (Stmt(stmt,(ln,_))) state acc =
  try match stmt with
  | If (expr, s1, s2) -> (
    let label_true = Helpers.new_label () in
    let label_stop = Helpers.new_label () in
    compile_expr expr state (Instr_GoToIf :: LabelRef(label_true) :: (compile_stmt s2 state (Instr_GoTo :: LabelRef(label_stop) :: Label(label_true) :: (compile_stmt s1 state (Label(label_stop) :: acc)))))
  )
  | IfIs(v,alts,opt) -> (
    let label_end = Helpers.new_label () in
    let rec comp_alts alts next acc = match alts with
      | [] -> acc
      | (alt_v,alt_s)::t -> Instr_Copy :: compile_expr alt_v state (Instr_Eq :: Instr_Not :: Instr_GoToIf :: LabelRef(next) :: compile_stmt alt_s state (Instr_GoTo :: LabelRef(label_end) :: Label(next) :: (comp_alts t (Helpers.new_label ()) acc)))
    in
    match opt with
    | None -> compile_expr v state (comp_alts alts (Helpers.new_label ()) (Label(label_end) :: acc))
    | Some other -> compile_expr v state (comp_alts alts (Helpers.new_label ()) (Instr_DecStack :: compile_stmt other state (Label(label_end) :: acc)))
  )
  | Block (stmt_list) -> (
    List.fold_left (fun acc stmt -> compile_stmt stmt state acc) acc (List.rev stmt_list) 
  )
  | While(v,s,None) -> 
    let label_cond = Helpers.new_label () in
    let label_start = Helpers.new_label () in
    let label_stop = Helpers.new_label () in
    let state' = {state with break = Some label_stop; continue = Some label_cond } in
    Instr_GoTo :: LabelRef(label_cond) :: Label(label_start) :: (compile_stmt s state' (Label(label_cond) :: compile_expr v state (Instr_GoToIf :: LabelRef(label_start) :: Label(label_stop)::acc)))
  | While(v,s,Some si) -> 
    let label_cond = Helpers.new_label () in
    let label_start = Helpers.new_label () in
    let label_stop = Helpers.new_label () in
    let label_incr = Helpers.new_label () in
    let state' = {state with break = Some label_stop; continue = Some label_incr } in
    Instr_GoTo :: LabelRef(label_cond) :: Label(label_start) :: (compile_stmt s state' (Label(label_incr) :: (compile_stmt si state (Label(label_cond) :: (compile_expr v state (Instr_GoToIf :: LabelRef(label_start) :: Label(label_stop) :: acc))))))
  | Continue -> (match state.continue with
    | Some label -> Instr_GoTo :: LabelRef(label) :: acc
    | None -> raise_failure "Nothing to continue"
  )
  | Break -> (match state.break with
    | Some label -> Instr_GoTo :: LabelRef(label) :: acc
    | None -> raise_failure "Nothing to break out of"
  )
  | Assign (target, expr) -> compile_assignment target expr state acc
  | DeclareAssign (_, name, expr) -> compile_assignment (Expr(VarAccess name, (ln, T_Null))) expr state acc
  | Label name -> Label name :: acc
  | GoTo n -> 
    if StringSet.mem n state.labels
    then Instr_GoTo :: LabelRef n :: acc
    else raise_failure ("Unavailable label: "^n)
  | Declare _ -> acc
  | Return v -> compile_expr v state (Instr_Return :: acc)
  | ExprStmt e -> compile_expr e state (Instr_DecStack :: acc)
  with 
  | Failure(p,None,msg) -> raise (Failure(p,Some ln, msg))
  | a -> raise a

let compile_player (File(program,i)) =
  let program = Stmt(Block program, i) in
  let vars = extract_declarations program |> List.rev in
  let labels = available_labels program in
  let state = {scopes = { local = vars ; global = None }; labels = labels; break = None; continue = None; ret_type = None;} in
  Instr_Declare :: I(size_of_vars vars) :: compile_stmt program state []

