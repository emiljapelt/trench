

type direction =
    | North
    | East
    | South
    | West

and statement =
    | Stmt of statement_inner * int

and statement_inner =
    | If of expression * statement * statement
    (*| While of expression * statement * statement option (* condition, body, incrementer *)*)
    | Block of statement list
    | Repeat of int * statement
    | Assign of string * expression
    | Label of string
    | Move of direction
    | Expand of direction
    | Shoot of direction
    | Bomb of direction * expression
    | Fortify
    | Trench
    | Wait
    | Pass
    | GoTo of string
    
and expression =
    | Reference of string
    | Value of value
    (*| Ternary of expression * expression * expression *)

and value =
    | Binary_op of string * expression * expression
    | Unary_op of string * expression
    | Int of int
    | Check of direction
    | Scan of direction

and register =
    | Register of bool * string * value

and file = 
    | File of register list * statement list