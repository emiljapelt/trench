open Absyn
open Exceptions

let type_string t = match t with
  | T_Int -> "int"
  | T_Dir -> "dir"
  | T_Field -> "field"

let require req_typ expr_t res = 
  if req_typ = expr_t then res ()
  else raise_failure ("required type: '" ^type_string req_typ ^"'")

let var_type vars name = 
  match List.find_opt (fun (Var(_,vn)) -> vn = name) vars with
  | Some Var(t,_) -> t
  | None -> raise_failure ("No such variable: "^name)

let rec type_value (state:compile_state) v = match v with
    | Reference Local n -> var_type state.vars n  
    | Reference Global (t,_) -> t
    | MetaReference m -> type_meta m
    | Binary_op(op,v0,v1) -> (match op, type_value state v0, type_value state v1 with
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
      | _,t0,t1 -> raise_failure ("Unknown binary operation: "^type_string t0^op^type_string t1)
    )
    | Unary_op _
    | Random
    | Int _ -> T_Int
    | Decrement(Local n,_) -> (match var_type state.vars n with 
      | T_Int -> T_Int
      | T_Dir -> T_Dir
      | _ -> raise_failure ("Only int and dir can be decremented")
    )
    | Decrement(Global(T_Int,_),_) -> T_Int
    | Decrement(Global(T_Dir,_),_) -> T_Dir
    | Decrement(Global(_,_),_) -> raise_failure ("Only int and dir can be decremented") 
    | Increment(Local n,_) -> (match var_type state.vars n with 
      | T_Int -> T_Int
      | T_Dir -> T_Dir
      | _ -> raise_failure ("Only int and dir can be incremented")
    )
    | Increment(Global(T_Int,_),_) -> T_Int
    | Increment(Global(T_Dir,_),_) -> T_Dir
    | Increment(Global(_,_),_) -> raise_failure ("Only int and dir can be decremented") 
    | Flag(v,_) -> require T_Field (type_value state v) (fun () -> T_Int)
    | Scan(d,p)  ->
      require T_Dir (type_value state d) (fun () -> ()) ;
      require T_Int (type_value state p) (fun () -> ()) ;
      T_Field
    | Look(e,_) -> require T_Dir (type_value state e) (fun () -> T_Int)
    | Direction _ -> T_Dir
    | RandomSet vals -> (match vals with
      | [] -> raise_failure "Empty random set"
      | h::t -> 
        let ty = type_value state h in
        List.iter (fun h -> require ty (type_value state h) (fun () -> ())) t ;
        ty
    )

and type_meta m = match m with
    | PlayerX     
    | PlayerY     
    | BoardX      
    | BoardY 
    | PlayerResource _
    | GlobalArraySize -> T_Int     


let rec type_check_stmt_inner state stmt = match stmt with
  | If(c,a,b) -> require T_Int (type_value state c) (fun () -> type_check_stmt state a |> ignore ; type_check_stmt state b |> ignore ; state)
  | IfIs(v,alts,opt) -> 
    let v_typ = type_value state v in
    let (alt_vs, alt_stmts) = List.split alts in
    let _ = List.iter (fun v -> require v_typ (type_value state v) (fun _ -> state) |> ignore) alt_vs in
    type_check_stmts state alt_stmts |> ignore ;
    Option.map (type_check_stmt state) opt |> ignore ;
    state
  | Block stmts -> type_check_stmts state stmts
  | While(v,s,None) -> 
    require T_Int (type_value state v) (fun () -> type_check_stmt state s |> ignore ; state)
  | While(v,s,Some si) -> 
    require T_Int (type_value state v) (fun () -> type_check_stmt state s |> ignore ; type_check_stmt state si |> ignore ; state)
  | Assign(Local n,e) -> require (var_type state.vars n) (type_value state e) (fun () -> state)
  | Assign(Global(t,_),e) -> require t (type_value state e) (fun () -> state)
  | Move e 
  | Shoot e -> require T_Dir (type_value state e) (fun () -> state)
  | Bomb(d,p) -> 
    require T_Dir (type_value state d) (fun () -> state) |> ignore ;
    require T_Int (type_value state p) (fun () -> state)
  | Attack d
  | Mine d -> require T_Dir (type_value state d) (fun () -> state)
  | Fortify o
  | Trench o -> (match o with
    | Some e -> require T_Dir (type_value state e) (fun () -> state)
    | None -> state
  )
  | DeclareAssign(t,n,v) -> require t (type_value state v) (fun () -> {state with vars = Var(t,n)::state.vars})
  | Declare(t,n) -> {state with vars = Var(t,n)::state.vars}
  | Wait
  | GoTo _
  | Label _
  | Continue
  | Break
  | Pass -> state

and type_check_stmt regs (Stmt(stmt,ln)) = 
  try 
    type_check_stmt_inner regs stmt
  with 
  | Failure(p, None, msg) -> raise (Failure(p, Some ln, msg))
  | e -> raise e

and type_check_stmts regs stmts =
  List.fold_left (fun regs stmt -> type_check_stmt regs stmt) regs stmts |> ignore ; regs

let type_check_program (File(vars,prog)) =
  type_check_stmts {vars = vars; break = None; continue = None} prog |> ignore ; File(vars,prog)