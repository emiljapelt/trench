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
    | ("!", Int i) -> Int(if i = 0 then 1 else 0)
    | _ -> Unary_op(op, opte)
  )
  | Scan(d,p) -> Scan(optimize_value d, optimize_value p) 
  | Look _
  | Direction _
  | Random
  | RandomSet _
  | Int _ -> expr
  | MetaReference _
  | FieldProp _ 
  | Decrement _ 
  | Increment _
  | PagerRead
  | Read
  | Reference Local _ -> expr
  | Reference(Array(_,_)) -> expr
  | Func(ret,args,body) -> Func(ret,args,optimize_stmt body)
  | Call(f,args) -> Call(optimize_value f, List.map optimize_value args)

and optimize_assign_target tar = match tar with
  | _ -> tar

and optimize_stmt (Stmt(stmt_i,ln) as stmt) = 
  try (match stmt_i with
  | If(c,a,b) -> ( match optimize_value c with
    | Int 0 -> optimize_stmt b
    | Int _ -> optimize_stmt a
    | o -> Stmt(If(o, optimize_stmt a, optimize_stmt b),ln)
  )
  | IfIs(v,alts,opt) -> Stmt(IfIs(optimize_value v, List.map (fun (v,s) -> (optimize_value v, optimize_stmt s)) alts, Option.map optimize_stmt opt),ln)
  | Block stmts -> Stmt(Block(optimize_stmts stmts),ln)
  | While(v,s,None) -> Stmt(While(optimize_value v,optimize_stmt s,None),ln)
  | While(v,s,Some si) -> Stmt(While(optimize_value v,optimize_stmt s,Some(optimize_stmt si)),ln)
  | Assign(n,e) -> Stmt(Assign(optimize_assign_target n,optimize_value e),ln)
  | DeclareAssign(ty,n,v) -> Stmt(DeclareAssign(ty,n,optimize_value v), ln)
  | PagerSet v -> Stmt(PagerSet(optimize_value v),ln)
  | PagerWrite v -> Stmt(PagerWrite(optimize_value v),ln)
  | Say v -> Stmt(Say(optimize_value v),ln)
  | Write v -> Stmt(Write(optimize_value v),ln)
  | Directional(stmt,d) -> Stmt(Directional(stmt,optimize_value d),ln)
  | OptionDirectional(stmt,d) -> Stmt(OptionDirectional(stmt,Option.map optimize_value d),ln)
  | Targeting(stmt,dir,dis) -> Stmt(Targeting(stmt,optimize_value dir, optimize_value dis),ln)
  | Return v -> Stmt(Return(optimize_value v),ln)
  | Declare _
  | GoTo _
  | Label _
  | Continue
  | Break
  | Unit _ -> stmt)
  with
  | Failure (f, None, msg) -> raise (Failure(f,Some ln,msg))
  | e -> raise e

and optimize_stmts stmts =
  List.map optimize_stmt stmts

let optimize_program (File(regs,prog)) =
  File(regs,optimize_stmts prog)