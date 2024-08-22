open Absyn
open ProgramRep
open Exceptions
open Typing

(*** Compiling functions ***)

module StringSet = Set.Make(String)

let fetch_var_index name vars = 
  let rec aux vars i = match vars with
    | [] -> failwith ("No such register: "^name)
    | Var(_,n)::t ->
      if n = name then i else aux t (i+1)
  in
  aux vars 0

let rec compile_value val_expr (state:compile_state) acc =
  match val_expr with
  | Reference Local name -> Instruction ("#"^(Helpers.binary_int_string(fetch_var_index name state.vars))) :: acc
  | Reference Global (t,v) -> Instruction ("p"^Helpers.binary_int_string (type_index t)) :: (compile_value v state (Instruction "@" :: acc))
  | MetaReference md -> ( match md with
    | PlayerX -> Instruction ("#x") 
    | PlayerY -> Instruction ("#y") 
    | PlayerBombs -> Instruction ("#b")
    | PlayerShots -> Instruction ("#s")
    | BoardX -> Instruction ("#_")
    | BoardY -> Instruction ("#|")
    | GlobalArraySize -> Instruction ("#g")
  ) :: acc
  | Int i -> Instruction("p"^Helpers.binary_int_string i) :: acc
  | Random -> Instruction("r") :: acc
  | RandomSet vals -> 
    List.fold_left (fun acc v -> compile_value v state acc) (Instruction("R"^(Helpers.binary_int_string (List.length vals))) ::acc) vals
  | Direction d -> Instruction ("p"^Helpers.binary_int_string(int_of_dir d)) :: acc
  | Look d -> compile_value d state (Instruction "l" :: acc)
  | Scan(d,p) -> compile_value d state (compile_value p state (Instruction "s" :: acc))
  | Binary_op (op, e1, e2) -> ( match op, type_value state e1, type_value state e2 with
    | "+", T_Int, T_Int -> compile_value e1 state (compile_value e2 state (Instruction "+" :: acc))
    | "-", T_Int, T_Int -> compile_value e2 state (compile_value e1 state (Instruction "-" :: acc))
    | "*", T_Int, T_Int -> compile_value e1 state (compile_value e2 state (Instruction "*" :: acc))
    | "&", T_Int, T_Int -> compile_value e1 state (compile_value e2 state (Instruction "&" :: acc))
    | "|", T_Int, T_Int -> compile_value e1 state (compile_value e2 state (Instruction "|" :: acc))
    | "=", T_Int, T_Int -> compile_value e1 state (compile_value e2 state (Instruction "=" :: acc))
    | "=", T_Dir, T_Dir -> compile_value e1 state (compile_value e2 state (Instruction "=" :: acc))
    | "!=", T_Int, T_Int -> compile_value e1 state (compile_value e2 state (Instruction "=" :: Instruction "~" :: acc))
    | "!=", T_Dir, T_Dir -> compile_value e1 state (compile_value e2 state (Instruction "=" :: Instruction "~" :: acc))
    | "<", T_Int, T_Int -> compile_value e2 state (compile_value e1 state (Instruction "<" :: acc))
    | ">", T_Int, T_Int -> compile_value e1 state (compile_value e2 state (Instruction "<" :: acc))
    | "<=", T_Int, T_Int -> compile_value e1 state (compile_value e2 state (Instruction "<" :: Instruction "~" :: acc))
    | ">=", T_Int, T_Int -> compile_value e2 state (compile_value e1 state (Instruction "<" :: Instruction "~" :: acc))
    | "/", T_Int, T_Int -> compile_value e2 state (compile_value e1 state (Instruction "/" :: acc))
    | "%", T_Int, T_Int -> compile_value e2 state (compile_value e1 state (Instruction "%" :: acc))
    | "+", T_Dir, T_Int -> Instruction ("p"^Helpers.binary_int_string 4) :: (compile_value e1 state (compile_value e2 state (Instruction "+%" :: acc)))
    | "+", T_Int, T_Dir -> Instruction ("p"^Helpers.binary_int_string 4) :: (compile_value e1 state (compile_value e2 state (Instruction "+%" :: acc)))
    (*Subtraction from direction does not work currently*)
    | "-", T_Dir, T_Int -> Instruction ("p"^Helpers.binary_int_string 4) :: (compile_value e1 state (compile_value e2 state (Instruction "-%" :: acc)))
    | "-", T_Int, T_Dir -> Instruction ("p"^Helpers.binary_int_string 4) :: (compile_value e2 state (compile_value e1 state (Instruction "-%" :: acc)))
    | _ -> raise_failure "Unknown binary operation"
  )
  | Unary_op (op, e) -> ( match op with 
      | "~" -> compile_value e state (Instruction "~" :: acc)
      | _ -> raise_failure "Unknown unary operation"
  )
  | Flag(v,f) -> ( match f with 
    | PLAYER -> compile_value v state (Instruction ("'"^Helpers.binary_int_string 1) :: acc)
    | TRENCH -> compile_value v state (Instruction ("'"^Helpers.binary_int_string 2) :: acc)
    | MINE -> compile_value v state (Instruction ("'"^Helpers.binary_int_string 4) :: acc)
    | DESTROYED -> compile_value v state (Instruction ("'"^Helpers.binary_int_string 8) :: acc)
  )

