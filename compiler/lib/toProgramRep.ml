open Absyn
open ProgramRep
open Exceptions
open Typing
open Field_props
open Transform

(*** Compiling functions ***)

module StringSet = Set.Make(String)

let target_scope target scopes : scope = 
  let rec find_name target = match target with
    | Local n -> n
    | Array(t, _) -> find_name t
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
    | _ -> raise_failure ("No such register: "^name)
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

let rec compile_value val_expr (state:compile_state) acc =
  match val_expr with
  | Reference target -> compile_target_index target state (instr_access target state.scopes :: acc)
  | MetaReference md -> ( match md with
    | PlayerX -> Meta_PlayerX :: acc
    | PlayerY -> Meta_PlayerY :: acc
    | BoardX -> Meta_BoardX :: acc
    | BoardY -> Meta_BoardY :: acc
    | PlayerID -> Meta_PlayerID :: acc
    | PlayerResource n -> (match List.find_index (fun name -> n = name) Flags.compile_flags.resources with
      | Some i -> Meta_Resource :: I(i) :: acc
      | None -> raise_failure ("Unknown resource lookup: "^n)
    )
  )
  | Int i -> Instr_Place :: I(i) :: acc
  | Random -> Instr_Random :: acc
  | RandomSet vals -> 
    List.fold_left (fun acc v -> compile_value v state acc) (Instr_RandomSet :: I(List.length vals) :: acc) vals
  | Direction d -> Instr_Place :: I(int_of_dir d) :: acc
  | Look(d,f) -> compile_value d state (Instr_Look :: I(prop_index f) :: acc)
  | Scan(d,p) -> compile_value d state (compile_value p state (Instr_Scan :: acc))
  | Binary_op (op, e1, e2) -> ( match op, type_value state e1, type_value state e2 with
    | "+", T_Int, T_Int -> compile_value e1 state (compile_value e2 state (Instr_Add :: acc))
    | "-", T_Int, T_Int -> compile_value e1 state (compile_value e2 state (Instr_Sub :: acc))
    | "*", T_Int, T_Int -> compile_value e1 state (compile_value e2 state (Instr_Mul :: acc))
    | "&", T_Int, T_Int -> compile_value e1 state (compile_value e2 state (Instr_And :: acc))
    | "|", T_Int, T_Int -> compile_value e1 state (compile_value e2 state (Instr_Or :: acc))
    | "=", T_Int, T_Int -> compile_value e1 state (compile_value e2 state (Instr_Eq :: acc))
    | "=", T_Dir, T_Dir -> compile_value e1 state (compile_value e2 state (Instr_Eq :: acc))
    | "!=", T_Int, T_Int -> compile_value e1 state (compile_value e2 state (Instr_Eq :: Instr_Not :: acc))
    | "!=", T_Dir, T_Dir -> compile_value e1 state (compile_value e2 state (Instr_Eq :: Instr_Not :: acc))
    | "<", T_Int, T_Int -> compile_value e2 state (compile_value e1 state (Instr_Lt :: acc))
    | ">", T_Int, T_Int -> compile_value e1 state (compile_value e2 state (Instr_Lt :: acc))
    | "<=", T_Int, T_Int -> compile_value e1 state (compile_value e2 state (Instr_Lt :: Instr_Not :: acc))
    | ">=", T_Int, T_Int -> compile_value e2 state (compile_value e1 state (Instr_Lt :: Instr_Not :: acc))
    | "/", T_Int, T_Int -> compile_value e2 state (compile_value e1 state (Instr_Div :: acc))
    | "%", T_Int, T_Int -> compile_value e2 state (compile_value e1 state (Instr_Mod :: acc))
    | ">>", T_Dir, T_Int -> Instr_Place :: I(4) :: (compile_value e1 state (compile_value e2 state (Instr_Add :: Instr_Mod :: acc)))
    | "<<", T_Dir, T_Int -> Instr_Place :: I(4) :: (compile_value e1 state (compile_value e2 state (Instr_Sub :: Instr_Mod :: acc)))
    | _ -> raise_failure "Unknown binary operation"
  )
  | Unary_op (op, e) -> ( match op with 
      | "!" -> compile_value e state (Instr_Not :: acc)
      | _ -> raise_failure "Unknown unary operation"
  )
  | FieldProp(v,f) -> compile_value v state (Instr_FieldProp :: I(prop_index f) :: acc)
  (* true = pre*)
  | Increment(target, true)  ->  compile_target_index target state (Instr_Copy :: Instr_Copy :: instr_access target state.scopes :: Instr_Place :: I(1) :: Instr_Add :: instr_assign target state.scopes :: instr_access target state.scopes :: acc)
  | Increment(target, false) ->  compile_target_index target state (Instr_Copy :: instr_access target state.scopes :: Instr_Swap :: Instr_Copy :: instr_access target state.scopes :: Instr_Place :: I(1) :: Instr_Add :: instr_assign target state.scopes :: acc)
  | Decrement(target, true)  ->  compile_target_index target state (Instr_Copy :: Instr_Copy :: instr_access target state.scopes :: Instr_Place :: I(1) :: Instr_Sub :: instr_assign target state.scopes :: instr_access target state.scopes :: acc)
  | Decrement(target, false) ->  compile_target_index target state (Instr_Copy :: instr_access target state.scopes :: Instr_Swap :: Instr_Copy :: instr_access target state.scopes :: Instr_Place :: I(1) :: Instr_Sub :: instr_assign target state.scopes :: acc)
  | PagerRead -> Instr_PagerRead :: acc
  | Read -> Instr_Read :: acc
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
    } 
  in
    let (body,_) = type_check_stmt new_state body in
    let vars = extract_declarations body in
    Instr_GoTo :: LabelRef(end_label) :: Label(func_label) :: Instr_Declare :: I(size_of_vars vars) :: compile_stmt body {new_state with scopes = ({ local = new_state.scopes.local @ vars; global = new_state.scopes.global })} (Instr_Place :: I(0) :: Instr_Return :: Label(end_label) :: Instr_Place :: LabelRef(func_label) :: acc)
  )
  | Call(f,args) -> (
    let comped_args = List.map (fun arg -> compile_value arg state []) args |> List.flatten in
    comped_args @ (compile_value f state (Instr_Place :: I(List.length args) :: Instr_Call :: acc))
  )
  | Ternary(c,a,b) -> (
    let label_true = Helpers.new_label () in
    let label_stop = Helpers.new_label () in
    compile_value c state (Instr_GoToIf :: LabelRef(label_true) :: (compile_value b state (Instr_GoTo :: LabelRef(label_stop) :: Label(label_true) :: (compile_value a state (Label(label_stop) :: acc)))))
  )


