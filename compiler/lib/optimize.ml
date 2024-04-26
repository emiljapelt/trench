open Absyn
open Exceptions

let rec optimize_expr expr var_env =
  match expr with
  | Reference _ -> expr
  | Value val_expr -> optimize_value val_expr var_env

and optimize_value expr var_env =
  match expr with
  | Binary_op (op, e1, e2) -> ( 
    let opte1 = optimize_expr e1 var_env in
    let opte2 = optimize_expr e2 var_env in
    match op, opte1, opte2 with
    (*| "&&", Value(Bool b1), Value(Bool b2) -> Value(Bool(b1&&b2))
    | "&&", Value(Bool true), _ -> opte2
    | "&&", _, Value(Bool true) -> opte1
    | "&&", Value(Bool false), _ -> Value(Bool false)
    | "&&", _, Value(Bool false) -> Value(Bool false)
    | "||", Value(Bool b1), Value(Bool b2) -> Value(Bool(b1||b2))
    | "||", Value(Bool true), _ -> Value(Bool true)
    | "||", _, Value(Bool true) -> Value(Bool true)
    | "||", Value(Bool false), _ -> opte2
    | "||", _, Value(Bool false) -> opte1*)
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
    let opte = optimize_expr e var_env in
    match (op, opte) with
    | ("!", Value(Int i)) -> Value(Int (if i = 0 then 1 else 0))
    | _ -> Value(Unary_op(op, opte))
  )
  | Int _ -> Value(expr)
