

type direction =
    | North
    | East
    | South
    | West

and flag =
    | PLAYER
    | TRENCH
    | TRAPPED
    | OBSTRUCTION

and typ =
    | T_Int
    | T_Dir
    | T_Field

and statement =
    | Stmt of statement_inner * int

and target =
    | Local of string

and statement_inner =
    | If of value * statement * statement
    | IfIs of value * (value * statement) list * statement option
    | Block of statement list
    | While of value * statement * statement option
    | Continue
    | Break
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
    | Declare of typ * string
    | DeclareAssign of typ * string * value
    | Write of value

and value =
    | Reference of target
    | MetaReference of meta_data
    | Increment of target * bool
    | Decrement of target * bool
    | Binary_op of string * value * value
    | Unary_op of string * value
    | Int of int
    | Scan of value * value
    | Look of value * flag
    | Direction of direction
    | Random
    | RandomSet of value list
    | Flag of value * flag
    | Read

and meta_data =
    | PlayerX     (* #x *)
    | PlayerY     (* #y *)  
    | PlayerResource of string
    | BoardX      (* #board_x *)  
    | BoardY      (* #board_y *)

and variable =
    | Var of typ * string

and file = 
    | File of variable list * statement list

type compile_state = {
    vars: variable list;
    break: string option;
    continue: string option;
}

let flag_index f = match f with
    | OBSTRUCTION -> 0
    | TRENCH -> 1
    | PLAYER -> 2
    | TRAPPED -> 3

let string_of_dir d = match d with
    | North -> "0"
    | East -> "1"
    | South -> "2"
    | West -> "3"

let int_of_dir d = match d with
    | North -> 0
    | East -> 1
    | South -> 2
    | West -> 3

let type_index t = match t with
    | T_Int -> 0
    | T_Dir -> 1
    | T_Field -> 2

type player_info_field =
    | PlayerTeam of int
    | PlayerName of string
    | PlayerPosition of int * int
    | PlayerFile of string

type player_info = PI of { team: int; name: string; position: int * int; file: string }

type resource = string * int

type exec_mode =
    | AsyncExec
    | SyncExec

type game_setup_part =
    | Player of player_info
    | Resources of resource list
    | Themes of string list
    | Actions of int
    | Steps of int
    | Mode of int
    | Board of int * int
    | Nuke of int
    | FeatureLevel of int
    | ExecMode of exec_mode
    | Seed of int option
    | TimeScale of float

type game_setup = GS of {
    players: player_info list;
    resources: resource list;
    themes: string list;
    actions: int;
    steps: int;
    mode: int;
    board: int * int;
    nuke: int;
    feature_level: int;
    exec_mode: exec_mode;
    seed: int option;
    time_scale: float;
}