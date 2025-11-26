open Absyn
open Exceptions

let rec optimize_value expr =
  match expr with
  | Binary_op (op, e1, e2) -> ( 
    let opte1 = optimize_value e1 in
    let opte2 = optimize_value e2 in
    match op, opte1, opte2 with
    | "&", Int i1, Int i2 -> Int(i1*i2)
    | "&", Int i, _ when i <> 0 -> opte2
    | "&", _, Int i when i <> 0 -> opte1
    | "&", Int 0, _
    | "&", _, Int 0 -> Int 0
    | "|", Int i1, Int i2 -> Int(i1+i2)
    | "|", Int i, _
    | "|", _, Int i when i <> 0 -> Int 1
    | "|", Int 0, _ -> opte2
    | "|", _, Int 0 -> opte1
    | "+", Int i1, Int i2 -> Int (i1+i2)
    | "+", Int 0, _ -> opte2
    | "+", _, Int 0 -> opte1
    | "-", Int 0, Int i -> Int (-i)
    | "-", Int i1, Int i2 -> Int (i1-i2)
    | "-", _, Int 0 -> opte1
    | "*", Int i1, Int i2 -> Int (i1*i2)
    | "*", Int 0, _ -> Int 0
    | "*", _, Int 0 -> Int 0
    | "*", Int 1, _ -> opte2
    | "*", _, Int 1 -> opte1
    | "+", Reference r1, Reference r2 when r1 = r2 -> Binary_op("*", Int 2, opte1)
    | "+", Reference r1, Binary_op("*", Int i, Reference r2) when r2 = r1 -> Binary_op("*", Int (i+1), Reference r1)
    | "+", Reference r1, Binary_op("*", Reference r2, Int i) when r2 = r1 -> Binary_op("*", Int (i+1), Reference r1)
    | "+", Binary_op("*", Int i, Reference r2), Reference r1 when r2 = r1 -> Binary_op("*", Int (i+1), Reference r1)
    | "+", Binary_op("*", Reference r2, Int i), Reference r1 when r2 = r1 -> Binary_op("*", Int (i+1), Reference r1)
    | "/", Int _, Int 0 -> raise_failure "Division by zero"
    | "/", Int i1, Int 1 -> Int (i1)
    | "/", Int i1, Int i2 -> Int (i1/i2)
    | "%", Int _, Int 0 -> raise_failure "Division by zero"
    | "%", Int i1, Int 1 -> Int (i1)
    | "%", Int i1, Int i2 -> 
      if i2 <= 0 then raise_failure "Right hand side of modolo operator, must be positive"
      else Int (((i1 mod i2) + i2) mod i2)
    | "=", Int i1, Int i2 -> Int (if i1=i2 then 1 else 0)
    | "!=", Int i1, Int i2 -> Int (if i1!=i2 then 1 else 0)
    | "<", Int i1, Int i2 -> Int (if i1<i2 then 1 else 0)
    | "<=", Int i1, Int i2 -> Int (if i1<=i2 then 1 else 0)
    | ">", Int i1, Int i2 -> Int (if i1>i2 then 1 else 0)
    | ">=", Int i1, Int i2 -> Int (if i1>=i2 then 1 else 0)
    | _ -> Binary_op(op, opte1, opte2)
  )
  | Unary_op (op, e) -> ( 
    let opte = optimize_value e in
    match (op, opte) with
    | ("!", Int i) -> Int(if i > 0 then 0 else 1)
    | _ -> Unary_op(op, opte)
  )
  | Direction _
  | Random
  | RandomSet _
  | Int _
  | Prop _
  | Resource _
  | FieldProp _ 
  | Decrement _ 
  | Increment _
  | Reference Local _ -> expr
  | Reference(Array(_,_)) -> expr
  | Func(ret,args,body) -> Func(ret,args,optimize_stmt body)
  | Call(f,args) -> Call(optimize_value f, List.map optimize_value args)
  | Ternary(c,a,b) -> 
    let c_opt = optimize_value c in
    let a_opt = optimize_value a in
    let b_opt = optimize_value b in
    match c_opt with
    | Int i when i > 0 -> a_opt
    | Int _ -> b_opt
    | _ -> Ternary(c_opt, a_opt, b_opt)

and optimize_assign_target tar = match tar with
  | _ -> tar

and optimize_stmt (Stmt(stmt_i,ln) as stmt) = 
  try (match stmt_i with
  | If(c,a,b) -> ( match optimize_value c with
    | Int i when i > 0 -> optimize_stmt a
    | Int _ -> optimize_stmt b
    | o -> Stmt(If(o, optimize_stmt a, optimize_stmt b),ln)
  )
  | IfIs(v,alts,opt) -> Stmt(IfIs(optimize_value v, List.map (fun (v,s) -> (optimize_value v, optimize_stmt s)) alts, Option.map optimize_stmt opt),ln)
  | Block stmts -> Stmt(Block(optimize_stmts stmts),ln)
  | While(v,s,None) -> Stmt(While(optimize_value v,optimize_stmt s,None),ln)
  | While(v,s,Some si) -> Stmt(While(optimize_value v,optimize_stmt s,Some(optimize_stmt si)),ln)
  | Assign(n,e) -> Stmt(Assign(optimize_assign_target n,optimize_value e),ln)
  | DeclareAssign(ty,n,v) -> Stmt(DeclareAssign(ty,n,optimize_value v), ln)
  | Return v -> Stmt(Return(optimize_value v),ln)
  | CallStmt(f,args) -> Stmt(CallStmt(optimize_value f, List.map optimize_value args),ln)
  | Declare _
  | GoTo _
  | Label _
  | Continue
  | Break -> stmt)
  with
  | Failure (f, None, msg) -> raise (Failure(f,Some ln,msg))
  | e -> raise e

and optimize_stmts stmts =
  List.map optimize_stmt stmts

let optimize_program (File prog) =
  File(optimize_stmts prog)