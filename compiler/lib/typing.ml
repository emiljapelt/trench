open Absyn
open Exceptions

let rec type_string t = match t with
  | T_Int -> "int"
  | T_Dir -> "dir"
  | T_Field -> "field"
  | T_Array(t,_) -> (type_string t) ^ "[]"

let rec type_eq t1 t2 = match t1,t2 with
  | T_Int, T_Int
  | T_Dir, T_Dir
  | T_Field, T_Field -> true
  | T_Array(st1,_), T_Array(st2,_) -> type_eq st1 st2
  | _ -> false

let require req_typ expr_t res = 
  if type_eq req_typ expr_t then res ()
  else raise_failure ("required type: '" ^type_string req_typ^ "' but got :'" ^type_string expr_t^ "'")

let var_type vars name = 
  match List.find_opt (fun (Var(_,vn)) -> vn = name) vars with
  | Some Var(t,_) -> t
  | None -> raise_failure ("No such variable: "^name)

let rec type_value (state:compile_state) v = match v with
    | Reference Local n -> var_type state.vars n  
    | Reference Array(target,idx) -> (
      require T_Int (type_value state idx) (fun () -> ()) ;
      match type_value state (Reference target) with 
      | T_Array(t,_) -> t
      | _ -> raise_failure ("not an array")
    )
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
      | "<<", T_Dir, T_Int 
      | ">>", T_Dir, T_Int -> T_Dir
      | _,t0,t1 -> raise_failure ("Unknown binary operation: "^type_string t0^op^type_string t1)
    )
    | Unary_op _
    | Random
    | Int _ -> T_Int
    | Decrement(target,_) -> (match type_value state (Reference target) with 
      | T_Int -> T_Int
      | _ -> raise_failure ("Only int and dir can be decremented")
    )
    | Increment(target,_) -> (match type_value state (Reference target) with 
      | T_Int -> T_Int
      | _ -> raise_failure ("Only int and dir can be incremented")
    )
    | FieldProp(v,_) -> require T_Field (type_value state v) (fun () -> T_Int)
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
    | PagerRead
    | Read -> T_Int

and type_meta m = match m with
    | PlayerX     
    | PlayerY
    | BoardX      
    | BoardY 
    | PlayerID
    | PlayerResource _ -> T_Int


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

  | Assign(Array(target, index), e) -> (
    require T_Int (type_value state index) (fun () -> 
      match type_value state (Reference target) with
      | T_Array(t,_) -> require (t) (type_value state e) (fun () -> state)
      | _ -> raise_failure "array assignment to non-array"
    )
  )

  | Directional(_,dir) -> require T_Dir (type_value state dir) (fun () -> state)
  | OptionDirectional(_,None) -> state
  | OptionDirectional(_,Some dir) -> require T_Dir (type_value state dir) (fun () -> state)
  | Targeting(_,dir,dis) -> 
    require T_Dir (type_value state dir) (fun () -> 
      require T_Int (type_value state dis) (fun () -> state)
    )
  | DeclareAssign(t,n,v) -> require t (type_value state v) (fun () -> {state with vars = Var(t,n)::state.vars})
  | Declare(t,n) -> {state with vars = Var(t,n)::state.vars}
  | GoTo _
  | Label _
  | Continue
  | Break
  | Unit _ -> state
  | PagerSet v
  | PagerWrite v
  | Write v -> require T_Int (type_value state v) (fun () -> state)
  | Say v -> require T_Int (type_value state v) (fun () -> state)

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