open Absyn
open ProgramRep
open Exceptions
open Optimize

(*** Compiling functions ***)

module StringSet = Set.Make(String)

let fetch_reg_index (name: string) regs = 
  let rec aux regs i = match regs with
    | [] -> failwith ("No such register: "^name)
    | Register(_,n,_)::t ->
      if n = name then i else aux t (i+1)
  in
  aux regs 0

let is_const_reg (name: string) regs = 
let rec aux regs i = match regs with
  | [] -> failwith ("No such register: "^name)
  | Register(c,n,_)::t ->
    if n = name then c else aux t (i+1)
in
aux regs 0

let rec compile_expr expr regs acc =
  match expr with
  | Reference name -> Instruction ("#"^(string_of_int (fetch_reg_index name regs))) :: acc
  | Value val_expr -> compile_value val_expr regs acc

and compile_value val_expr regs acc =
  match val_expr with
  | Int i -> Instruction("p"^string_of_int i) :: acc
  | Check d -> Instruction ("c"^direction_string d) :: acc
  | Scan d -> Instruction ("s"^direction_string d) :: acc
  | Binary_op (op, e1, e2) -> ( match op with
    | "&" -> compile_expr e1 regs (compile_expr e2 regs (Instruction "&" :: acc))
    | "|" -> compile_expr e1 regs (compile_expr e2 regs (Instruction "|" :: acc))
    | "=" -> compile_expr e1 regs (compile_expr e2 regs (Instruction "=" :: acc))
    | "!=" -> compile_expr e1 regs (compile_expr e2 regs (Instruction "=" :: Instruction "~" :: acc))
    | "<=" -> compile_expr e1 regs (compile_expr e2 regs (Instruction "<" :: Instruction "~" :: acc))
    | "<" -> compile_expr e2 regs (compile_expr e1 regs (Instruction "<" :: acc))
    | ">=" -> compile_expr e2 regs (compile_expr e1 regs (Instruction "<" :: Instruction "~" :: acc))
    | ">" -> compile_expr e1 regs (compile_expr e2 regs (Instruction "<" :: acc))
    | "+" -> compile_expr e1 regs (compile_expr e2 regs (Instruction "+" :: acc))
    | "/" -> compile_expr e2 regs (compile_expr e1 regs (Instruction "/" :: acc))
    | "%" -> compile_expr e2 regs (compile_expr e1 regs (Instruction "%" :: acc))
    | "-" -> compile_expr e2 regs (compile_expr e1 regs (Instruction "-" :: acc))
    | "*" -> compile_expr e1 regs (compile_expr e2 regs (Instruction "*" :: acc))
    | _ -> raise_failure "Unknown binary operation"
  )
  | Unary_op (op, e) -> match op with 
      | "~" -> compile_expr e regs (Instruction "~" :: acc)
      | _ -> raise_failure "Unknown unary operation"

and direction_string d = match d with
  | North -> "n"
  | East -> "e"
  | South -> "s"
  | West -> "w"

and compile_stmt (Stmt(stmt,ln)) regs acc =
  try match stmt with
  | If (expr, s1, s2) -> (
    let label_true = Helpers.new_label () in
    let label_stop = Helpers.new_label () in
    let opt_expr = optimize_expr expr regs in
    match opt_expr with 
    | Value(Int 0) -> (compile_stmt s2 regs acc)
    | Value(Int _) -> (compile_stmt s1 regs acc)
    | _ -> compile_expr expr regs (IfTrue(label_true,None) :: (compile_stmt s2 regs (CGoTo(label_stop, None) :: CLabel label_true :: (compile_stmt s1 regs (CLabel label_stop :: acc)))))
  )
  | Block (stmt_list) -> (
    List.fold_left (fun acc stmt -> compile_stmt stmt regs acc) acc stmt_list
  )
  | Assign (target, aexpr) -> 
    if is_const_reg target regs then raise_failure ("Assignment to constant register: "^target)
    else Instruction ("p"^string_of_int (fetch_reg_index target regs)) :: compile_expr (optimize_expr aexpr regs) regs (Instruction "a" :: acc)
  | Label name -> CLabel name :: acc
  | Move d -> Instruction ("m"^direction_string d) :: acc
  | Expand d -> Instruction ("E"^direction_string d) :: acc
  | Shoot d -> Instruction ("S"^direction_string d) :: acc
  | Fortify -> Instruction "F" :: acc
  | Wait -> Instruction "W" :: acc
  | Pass -> Instruction "P" :: acc
  | GoTo n -> CGoTo(n, None) :: acc
  | Bomb(x,y) -> compile_expr y regs (compile_expr x regs (Instruction "B" :: acc))
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


let default_regs = [
  Register(true, "x", Int 0);
  Register(true, "y", Int 0);
  Register(true, "b", Int 0);
]

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
    let regs = default_regs@regs in
    check_registers_unique regs;
    (regs,List.fold_right (fun stmt acc -> compile_stmt stmt regs acc) absyn [])
  )
  with 
  | Failure _ as f -> raise f
  | _ -> raise (Failure(None, "Parser error"))