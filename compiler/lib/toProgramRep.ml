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
  | Reference Local name -> Instruction ("p"^(Helpers.binary_int_string(fetch_var_index name state.vars))^"#v") :: acc
  | Reference Global (t,v) -> Instruction ("p"^Helpers.binary_int_string (type_index t)) :: (compile_value v state (Instruction "@" :: acc))
  | MetaReference md -> ( match md with
    | PlayerX -> Instruction ("#x") 
    | PlayerY -> Instruction ("#y") 
    | BoardX -> Instruction ("#_")
    | BoardY -> Instruction ("#|")
    | GlobalArraySize -> Instruction ("#g")
    | PlayerResource n -> ( match List.find_index (fun name -> n = name) Flags.compile_flags.resources with
      | Some i -> Instruction("#r"^Helpers.binary_int_string(i))
      | None -> raise_failure ("Unknown resource lookup: "^n)
    )
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
    | "-", T_Int, T_Int -> compile_value e1 state (compile_value e2 state (Instruction "-" :: acc))
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
  (* true = pre*)
  | Increment(Local n, true)  -> Instruction ("p"^(Helpers.binary_int_string(fetch_var_index n state.vars))) :: Instruction ("cc#vp"^(Helpers.binary_int_string 1)) :: Instruction "+a_#v" :: acc
  | Increment(Local n, false) -> Instruction ("p"^(Helpers.binary_int_string(fetch_var_index n state.vars))) :: Instruction ("c#v^c#vp"^(Helpers.binary_int_string 1)) :: Instruction "+a_" :: acc
  | Increment(Global (_,_), true) -> failwith "Global incrementing not implemented"
  | Increment(Global (_,_), false) -> failwith "Global incrementing not implemented"
  | Decrement(Local n, true)  -> Instruction ("p"^(Helpers.binary_int_string(fetch_var_index n state.vars))) :: Instruction ("cc#vp"^(Helpers.binary_int_string 1)) :: Instruction "-a_#v" :: acc
  | Decrement(Local n, false) -> Instruction ("p"^(Helpers.binary_int_string(fetch_var_index n state.vars))) :: Instruction ("c#v^c#vp"^(Helpers.binary_int_string 1)) :: Instruction "-a_" :: acc
  | Decrement(Global (_,_), true) -> failwith "Global decrementing not implemented"
  | Decrement(Global (_,_), false) -> failwith "Global decrementing not implemented"

and compile_stmt (Stmt(stmt,ln)) (state:compile_state) acc =
  try match stmt with
  | If (expr, s1, s2) -> (
    let label_true = Helpers.new_label () in
    let label_stop = Helpers.new_label () in
    compile_value expr state (IfTrue label_true :: (compile_stmt s2 state (CGoTo label_stop :: CLabel label_true :: (compile_stmt s1 state (CLabel label_stop :: acc)))))
  )
  | IfIs(v,alts,opt) -> (
    let label_end = Helpers.new_label () in
    let rec comp_alts alts next acc = match alts with
      | [] -> acc
      | (alt_v,alt_s)::t -> Instruction("c") :: compile_value alt_v state (Instruction("=~") :: IfTrue next :: compile_stmt alt_s state (CGoTo label_end :: CLabel(next) :: (comp_alts t (Helpers.new_label ()) acc)))
    in
    match opt with
    | None -> compile_value v state (comp_alts alts (Helpers.new_label ()) (CLabel(label_end) :: acc))
    | Some other -> compile_value v state (comp_alts alts (Helpers.new_label ()) (Instruction "d" :: compile_stmt other state (CLabel(label_end) :: acc)))
  )
  | Block (stmt_list) -> (
    List.fold_left (fun acc stmt -> compile_stmt stmt state acc) acc (List.rev stmt_list) 
  )
  | While(v,s,None) -> 
    let label_cond = Helpers.new_label () in
    let label_start = Helpers.new_label () in
    let label_stop = Helpers.new_label () in
    let state' = {state with break = Some label_stop; continue = Some label_cond } in
    CGoTo label_cond :: CLabel(label_start) :: (compile_stmt s state' (CLabel(label_cond) :: compile_value v state (IfTrue label_start :: CLabel(label_stop)::acc)))
  | While(v,s,Some si) -> 
    let label_cond = Helpers.new_label () in
    let label_start = Helpers.new_label () in
    let label_stop = Helpers.new_label () in
    let label_incr = Helpers.new_label () in
    let state' = {state with break = Some label_stop; continue = Some label_incr } in
    CGoTo label_cond :: CLabel(label_start) :: (compile_stmt s state' (CLabel(label_incr) :: (compile_stmt si state (CLabel(label_cond) :: (compile_value v state (IfTrue label_start :: CLabel(label_stop) :: acc))))))
  | Continue -> (match state.continue with
    | Some label -> CGoTo label :: acc
    | None -> raise_failure "Nothing to continue"
  )
  | Break -> (match state.break with
    | Some label -> CGoTo label :: acc
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
  | GoTo n -> CGoTo n :: acc
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

