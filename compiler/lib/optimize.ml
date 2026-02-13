open Absyn
open Exceptions

let rec optimize_expr expr =
  match expr with
  | Binary_op (op, e1, e2) -> ( 
    let Expr(opte1,info1) = optimize_expression e1 in
    let Expr(opte2,info2) = optimize_expression e2 in
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
    (*| "+", Reference r1, Reference r2 when r1 = r2 -> Binary_op("*", Expr(Int 2, info1), Expr(opte1, info2))
    | "+", Reference r1, Binary_op("*", Int i, Reference r2) when r2 = r1 -> Binary_op("*", Int (i+1), Reference r1)
    | "+", Reference r1, Binary_op("*", Reference r2, Int i) when r2 = r1 -> Binary_op("*", Int (i+1), Reference r1)
    | "+", Binary_op("*", Int i, Reference r2), Reference r1 when r2 = r1 -> Binary_op("*", Int (i+1), Reference r1)
    | "+", Binary_op("*", Reference r2, Int i), Reference r1 when r2 = r1 -> Binary_op("*", Int (i+1), Reference r1)*)
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
    | _ -> Binary_op(op, Expr(opte1,info1), Expr(opte2,info2))
  )
  | Unary_op (op, e) -> ( 
    let Expr(opte,info) = optimize_expression e in
    match (op, opte) with
    | ("!", Int i) -> Int(if i > 0 then 0 else 1)
    | _ -> Unary_op(op, Expr(opte, info))
  )
  | Direction _
  | Random
  | Int _
  | Prop _
  | Null
  | Resource _
  | Decrement _ 
  | Increment _
  | VarAccess _
  | ArrayAccess(_,_) -> expr
  | RandomSet exprs -> RandomSet(List.map optimize_expression exprs)
  | ArrayLiteral exprs -> ArrayLiteral(List.map optimize_expression exprs)
  | Func(ret,args,body) -> Func(ret,args,optimize_stmt body)
  | Call(f,args) -> Call(optimize_expression f, List.map optimize_expression args)
  | Ternary(c,a,b) -> 
    let Expr(c_opt, c_info) = optimize_expression c in
    let Expr(a_opt, a_info) = optimize_expression a in
    let Expr(b_opt, b_info) = optimize_expression b in
    match c_opt with
    | Int i when i > 0 -> a_opt
    | Int _ -> b_opt
    | _ -> Ternary(Expr(c_opt, c_info), Expr(a_opt, a_info), Expr(b_opt, b_info))

and optimize_expression e : int expression = match e with
    | Expr(e,i) -> Expr(optimize_expr e, i)

and optimize_assign_target tar = match tar with
  | _ -> tar

and optimize_stmt (Stmt(stmt_i,ln) as stmt) = 
  try (match stmt_i with
  | If(c,a,b) -> Stmt(If(optimize_expression c, optimize_stmt a, optimize_stmt b),ln)
  | IfIs(v,alts,opt) -> Stmt(IfIs(optimize_expression v, List.map (fun (v,s) -> (optimize_expression v, optimize_stmt s)) alts, Option.map optimize_stmt opt),ln)
  | Block stmts -> Stmt(Block(optimize_stmts stmts),ln)
  | While(v,s,None) -> Stmt(While(optimize_expression v,optimize_stmt s,None),ln)
  | While(v,s,Some si) -> Stmt(While(optimize_expression v,optimize_stmt s,Some(optimize_stmt si)),ln)
  | Assign(n,e) -> Stmt(Assign(optimize_assign_target n,optimize_expression e),ln)
  | DeclareAssign(ty,n,v) -> Stmt(DeclareAssign(ty,n,optimize_expression v), ln)
  | Return v -> Stmt(Return(optimize_expression v),ln)
  | ExprStmt e -> Stmt(ExprStmt(optimize_expression e),ln)
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

let optimize_program (File(prog,i)) =
  File(optimize_stmts prog, i)