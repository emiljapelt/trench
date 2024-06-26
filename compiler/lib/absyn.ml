

type direction =
    | North
    | East
    | South
    | West

and flag =
    | PLAYER
    | TRENCH
    | MINE
    | DESTROYED

and typ =
    | T_Int
    | T_Dir
    | T_Field

and statement =
    | Stmt of statement_inner * int

and target =
    | Local of string
    | Global of typ * value

and statement_inner =
    | If of value * statement * statement
    | Block of statement list
    | Repeat of int * statement
    | Assign of target * value
    | Label of string
    | Move of value
    | Shoot of value
    | Bomb of value * value
    | Mine of value
    | Attack of value
    | Fortify of value option
    | Trench of value option
    | Wait
    | Pass
    | GoTo of string

and value =
    | Reference of target
    | MetaReference of meta_data
    | Value of value
    | Binary_op of string * value * value
    | Unary_op of string * value
    | Int of int
    | Scan of value * value
    | Look of value
    | Direction of direction
    | Random
    | RandomSet of value list
    | Flag of value * flag

and meta_data =
    | PlayerX     (* #x *)
    | PlayerY     (* #y *)  
    | PlayerBombs (* #bombs *)
    | PlayerShots (* #shots *)
    | BoardX      (* #board_x *)  
    | BoardY      (* #board_y *)
    | GlobalArraySize (* #array_size  *)

and register =
    | Register of typ * string * value

and file = 
    | File of register list * statement list

let string_of_dir d = match d with
    | North -> "0"
    | East -> "1"
    | South -> "2"
    | West -> "3"

let type_index t = match t with
    | T_Int -> 0
    | T_Dir -> 1
    | T_Field -> 2

type player_info = PI of { id: int; pos: int * int; file: string }

type game_setup_part =
    | Player of player_info
    | Bombs of int
    | Shots of int
    | Actions of int
    | Steps of int
    | Mode of int
    | Board of int * int
    | Nuke of int
    | GlobalArray of int

type game_setup = GS of {
    players: player_info list;
    bombs: int;
    shots: int;
    actions: int;
    steps: int;
    mode: int;
    board: int * int;
    nuke: int;
    array: int;
}