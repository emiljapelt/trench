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
  | Unary_op (op, e) -> match op with 
      | "~" -> compile_value e regs (Instruction "~" :: acc)
      | _ -> raise_failure "Unknown unary operation"

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
  with 
  | Failure(None,msg) -> raise (Failure(Some ln, msg))
  | a -> raise a

let compress_path path =
  let rec compress parts acc =
    match parts with
    | [] -> List.rev acc
    | h::t when h = "." -> compress t (acc)
    | _::h2::t when h2 = ".." -> compress t acc
    | h::t -> compress t (h::acc) 
  in
  String.concat "/" (compress (String.split_on_char '/' path) [])

let total_path path =
  if path.[0] = '.' then Sys.getcwd () ^ "/" ^ path
  else path

let complete_path base path = compress_path (if path.[0] = '.' then (String.sub base 0 ((String.rindex base '/')+1) ^ path) else path)


let check_registers_unique regs =
  let rec aux regs set = match regs with
  | [] -> ()
  | Register(_,n,_)::t -> 
    if StringSet.mem n set 
    then raise_failure ("Duplicate register name: "^n) 
    else aux t (StringSet.add n set)
  in
  aux regs StringSet.empty


let compile path parse =
  let path = (compress_path (total_path path)) in
  try (
    let File(regs,absyn) = parse path in
    check_registers_unique regs;
    (regs,List.fold_right (fun stmt acc -> compile_stmt stmt regs acc) absyn [])
  )
  with 
  | Failure _ as f -> raise f
  | _ -> raise (Failure(None, "Parser error"))