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


(* TODO: 
- Entirely remove Instr_Access and Instr_Assign 
- Cleanup
- More helper functions, especially for using 'find_expression_location'
*)

type location =
  | ComputeStack
  | StorageStack of instruction list

let expr_size (Expr(_, (_, t))) = type_size t

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
  | VarAccess _ -> ( match find_expr_location e state with
    | StorageStack instr -> instr @ Instr_Load :: I(expr_size e) :: acc
    | ComputeStack -> raise_failure ""
  ) 
  | ArrayAccess(target, index) -> (
    match get_type target, find_expr_location target state with
    | T_Array(elem_t, size), ComputeStack -> compile_expr target state (compile_expr index state (Instr_Extract :: I(size * type_size elem_t) :: I(type_size elem_t) :: acc))
    | T_Array(elem_t, size), StorageStack(loc_instrs) -> loc_instrs @ (compile_expr index state (Instr_Index :: I(size) :: I(type_size elem_t) :: Instr_Load :: I(type_size elem_t) :: acc))
    | _ -> raise_failure "Dont know how to access array"
  )
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
  | Unary_op (op, e) -> ( match op, get_type e with 
      | "!", T_Int -> compile_expr e state (Instr_Not :: acc)
      | "!", T_Field -> Instr_Place :: I(prop_index All_Prop) :: compile_expr e state (Instr_BinNot :: Instr_BinAnd :: acc)
      | _ -> raise_failure "Unknown unary operation"
  )
  (* true = pre *)
  | Increment(target, true) -> (match find_expr_location target state with
    | StorageStack location -> location @ Instr_Copy :: Instr_Copy :: Instr_Load :: I(expr_size target) :: Instr_Place :: I(1) :: Instr_Add :: Instr_Store :: I(expr_size target) :: Instr_Load :: I(expr_size target) :: acc
    | ComputeStack -> raise_failure ""
  )
  | Decrement(target, true)  -> (match find_expr_location target state with
    | StorageStack location -> location @ Instr_Copy :: Instr_Copy :: Instr_Load :: I(expr_size target) :: Instr_Place :: I(1) :: Instr_Sub :: Instr_Store :: I(expr_size target) :: Instr_Load :: I(expr_size target) :: acc
    | ComputeStack -> raise_failure ""
  )
  | Increment(target, false) -> (match find_expr_location target state with
    | StorageStack location -> location @ Instr_Copy :: Instr_Load :: I(expr_size target) :: Instr_Swap :: Instr_Copy :: Instr_Load :: I(expr_size target) :: Instr_Place :: I(1) :: Instr_Add :: Instr_Swap :: Instr_Store :: I(expr_size target) :: acc 
    | ComputeStack -> raise_failure ""
  )
  | Decrement(target, false) -> (match find_expr_location target state with
    | StorageStack location -> location @ Instr_Copy :: Instr_Load :: I(expr_size target) :: Instr_Swap :: Instr_Copy :: Instr_Load :: I(expr_size target) :: Instr_Place :: I(1) :: Instr_Sub :: Instr_Swap :: Instr_Store  :: I(expr_size target) :: acc 
    | ComputeStack -> raise_failure ""
  )
  | Func(ret,args,body) -> (
    let func_label = Helpers.new_label "func" in
    let end_label = Helpers.new_label "func_end" in
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
    Instr_GoTo :: LabelRef(end_label) :: Label(func_label) :: Instr_Declare :: I(size_of_vars vars) :: compile_stmt body {new_state with scopes = ({ local = new_state.scopes.local @ vars; global = new_state.scopes.global })} (Instr_Declare :: I(type_size ret) :: Instr_Return :: Label(end_label) :: Instr_Place :: LabelRef(func_label) :: acc)
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
  | Call(f,args) -> ( match get_type f with
    | T_Func(_, params) ->
      let total_params_size = params |> List.map type_size |> List.fold_left (+) 0 in
      let comped_args = args |> List.combine params |> List.map (fun (param, arg) -> fix_size arg param state []) |> List.flatten in
      comped_args @ (compile_expr f state (Instr_Place :: I(total_params_size) :: Instr_Call :: acc))
    | _ -> raise_failure "Calling non-function"
  )
  | Ternary(c,a,b) -> (
    let label_true = Helpers.new_label "ternary_true" in
    let label_stop = Helpers.new_label "ternary_stop" in
    compile_expr c state (Instr_GoToIf :: LabelRef(label_true) :: (compile_expr b state (Instr_GoTo :: LabelRef(label_stop) :: Label(label_true) :: (compile_expr a state (Label(label_stop) :: acc)))))
  )
  | Null -> Instr_Place :: I(0) :: acc
  | ArrayLiteral exprs -> 
    exprs |> List.rev |> List.fold_left (fun acc v -> compile_expr v state acc) acc
  with
  | Failure(p,None,msg) -> raise (Failure(p,Some ln, msg))
  | a -> raise a

and fix_size expr typ state acc = 
  let size_diff = type_size typ - expr_size expr in
  match size_diff with
  | 0 -> compile_expr expr state acc
  | x when x < 0 ->  compile_expr expr state (Instr_MoveSP :: I(x) :: acc) (*arg is too large*)
  | x -> compile_expr expr state (Instr_Declare :: I(x) :: acc) (*arg is too small*)

and find_expr_location e state = match e with
  | Expr(e,_) -> find_expression_location e state

and find_expression_location e state = match e with
  | VarAccess name -> find_variable_location name state.scopes
  | ArrayAccess (target, index) -> (match get_type target, find_expr_location target state with
    | _, ComputeStack -> ComputeStack
    | T_Array(elem_t, array_size), StorageStack instrs -> StorageStack(instrs @ compile_expr index state ([Instr_Index ; I(array_size) ; I(type_size elem_t) ]))
    | _ -> raise_failure ""
  )
  | _ -> ComputeStack

and find_variable_location name scopes =
  let rec aux vars i = match vars with
    | [] -> None
    | Var(ty,n)::t ->
      if n = name then Some i else aux t (i+type_size ty)
  in
  match aux scopes.local 0 with
  | Some i -> (StorageStack [Instr_BP ; Instr_Place ; I(i) ; Instr_Add])
  | None -> (
    match scopes.global |> Option.map (fun scope -> aux scope 0) |> Option.join with
    | Some i -> (StorageStack [Instr_Place ; I(i)])
    | None -> raise_failure ("No such variable: "^name)
  )


and compile_assignment target expr state acc = match find_expr_location target state with
  | ComputeStack -> raise_failure "";
  | StorageStack instr -> fix_size expr (get_type target) state (instr @ Instr_Store :: I(expr_size target) :: acc)

and compile_stmt (Stmt(stmt,(ln,_))) state acc =
  try match stmt with
  | If (expr, s1, s2) -> (
    let label_true = Helpers.new_label "if_true" in
    let label_stop = Helpers.new_label "if_stop" in
    compile_expr expr state (Instr_GoToIf :: LabelRef(label_true) :: (compile_stmt s2 state (Instr_GoTo :: LabelRef(label_stop) :: Label(label_true) :: (compile_stmt s1 state (Label(label_stop) :: acc)))))
  )
  | IfIs(v,alts,opt) -> (
    let label_end = Helpers.new_label "ifis_end" in
    let rec comp_alts alts next acc = match alts with
      | [] -> acc
      | (alt_v,alt_s)::t -> Instr_Copy :: compile_expr alt_v state (Instr_Eq :: Instr_Not :: Instr_GoToIf :: LabelRef(next) :: compile_stmt alt_s state (Instr_GoTo :: LabelRef(label_end) :: Label(next) :: (comp_alts t (Helpers.new_label "ifis_alt") acc)))
    in
    match opt with
    | None -> compile_expr v state (comp_alts alts (Helpers.new_label "ifis_alt") (Label(label_end) :: acc))
    | Some other -> compile_expr v state (comp_alts alts (Helpers.new_label "ifis_alt") (Instr_MoveSP :: I(-1) :: compile_stmt other state (Label(label_end) :: acc)))
  )
  | Block (stmt_list) -> (
    List.fold_left (fun acc stmt -> compile_stmt stmt state acc) acc (List.rev stmt_list) 
  )
  | While(v,s,None) -> 
    let label_cond = Helpers.new_label "while_cond" in
    let label_start = Helpers.new_label "while_start" in
    let label_stop = Helpers.new_label "while_stop" in
    let state' = {state with break = Some label_stop; continue = Some label_cond } in
    Instr_GoTo :: LabelRef(label_cond) :: Label(label_start) :: (compile_stmt s state' (Label(label_cond) :: compile_expr v state (Instr_GoToIf :: LabelRef(label_start) :: Label(label_stop)::acc)))
  | While(v,s,Some si) -> 
    let label_cond = Helpers.new_label "while_iter_cond" in
    let label_start = Helpers.new_label "while_iter_start" in
    let label_stop = Helpers.new_label "while_iter_stop" in
    let label_iter = Helpers.new_label "while_iter_iter" in
    let state' = {state with break = Some label_stop; continue = Some label_iter } in
    Instr_GoTo :: LabelRef(label_cond) :: Label(label_start) :: (compile_stmt s state' (Label(label_iter) :: (compile_stmt si state (Label(label_cond) :: (compile_expr v state (Instr_GoToIf :: LabelRef(label_start) :: Label(label_stop) :: acc))))))
  | Continue -> (match state.continue with
    | Some label -> Instr_GoTo :: LabelRef(label) :: acc
    | None -> raise_failure "Nothing to continue"
  )
  | Break -> (match state.break with
    | Some label -> Instr_GoTo :: LabelRef(label) :: acc
    | None -> raise_failure "Nothing to break out of"
  )
  | Assign (target, expr) -> compile_assignment target expr state acc
  | DeclareAssign (_, name, expr) -> compile_assignment (Expr(VarAccess name, (ln, get_type expr))) expr state acc
  | Label name -> Label name :: acc
  | GoTo n -> 
    if StringSet.mem n state.labels
    then Instr_GoTo :: LabelRef n :: acc
    else raise_failure ("Unavailable label: "^n)
  | Declare _ -> acc
  | Return v -> (match state.ret_type with 
    | Some t -> fix_size v t state (Instr_Return :: I(type_size t) :: acc)
    | None -> raise_failure ""
  )
  | ExprStmt e -> compile_expr e state (Instr_MoveSP :: I(-(expr_size e)) :: acc)
  with 
  | Failure(p,None,msg) -> raise (Failure(p,Some ln, msg))
  | a -> raise a

let compile_player (File(program,i)) =
  let program = Stmt(Block program, i) in
  let vars = extract_declarations program |> List.rev in
  let labels = available_labels program in
  let state = {scopes = { local = vars ; global = None }; labels = labels; break = None; continue = None; ret_type = None;} in
  Instr_Declare :: I(size_of_vars vars) :: compile_stmt program state []