and compile_stmt (Stmt(stmt,ln)) (state:compile_state) acc =
  try match stmt with
  | If (expr, s1, s2) -> (
    let label_true = Helpers.new_label () in
    let label_stop = Helpers.new_label () in
    compile_value expr state (IfTrue(label_true,None) :: (compile_stmt s2 state (CGoTo(label_stop, None) :: CLabel label_true :: (compile_stmt s1 state (CLabel label_stop :: acc)))))
  )
  | Block (stmt_list) -> (
    List.fold_left (fun acc stmt -> compile_stmt stmt state acc) acc (List.rev stmt_list) 
  )
  | While(v,s,None) -> 
    let label_cond = Helpers.new_label () in
    let label_start = Helpers.new_label () in
    let label_stop = Helpers.new_label () in
    let state' = {state with break = Some label_stop; continue = Some label_cond } in
    CGoTo(label_cond,None) :: CLabel(label_start) :: (compile_stmt s state' (CLabel(label_cond) :: compile_value v state (IfTrue(label_start,None) :: CLabel(label_stop)::acc)))
  | While(v,s,Some si) -> 
    let label_cond = Helpers.new_label () in
    let label_start = Helpers.new_label () in
    let label_stop = Helpers.new_label () in
    let label_incr = Helpers.new_label () in
    let state' = {state with break = Some label_stop; continue = Some label_incr } in
    CGoTo(label_cond,None) :: CLabel(label_start) :: (compile_stmt s state' (CLabel(label_incr) :: (compile_stmt si state (CLabel(label_cond) :: (compile_value v state (IfTrue(label_start,None) :: CLabel(label_stop) :: acc))))))
  | Continue -> (match state.continue with
    | Some label -> CGoTo(label,None) :: acc
    | None -> raise_failure "Nothing to continue"
  )
  | Break -> (match state.break with
    | Some label -> CGoTo(label,None) :: acc
    | None -> raise_failure "Nothing to break out of"
  )
  | Assign (Local target, aexpr) -> 
    Instruction ("p"^Helpers.binary_int_string (fetch_var_index target state.vars)) :: compile_value aexpr state (Instruction "a_" :: acc)
  | Assign (Global(typ,v), aexpr) -> 
    Instruction ("p"^Helpers.binary_int_string (type_index typ)) :: compile_value v state (compile_value aexpr state (Instruction "a@" :: acc))
  | Label name -> CLabel name :: acc
  | Move e -> compile_value e state (Instruction "m" :: acc)
  | Shoot e -> compile_value e state (Instruction "S" :: acc)
  | Fortify Some e -> compile_value e state ( Instruction "F" :: acc)
  | Fortify None -> Instruction ("p"^Helpers.binary_int_string 4^"F") :: acc
  | Trench Some e -> compile_value e state ( Instruction "T" :: acc)
  | Trench None -> Instruction ("p"^Helpers.binary_int_string 4^"T") :: acc
  | Wait -> Instruction "W" :: acc
  | Pass -> Instruction "P" :: acc
  | GoTo n -> CGoTo(n, None) :: acc
  | Bomb(d,i) -> compile_value d state (compile_value i state (Instruction "B" :: acc))
  | Mine d -> compile_value d state (Instruction "M" :: acc)
  | Attack d -> compile_value d state (Instruction "A" :: acc)
  | Declare _ -> Instruction ("p"^Helpers.binary_int_string 0) :: acc
  | DeclareAssign _ -> failwith "DeclareAssign still present"
  with 
  | Failure(p,None,msg) -> raise (Failure(p,Some ln, msg))
  | a -> raise a

let compile_player absyn =
  let File(vars,program) = absyn in
  (vars,List.fold_right (fun stmt acc -> compile_stmt stmt {vars = vars; break = None; continue = None} acc) program [])
