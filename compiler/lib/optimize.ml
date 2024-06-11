open Absyn
open Exceptions

let rec optimize_value expr =
  match expr with
  | Binary_op (op, e1, e2) -> ( 
    let opte1 = optimize_value e1 in
    let opte2 = optimize_value e2 in
    match op, opte1, opte2 with
    | "&", Value(Int i1), Value(Int i2) -> Value(Int(i1*i2))
    | "&", Value(Int i), _ when i <> 0 -> opte2
    | "&", _, Value(Int i) when i <> 0 -> opte1
    | "&", Value(Int 0), _
    | "&", _, Value(Int 0) -> Value(Int 0)
    | "|", Value(Int i1), Value(Int i2) -> Value(Int(i1+i2))
    | "|", Value(Int i), _
    | "|", _, Value(Int i) when i <> 0 -> Value(Int 1)
    | "|", Value(Int 0), _ -> opte2
    | "|", _, Value(Int 0) -> opte1
    | "+", Value(Int i1), Value(Int i2) -> Value(Int (i1+i2))
    | "+", Value(Int 0), _ -> opte2
    | "+", _, Value(Int 0) -> opte1
    | "-", Value(Int 0), Value(Int i) -> Value(Int (-i))
    | "-", Value(Int i1), Value(Int i2) -> Value(Int (i1-i2))
    | "-", _, Value(Int 0) -> opte1
    | "*", Value(Int i1), Value(Int i2) -> Value(Int (i1*i2))
    | "*", Value(Int 0), _ -> Value(Int 0)
    | "*", _, Value(Int 0) -> Value(Int 0)
    | "*", Value(Int 1), _ -> opte2
    | "*", _, Value(Int 1) -> opte1
    | "+", Reference r1, Reference r2 when r1 = r2 -> Value(Binary_op("*", Value(Int 2), opte1))
    | "+", Reference r1, Value(Binary_op("*", Value(Int i), Reference r2)) when r2 = r1 -> Value(Binary_op("*", Value(Int (i+1)), Reference r1))
    | "+", Reference r1, Value(Binary_op("*", Reference r2, Value(Int i))) when r2 = r1 -> Value(Binary_op("*", Value(Int (i+1)), Reference r1))
    | "+", Value(Binary_op("*", Value(Int i), Reference r2)), Reference r1 when r2 = r1 -> Value(Binary_op("*", Value(Int (i+1)), Reference r1))
    | "+", Value(Binary_op("*", Reference r2, Value(Int i))), Reference r1 when r2 = r1 -> Value(Binary_op("*", Value(Int (i+1)), Reference r1))
    | "/", Value(Int _), Value(Int 0) -> raise_failure "Division by zero"
    | "/", Value(Int i1), Value(Int 1) -> Value(Int (i1))
    | "/", Value(Int i1), Value(Int i2) -> Value(Int (i1/i2))
    | "%", Value(Int _), Value(Int 0) -> raise_failure "Division by zero"
    | "%", Value(Int i1), Value(Int 1) -> Value(Int (i1))
    | "%", Value(Int i1), Value(Int i2) -> 
      if i2 <= 0 then raise_failure "Right hand side of modolo operator, must be positive"
      else Value(Int (((i1 mod i2) + i2) mod i2))
    | "=", Value(Int i1), Value(Int i2) -> Value(Int (if i1=i2 then 1 else 0))
    | "!=", Value(Int i1), Value(Int i2) -> Value(Int (if i1!=i2 then 1 else 0))
    | "<", Value(Int i1), Value(Int i2) -> Value(Int (if i1<i2 then 1 else 0))
    | "<=", Value(Int i1), Value(Int i2) -> Value(Int (if i1<=i2 then 1 else 0))
    | ">", Value(Int i1), Value(Int i2) -> Value(Int (if i1>i2 then 1 else 0))
    | ">=", Value(Int i1), Value(Int i2) -> Value(Int (if i1>=i2 then 1 else 0))
    | _ -> Value(Binary_op(op, opte1, opte2))
  )
  | Unary_op (op, e) -> ( 
    let opte = optimize_value e in
    match (op, opte) with
    | ("!", Value(Int i)) -> Value(Int (if i = 0 then 1 else 0))
    | _ -> Value(Unary_op(op, opte))
  )
  | Scan(d,p) -> Scan(optimize_value d, optimize_value p) 
  | Look _
  | Direction _
  | Random
  | RandomSet _
  | Int _ -> Value(expr)
  | MetaReference _
  | Flag _ 
  | Reference Local _ -> expr
  | Reference Global(t,v) -> Reference(Global(t,optimize_value v))
  | Value val_expr -> optimize_value val_expr


let optimize_assign_target tar = match tar with
      | Local _ -> tar
      | Global(t,v) -> Global(t,optimize_value v)

let rec optimize_stmt (Stmt(stmt_i,ln) as stmt) = match stmt_i with
  | If(c,a,b) -> ( match optimize_value c with
    | Value(Int 0) -> optimize_stmt b
    | Value(Int _) -> optimize_stmt a
    | o -> Stmt(If(o, optimize_stmt a, optimize_stmt b),ln)
  )
  | Block stmts -> Stmt(Block(optimize_stmts stmts),ln)
  | Repeat(i,stmt) -> Stmt(Repeat(i,optimize_stmt stmt),ln)
  | Assign(n,e) -> Stmt(Assign(optimize_assign_target n,optimize_value e),ln)
  | Move e -> Stmt(Move(optimize_value e),ln)
  | Shoot e -> Stmt(Shoot(optimize_value e),ln)
  | Bomb(d,p) -> Stmt(Bomb(optimize_value d, optimize_value p),ln)
  | Mine d -> Stmt(Mine(optimize_value d),ln)
  | Attack d -> Stmt(Attack(optimize_value d),ln)
  | Fortify o -> Stmt(Fortify(Option.map optimize_value o),ln)
  | Trench o -> Stmt(Trench(Option.map optimize_value o),ln)
  | Wait
  | GoTo _
  | Label _
  | Pass -> stmt

and optimize_stmts stmts =
  List.map optimize_stmt stmts

let optimize_program (File(regs,prog)) =
  File(regs,optimize_stmts prog)