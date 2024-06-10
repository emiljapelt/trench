open Absyn
open ProgramRep
open Exceptions

(*** Compiling functions ***)

module StringSet = Set.Make(String)

let fetch_reg_index (name: string) regs = 
  let rec aux regs i = match regs with
    | [] -> failwith ("No such register: "^name)
    | Register(_,n,_)::t ->
      if n = name then i else aux t (i+1)
  in
  aux regs 0

let rec compile_value val_expr regs acc =
  match val_expr with
  | Reference name -> Instruction ("#"^(string_of_int (fetch_reg_index name regs))) :: acc
  | MetaReference md -> (match md with
    | PlayerX -> Instruction ("#x") :: acc
    | PlayerY -> Instruction ("#y") :: acc
    | PlayerBombs -> Instruction ("#b") :: acc
    | PlayerShots -> Instruction ("#s") :: acc
    | BoardX -> Instruction ("#_") :: acc
    | BoardY -> Instruction ("#|") :: acc
  )
  | Value val_expr -> compile_value val_expr regs acc
  | Int i -> Instruction("p"^string_of_int i) :: acc
  | Random -> Instruction("r") :: acc
  | RandomSet vals -> 
    List.fold_left (fun acc v -> compile_value v regs acc) (Instruction("R"^(string_of_int (List.length vals))) ::acc) vals
  | Direction d -> Instruction ("p"^string_of_dir d) :: acc
  | Check e -> compile_value e regs (Instruction "c" :: acc)
  | Scan e -> compile_value e regs (Instruction "s" :: acc)
  | Binary_op (op, e1, e2) -> ( match op with
    | "&" -> compile_value e1 regs (compile_value e2 regs (Instruction "&" :: acc))
    | "|" -> compile_value e1 regs (compile_value e2 regs (Instruction "|" :: acc))
    | "=" -> compile_value e1 regs (compile_value e2 regs (Instruction "=" :: acc))
    | "!=" -> compile_value e1 regs (compile_value e2 regs (Instruction "=" :: Instruction "~" :: acc))
    | "<=" -> compile_value e1 regs (compile_value e2 regs (Instruction "<" :: Instruction "~" :: acc))
    | "<" -> compile_value e2 regs (compile_value e1 regs (Instruction "<" :: acc))
    | ">=" -> compile_value e2 regs (compile_value e1 regs (Instruction "<" :: Instruction "~" :: acc))
    | ">" -> compile_value e1 regs (compile_value e2 regs (Instruction "<" :: acc))
    | "+" -> compile_value e1 regs (compile_value e2 regs (Instruction "+" :: acc))
    | "/" -> compile_value e2 regs (compile_value e1 regs (Instruction "/" :: acc))
    | "%" -> compile_value e2 regs (compile_value e1 regs (Instruction "%" :: acc))
    | "-" -> compile_value e2 regs (compile_value e1 regs (Instruction "-" :: acc))
    | "*" -> compile_value e1 regs (compile_value e2 regs (Instruction "*" :: acc))
    | _ -> raise_failure "Unknown binary operation"
  )
  | Unary_op (op, e) -> ( match op with 
      | "~" -> compile_value e regs (Instruction "~" :: acc)
      | _ -> raise_failure "Unknown unary operation"
  )
  | Flag(v,f) -> ( match f with 
    | PLAYER -> compile_value v regs (Instruction "'1" :: acc)
    | TRENCH -> compile_value v regs (Instruction "'2" :: acc)
    | MINE -> compile_value v regs (Instruction "'4" :: acc)
    | DESTROYED -> compile_value v regs (Instruction "'8" :: acc)
  )

and compile_stmt (Stmt(stmt,ln)) regs acc =
  try match stmt with
  | If (expr, s1, s2) -> (
    let label_true = Helpers.new_label () in
    let label_stop = Helpers.new_label () in
    compile_value expr regs (IfTrue(label_true,None) :: (compile_stmt s2 regs (CGoTo(label_stop, None) :: CLabel label_true :: (compile_stmt s1 regs (CLabel label_stop :: acc)))))
  )
  | Block (stmt_list) -> (
    List.fold_left (fun acc stmt -> compile_stmt stmt regs acc) acc (List.rev stmt_list) 
  )
  | Repeat (count, stmt) -> (
    let rec aux c s acc = 
      if c = 0 then acc else aux (c-1) s (compile_stmt s regs acc)
    in
    aux count stmt acc
  )
  | Assign (target, aexpr) -> 
    Instruction ("p"^string_of_int (fetch_reg_index target regs)) :: compile_value aexpr regs (Instruction "a" :: acc)
  | Label name -> CLabel name :: acc
  | Move e -> compile_value e regs (Instruction "m" :: acc)
  | Shoot e -> compile_value e regs (Instruction "S" :: acc)
  | Fortify Some e -> compile_value e regs ( Instruction "F" :: acc)
  | Fortify None -> Instruction "p4F" :: acc
  | Trench Some e -> compile_value e regs ( Instruction "T" :: acc)
  | Trench None -> Instruction "p4T" :: acc
  | Wait -> Instruction "W" :: acc
  | Pass -> Instruction "P" :: acc
  | GoTo n -> CGoTo(n, None) :: acc
  | Bomb(d,i) -> compile_value d regs (compile_value i regs (Instruction "B" :: acc))
  | Mine d -> compile_value d regs (Instruction "M" :: acc)
  | Attack d -> compile_value d regs (Instruction "A" :: acc)
  with 
  | Failure(p,None,msg) -> raise (Failure(p,Some ln, msg))
  | a -> raise a

let compile_player absyn =
  let File(regs,program) = absyn in
  (regs,List.fold_right (fun stmt acc -> compile_stmt stmt regs acc) program [])
