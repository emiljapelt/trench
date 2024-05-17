

type direction =
    | North
    | East
    | South
    | West

and typ =
    | T_Int
    | T_Dir

and statement =
    | Stmt of statement_inner * int

and statement_inner =
    | If of value * statement * statement
    | Block of statement list
    | Repeat of int * statement
    | Assign of string * value
    | Label of string
    | Move of value
    | Shoot of value
    | Bomb of value * value
    | Fortify of value option
    | Trench of value option
    | Wait
    | Pass
    | GoTo of string

and value =
    | Reference of string
    | MetaReference of meta_data
    | Value of value
    | Binary_op of string * value * value
    | Unary_op of string * value
    | Int of int
    | Check of value
    | Scan of value
    | Direction of direction
    | Random
    | RandomSet of value list

and meta_data =
    | PlayerX     (* #x *)
    | PlayerY     (* #y *)  
    | PlayerBombs (* #bombs *)
    | PlayerShots (* #shots *)
    | BoardX      (* #board_x *)  
    | BoardY      (* #board_y *)  

and register =
    | Register of typ * string * value

and file = 
    | File of register list * statement list

let string_of_dir d = match d with
    | North -> "0"
    | East -> "1"
    | South -> "2"
    | West -> "3"