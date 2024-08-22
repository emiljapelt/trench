open Absyn
open Exceptions

let type_string t = match t with
  | T_Int -> "int"
  | T_Dir -> "dir"
  | T_Field -> "field"

let require req_typ expr_t res = 
  if req_typ = expr_t then res ()
  else raise_failure ("required type: '" ^type_string req_typ ^"'")

let var_type regs name = 
  match List.find_opt (fun (Register(_,rn)) -> rn = name) regs with
  | Some Register(t,_) -> t
  | None -> raise_failure ("No such variable: "^name)

let rec type_value regs v = match v with
    | Reference Local n -> var_type regs n  
    | Reference Global (t,_) -> t
    | MetaReference m -> type_meta m
    | Binary_op(op,v0,v1) -> (match op, type_value regs v0, type_value regs v1 with
      | "+", T_Int, T_Int 
      | "-", T_Int, T_Int 
      | "*", T_Int, T_Int 
      | "&", T_Int, T_Int
      | "|", T_Int, T_Int 
      | "=", T_Int, T_Int
      | "=", T_Dir, T_Dir
      | "!=", T_Int, T_Int
      | "!=", T_Dir, T_Dir
      | "<", T_Int, T_Int 
      | ">", T_Int, T_Int 
      | "<=", T_Int, T_Int
      | ">=", T_Int, T_Int 
      | "/", T_Int, T_Int
      | "%", T_Int, T_Int -> T_Int
      | "+", T_Dir, T_Int 
      | "+", T_Int, T_Dir
      | "-", T_Dir, T_Int 
      | "-", T_Int, T_Dir -> T_Dir
      | _,t0,t1 -> failwith ("Unknown binary operation: "^type_string t0^op^type_string t1)
    )
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


let rec type_check_stmt_inner regs stmt : register list = match stmt with
  | If(c,a,b) -> require T_Int (type_value regs c) (fun () -> type_check_stmt regs a |> ignore ; type_check_stmt regs b |> ignore ; regs)
  | Block stmts -> type_check_stmts regs stmts
  | While(v,s,None) -> 
    require T_Int (type_value regs v) (fun () -> type_check_stmt regs s |> ignore ; regs)
  | While(v,s,Some si) -> 
    require T_Int (type_value regs v) (fun () -> type_check_stmt regs s |> ignore ; type_check_stmt regs si |> ignore ; regs)
  | Assign(Local n,e) -> require (var_type regs n) (type_value regs e) (fun () -> regs)
  | Assign(Global(t,_),e) -> require t (type_value regs e) (fun () -> regs)
  | Move e 
  | Shoot e -> require T_Dir (type_value regs e) (fun () -> regs)
  | Bomb(d,p) -> 
    require T_Dir (type_value regs d) (fun () -> regs) |> ignore ;
    require T_Int (type_value regs p) (fun () -> regs)
  | Attack d
  | Mine d -> require T_Dir (type_value regs d) (fun () -> regs)
  | Fortify o
  | Trench o -> (match o with
    | Some e -> require T_Dir (type_value regs e) (fun () -> regs)
    | None -> regs
  )
  | DeclareAssign(t,n,v) -> require t (type_value regs v) (fun () -> Register(t,n)::regs)
  | Declare(t,n) -> Register(t,n)::regs
  | Wait
  | GoTo _
  | Label _
  | Pass -> regs

and type_check_stmt regs (Stmt(stmt,ln)) = 
  try 
    type_check_stmt_inner regs stmt
  with 
  | Failure(p, None, msg) -> raise (Failure(p, Some ln, msg))
  | e -> raise e

and type_check_stmts regs stmts =
  List.fold_left (fun regs stmt -> type_check_stmt regs stmt) regs stmts |> ignore ; regs

let type_check_program (File(regs,prog)) =
  type_check_stmts regs prog |> ignore ; File(regs,prog)