and compile_target_index target (state:compile_state) acc = match target with
  | Local name -> Instr_Place :: I(fetch_var_index name state.scopes) :: acc
  | Array(t,i) -> 
    let t_type = type_value state (Reference target) in
    let t_type_size = type_size t_type in
    Instr_Place :: I(t_type_size) :: compile_value i state (Instr_Mul :: compile_target_index t state (Instr_Add :: acc))


and compile_assignment target expr state acc =
    let target_type = type_value state (Reference target) in
    let expr_type = type_value state expr in
    match target_type, expr_type, expr with
    | T_Array(_,st), T_Array(_,se), Reference(e) -> (
      let num = min st se in
      List.init num (fun i -> i)
      |> List.fold_left (fun acc i -> compile_assignment (Array(target, Int(i))) (Reference(Array(e,Int(i)))) state acc) acc
    )
    | _,_,_ -> compile_target_index target state (compile_value expr state (instr_assign target state.scopes :: acc))


and compile_stmt (Stmt(stmt,ln)) state acc =
  try match stmt with
  | If (expr, s1, s2) -> (
    let label_true = Helpers.new_label () in
    let label_stop = Helpers.new_label () in
    compile_value expr state (Instr_GoToIf :: LabelRef(label_true) :: (compile_stmt s2 state (Instr_GoTo :: LabelRef(label_stop) :: Label(label_true) :: (compile_stmt s1 state (Label(label_stop) :: acc)))))
  )
  | IfIs(v,alts,opt) -> (
    let label_end = Helpers.new_label () in
    let rec comp_alts alts next acc = match alts with
      | [] -> acc
      | (alt_v,alt_s)::t -> Instr_Copy :: compile_value alt_v state (Instr_Eq :: Instr_Not :: Instr_GoToIf :: LabelRef(next) :: compile_stmt alt_s state (Instr_GoTo :: LabelRef(label_end) :: Label(next) :: (comp_alts t (Helpers.new_label ()) acc)))
    in
    match opt with
    | None -> compile_value v state (comp_alts alts (Helpers.new_label ()) (Label(label_end) :: acc))
    | Some other -> compile_value v state (comp_alts alts (Helpers.new_label ()) (Instr_DecStack :: compile_stmt other state (Label(label_end) :: acc)))
  )
  | Block (stmt_list) -> (
    List.fold_left (fun acc stmt -> compile_stmt stmt state acc) acc (List.rev stmt_list) 
  )
  | While(v,s,None) -> 
    let label_cond = Helpers.new_label () in
    let label_start = Helpers.new_label () in
    let label_stop = Helpers.new_label () in
    let state' = {state with break = Some label_stop; continue = Some label_cond } in
    Instr_GoTo :: LabelRef(label_cond) :: Label(label_start) :: (compile_stmt s state' (Label(label_cond) :: compile_value v state (Instr_GoToIf :: LabelRef(label_start) :: Label(label_stop)::acc)))
  | While(v,s,Some si) -> 
    let label_cond = Helpers.new_label () in
    let label_start = Helpers.new_label () in
    let label_stop = Helpers.new_label () in
    let label_incr = Helpers.new_label () in
    let state' = {state with break = Some label_stop; continue = Some label_incr } in
    Instr_GoTo :: LabelRef(label_cond) :: Label(label_start) :: (compile_stmt s state' (Label(label_incr) :: (compile_stmt si state (Label(label_cond) :: (compile_value v state (Instr_GoToIf :: LabelRef(label_start) :: Label(label_stop) :: acc))))))
  | Continue -> (match state.continue with
    | Some label -> Instr_GoTo :: LabelRef(label) :: acc
    | None -> raise_failure "Nothing to continue"
  )
  | Break -> (match state.break with
    | Some label -> Instr_GoTo :: LabelRef(label) :: acc
    | None -> raise_failure "Nothing to break out of"
  )
  | Assign (target, expr) -> compile_assignment target expr state acc
  | DeclareAssign (_, name, expr) -> compile_assignment (Local name) expr state acc
  | Label name -> Label name :: acc
  | Unit stmt -> (
    let instr = match stmt with
    | Wait -> Instr_Wait
    | Pass -> Instr_Pass
    | Projection -> Instr_Projection
    | Meditate -> Instr_Meditate
    in
    instr :: acc
  )
  | Directional(stmt, dir) -> (
    let instr = match stmt with
    | Shoot -> Instr_Shoot
    | Mine -> Instr_Mine
    | Fireball -> Instr_Fireball
    | Move -> Instr_Move
    | Chop -> Instr_Chop
    | Dispel -> Instr_Dispel
    | Disarm -> Instr_Disarm
    | ManaDrain -> Instr_ManaDrain
    | Wall -> Instr_Wall
    | Bridge -> Instr_Bridge
    | PlantTree -> Instr_PlantTree
    | Mount -> Instr_Mount
    | Dismount -> Instr_Dismount
    | Boat -> Instr_Boat
    | BearTrap -> Instr_BearTrap
    in
    compile_value dir state (instr :: acc)
  )
  | OptionDirectional(stmt, dir_opt) -> (
    let instr = match stmt with
    | Trench -> Instr_Trench
    | Fortify -> Instr_Fortify
    | Collect -> Instr_Collect
    in
    match dir_opt with
    | Some dir -> compile_value dir state (instr :: acc)
    | None ->  Instr_Place :: I(4) :: instr :: acc
  )
  | Targeting(stmt, dir, dis) -> (
    let instr = match stmt with
    | Bomb -> Instr_Bomb
    | Freeze -> Instr_Freeze
    in
    compile_value dir state (compile_value dis state (instr :: acc))
  )
  | GoTo n -> 
    if StringSet.mem n state.labels
    then Instr_GoTo :: LabelRef n :: acc
    else raise_failure ("Unavailable label: "^n)
  | Declare _ -> acc
  | PagerSet v -> compile_value v state (Instr_PagerSet :: acc)
  | PagerWrite v -> compile_value v state (Instr_PagerWrite :: acc)
  | Write v -> compile_value v state (Instr_Write :: acc)
  | Say v -> compile_value v state (Instr_Say :: acc)
  | Return v -> compile_value v state (Instr_Return :: acc)
  | CallStmt(f,args) -> compile_value (Call(f,args)) state (Instr_DecStack :: acc)
  with 
  | Failure(p,None,msg) -> raise (Failure(p,Some ln, msg))
  | a -> raise a

let compile_player (File program) =
  let program = Stmt(Block program,0) in
  let vars = extract_declarations program in
  let labels = available_labels program in
  let state = {scopes = { local = vars ; global = None }; labels = labels; break = None; continue = None; ret_type = None;} in
  Instr_Declare :: I(size_of_vars vars) :: compile_stmt program state []

