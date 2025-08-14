open Absyn
open ProgramRep
open Exceptions

let rec type_string t = match t with
  | T_Int -> "int"
  | T_Dir -> "dir"
  | T_Field -> "field"
  | T_Array(t,_) -> (type_string t) ^ "[]"
  | T_Func(r,args) -> (type_string r) ^ ":(" ^ (args |> List.map type_string |> String.concat ",")  ^ ")"

let rec type_eq t1 t2 = match t1,t2 with
  | T_Int, T_Int
  | T_Dir, T_Dir
  | T_Field, T_Field -> true
  | T_Array(st1,_), T_Array(st2,_) -> type_eq st1 st2
  | T_Func(ret, params), T_Func(ret', params') -> 
    List.length params = List.length params' 
    && List.combine params params' |> List.for_all (fun (p,p') -> type_eq p p')
    && type_eq ret ret'  
  | _ -> false

let require req_typ expr_t res = 
  if type_eq req_typ expr_t then res ()
  else raise_failure ("required type: '" ^type_string req_typ^ "' but got :'" ^type_string expr_t^ "'")

let _string_of_vars vars = 
  vars 
  |> List.map (fun (Var(_,n)) -> n) 
  |> String.concat ", "

let var_type vars name = 
  match List.find_opt (fun (Var(_,vn)) -> vn = name) vars with
  | Some Var(t,_) -> t
  | None -> raise_failure ("No such variable: "^name ^ ":::" ^ _string_of_vars vars)

let rec type_value state v = match v with
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
    | Func(ret,args,_) -> T_Func(ret, List.map fst args)
    | Call(f,args) -> (
      let f_type = type_value state f in match f_type with
      | T_Func(ret, params) -> (
        if List.length params != List.length args then raise_failure "Incorrect amount of arguments"
        else if not (
          args 
          |> List.map (type_value state) 
          |> List.combine params
          |> List.for_all (fun (p, a) -> type_eq p a)
        )
        then raise_failure "Argument type mismatch"
        else ret
      )
      | _ -> raise_failure "Not a callable type"
    )
    | Ternary(c,a,b) -> 
      require T_Int (type_value state c) (fun () -> ()) ;
      let a_typ = type_value state a in
      let b_typ = type_value state b in
      if not(type_eq a_typ b_typ) then raise_failure "Type mismatch"
      else a_typ;
      

and type_meta m = match m with
    | PlayerX     
    | PlayerY
    | BoardX      
    | BoardY 
    | PlayerID
    | PlayerResource _ -> T_Int


let rec type_check_stmt_inner state stmt = match stmt with
  | If(c,a,b) -> require T_Int (type_value state c) (fun () -> type_check_stmt state a |> ignore ; type_check_stmt state b |> ignore ; (stmt, state))
  | IfIs(v,alts,opt) -> 
    let v_typ = type_value state v in
    let (alt_vs, alt_stmts) = List.split alts in
    let _ = List.iter (fun v -> require v_typ (type_value state v) (fun _ -> state) |> ignore) alt_vs in
    type_check_stmts state alt_stmts |> ignore ;
    Option.map (type_check_stmt state) opt |> ignore ;
    (stmt, state)
  | Block stmts -> (
    let (state', stmts') = type_check_stmts state stmts in
    (Block stmts', state')
  )
  | While(v,s,None) -> 
    require T_Int (type_value state v) (fun () -> type_check_stmt state s |> ignore ; (stmt,state))
  | While(v,s,Some si) -> 
    require T_Int (type_value state v) (fun () -> type_check_stmt state s |> ignore ; type_check_stmt state si |> ignore ; (stmt, state))
  | Assign(Local n,e) -> require (var_type state.vars n) (type_value state e) (fun () -> (stmt,state))
  | Assign(Array(target, index), e) -> (
    require T_Int (type_value state index) (fun () -> 
      match type_value state (Reference target) with
      | T_Array(t,_) -> require (t) (type_value state e) (fun () -> (stmt,state))
      | _ -> raise_failure "array assignment to non-array"
    )
  )
  | Directional(_,dir) -> require T_Dir (type_value state dir) (fun () -> (stmt,state))
  | OptionDirectional(_,None) -> (stmt,state)
  | OptionDirectional(_,Some dir) -> require T_Dir (type_value state dir) (fun () -> (stmt,state))
  | Targeting(_,dir,dis) -> 
    require T_Dir (type_value state dir) (fun () -> 
      require T_Int (type_value state dis) (fun () -> (stmt,state))
    )
  | DeclareAssign(Some t,n,v) -> require t (type_value state v) (fun () -> (stmt, {state with vars = Var(t,n)::state.vars}))
  | DeclareAssign(None, n, v) -> (
    let typ = type_value state v in
    (DeclareAssign(Some typ, n, v), {state with vars = Var(typ,n)::state.vars})
  )
  | Declare(t,n) -> (stmt, {state with vars = Var(t,n)::state.vars})
  | GoTo _
  | Label _
  | Continue
  | Break
  | Unit _ -> (stmt, state)
  | PagerSet v
  | PagerWrite v
  | Write v -> require T_Int (type_value state v) (fun () -> (stmt, state))
  | Say v -> require T_Int (type_value state v) (fun () -> (stmt, state))
  | Return v -> ( match state.ret_type with
      | None -> raise_failure "Return statement outside function"
      | Some ret_type -> 
        let v_type = type_value state v in
        if not(type_eq ret_type v_type) then raise_failure ("Return type mismatch: expected '" ^type_string ret_type^ "', but got '" ^type_string v_type^ "'")
        else (stmt, state)
  )

and type_check_stmt regs (Stmt(stmt,ln)) = 
  try 
    let (stmt', state') = type_check_stmt_inner regs stmt in
    (Stmt(stmt', ln), state')
  with 
  | Failure(p, None, msg) -> raise (Failure(p, Some ln, msg))
  | e -> raise e

and type_check_stmts state stmts =
  let (state, stmts) = List.fold_left (fun (state, stmts) stmt -> 
    let (stmt', state') = type_check_stmt state stmt in 
    (state', stmt'::stmts)
  ) (state,[]) stmts 
  in
  (state, List.rev stmts)

let type_check_program (File(vars,prog)) =
  let (_, prog') = type_check_stmts {vars = vars; labels = available_labels (Stmt(Block prog,0)); break = None; continue = None; ret_type = None;} prog in File(vars,prog')