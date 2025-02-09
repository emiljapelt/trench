open Absyn
open ProgramRep
open Exceptions
open Typing

(*** Compiling functions ***)

module StringSet = Set.Make(String)

let fetch_var_index name vars = 
  let rec aux vars i = match vars with
    | [] -> raise_failure ("No such register: "^name)
    | Var(_,n)::t ->
      if n = name then i else aux t (i+1)
  in
  aux vars 0

let rec compile_value val_expr (state:compile_state) acc =
  match val_expr with
  | Reference Local name ->  Instr_Place :: I(fetch_var_index name state.vars) :: Instr_Access :: acc
  | MetaReference md -> ( match md with
    | PlayerX -> Meta_PlayerX :: acc
    | PlayerY -> Meta_PlayerY :: acc
    | BoardX -> Meta_BoardX :: acc
    | BoardY -> Meta_BoardY :: acc
    | PlayerID -> Meta_PlayerID :: acc
    | PlayerResource n -> ( match List.find_index (fun name -> n = name) Flags.compile_flags.resources with
      | Some i -> Meta_Resource :: I(i) :: acc
      | None -> raise_failure ("Unknown resource lookup: "^n)
    )
  )
  | Int i -> Instr_Place :: I(i) :: acc
  | Random -> Instr_Random :: acc
  | RandomSet vals -> 
    List.fold_left (fun acc v -> compile_value v state acc) (Instr_RandomSet :: I(List.length vals) :: acc) vals
  | Direction d -> Instr_Place :: I(int_of_dir d) :: acc
  | Look(d,f) -> compile_value d state (Instr_Look :: I(flag_index f) :: acc)
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
    | "+", T_Dir, T_Int -> Instr_Place :: I(4) :: (compile_value e1 state (compile_value e2 state (Instr_Add :: Instr_Mod :: acc)))
    | "+", T_Int, T_Dir -> Instr_Place :: I(4) :: (compile_value e1 state (compile_value e2 state (Instr_Add :: Instr_Mod :: acc)))
    (*Subtraction from direction does not work currently*)
    | "-", T_Dir, T_Int -> Instr_Place :: I(4) :: (compile_value e1 state (compile_value e2 state (Instr_Sub :: Instr_Mod :: acc)))
    | "-", T_Int, T_Dir -> Instr_Place :: I(4) :: (compile_value e2 state (compile_value e1 state (Instr_Sub :: Instr_Mod :: acc)))
    | _ -> raise_failure "Unknown binary operation"
  )
  | Unary_op (op, e) -> ( match op with 
      | "~" -> compile_value e state (Instr_Not :: acc)
      | _ -> raise_failure "Unknown unary operation"
  )
  | Flag(v,f) -> compile_value v state (Instr_FieldFlag :: I(flag_index f) :: acc)
  (* true = pre*)
  | Increment(Local n, true)  -> Instr_Place :: I(fetch_var_index n state.vars) :: Instr_Copy :: Instr_Copy :: Instr_Access :: Instr_Place :: I(1) :: Instr_Add :: Instr_Assign :: Instr_Access :: acc
  | Increment(Local n, false) -> Instr_Place :: I(fetch_var_index n state.vars) :: Instr_Copy :: Instr_Access :: Instr_Swap :: Instr_Copy :: Instr_Access :: Instr_Place :: I(1) :: Instr_Add :: Instr_Assign :: acc
  | Decrement(Local n, true)  -> Instr_Place :: I(fetch_var_index n state.vars) :: Instr_Copy :: Instr_Copy :: Instr_Access :: Instr_Place :: I(1) :: Instr_Sub :: Instr_Assign :: Instr_Access :: acc
  | Decrement(Local n, false) -> Instr_Place :: I(fetch_var_index n state.vars) :: Instr_Copy :: Instr_Access :: Instr_Swap :: Instr_Copy :: Instr_Access :: Instr_Place :: I(1) :: Instr_Sub :: Instr_Assign :: acc
  | Read -> Instr_Read :: acc


and compile_stmt (Stmt(stmt,ln)) (state:compile_state) acc =
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
  | Assign (Local target, aexpr) -> 
    Instr_Place :: I(fetch_var_index target state.vars) :: compile_value aexpr state (Instr_Assign :: acc)
  | Label name -> Label name :: acc
  | Move e -> compile_value e state (Instr_Move :: acc)
  | Shoot e -> compile_value e state (Instr_Shoot :: acc)
  | Fortify Some e -> compile_value e state (Instr_Fortify :: acc)
  | Fortify None -> Instr_Place :: I(4) :: Instr_Fortify :: acc
  | Trench Some e -> compile_value e state ( Instr_Trench :: acc)
  | Trench None -> Instr_Place :: I(4) :: Instr_Trench :: acc
  | Wait -> Instr_Wait :: acc
  | Pass -> Instr_Pass :: acc
  | GoTo n -> Instr_GoTo :: LabelRef n :: acc
  | Bomb(d,i) -> compile_value d state (compile_value i state (Instr_Bomb :: acc))
  | Freeze(d,i) -> compile_value d state (compile_value i state (Instr_Freeze :: acc))
  | Mine d -> compile_value d state (Instr_Mine :: acc)
  | Attack d -> compile_value d state (Instr_Melee :: acc)
  | Declare _ -> Instr_Place :: I(0) :: acc
  | Write v -> compile_value v state (Instr_Write :: acc)
  | Projection -> Instr_Projection :: acc
  | DeclareAssign _ -> failwith "DeclareAssign still present"
  with 
  | Failure(p,None,msg) -> raise (Failure(p,Some ln, msg))
  | a -> raise a

let compile_player absyn =
  let File(vars,program) = absyn in
  (vars,List.fold_right (fun stmt acc -> compile_stmt stmt {vars = vars; break = None; continue = None} acc) program [])

