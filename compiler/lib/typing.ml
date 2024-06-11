open Absyn
open Exceptions

let type_string t = match t with
  | T_Int -> "int"
  | T_Dir -> "dir"
  | T_Field -> "field"

let require req_typ expr_t res = 
  if req_typ = expr_t then res ()
  else raise_failure ("required type: '" ^type_string req_typ ^"'")

let reg_type regs name = 
  match List.find_opt (fun (Register(_,rn,_)) -> rn = name) regs with
  | Some Register(t,_,_) -> t
  | None -> raise_failure ("No such register: "^name)

let rec type_value regs v = match v with
    | Reference Local n -> reg_type regs n  
    | Reference Global (t,_) -> t
    | MetaReference m -> type_meta m
    | Value v -> type_value regs v
    | Binary_op _
    | Unary_op _
    | Random
    | Int _ -> T_Int
    | Flag(v,_) -> require T_Field (type_value regs v) (fun () -> T_Int)
    | Scan(d,p)  ->
      require T_Dir (type_value regs d) (fun () -> ()) ;
      require T_Int (type_value regs p) (fun () -> ()) ;
      T_Field
    | Look e -> require T_Dir (type_value regs e) (fun () -> T_Int)
    | Direction _ -> T_Dir
    | RandomSet vals -> (match vals with
      | [] -> raise_failure "Empty random set"
      | h::t -> 
        let ty = type_value regs h in
        List.iter (fun h -> require ty (type_value regs h) (fun () -> ())) t ;
        ty
    )

and type_meta m = match m with
    | PlayerX     
    | PlayerY     
    | PlayerBombs 
    | PlayerShots 
    | BoardX      
    | BoardY 
    | GlobalArraySize -> T_Int     


let rec type_check_stmt_inner regs stmt = match stmt with
  | If(c,a,b) -> require T_Int (type_value regs c) (fun () -> type_check_stmt regs a ; type_check_stmt regs b)
  | Block stmts -> type_check_stmts regs stmts
  | Repeat(_,stmt) -> type_check_stmt regs stmt
  | Assign(Local n,e) -> require (reg_type regs n) (type_value regs e) (fun () -> ())
  | Assign(Global(t,_),e) -> require t (type_value regs e) (fun () -> ())
  | Move e 
  | Shoot e -> require T_Dir (type_value regs e) (fun () -> ())
  | Bomb(d,p) -> 
    require T_Dir (type_value regs d) (fun () -> ()) ;
    require T_Int (type_value regs p) (fun () -> ())
  | Attack d
  | Mine d -> require T_Dir (type_value regs d) (fun () -> ())
  | Fortify o
  | Trench o -> (match o with
    | Some e -> require T_Dir (type_value regs e) (fun () -> ())
    | None -> ()
  )
  | Wait
  | GoTo _
  | Label _
  | Pass -> ()

and type_check_stmt regs (Stmt(stmt,ln)) = 
  try 
    type_check_stmt_inner regs stmt
  with 
  | Failure(p, None, msg) -> raise (Failure(p, Some ln, msg))
  | e -> raise e

and type_check_stmts regs stmts =
  List.iter (type_check_stmt regs) stmts

let type_check_program (File(regs,prog)) =
  type_check_stmts regs prog ; File(regs,prog)