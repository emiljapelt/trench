module StringSet = Set.Make(String)

type direction =
    | North
    | East
    | South
    | West

type field_prop = 
    | Obstruction_Prop
    | Player_Prop
    | Trapped_Prop
    | Flammable_Prop
    | Cover_Prop
    | Shelter_Prop
    | IsEmpty_Prop
    | IsTrench_Prop
    | IsIceBlock_Prop
    | IsTree_Prop
    | IsOcean_Prop
    | IsWall_Prop
    | IsBridge_Prop

and typ =
    | T_Int
    | T_Dir
    | T_Field
    | T_Array of typ * int
    | T_Func of typ * typ list

and statement =
    | Stmt of statement_inner * int

and target =
    | Local of string
    | Array of target * value

and statement_inner =
    | If of value * statement * statement
    | IfIs of value * (value * statement) list * statement option
    | Block of statement list
    | While of value * statement * statement option
    | Continue
    | Break
    | Assign of target * value
    | Label of string
    | GoTo of string
    | Declare of typ * string
    | DeclareAssign of typ option * string * value
    | Write of value
    | PagerSet of value
    | PagerWrite of value
    | Unit of unit_statement
    | Directional of directional_statement * value
    | OptionDirectional of option_directional_statement * value option
    | Targeting of targeting_statement * value * value
    | Say of value
    | Return of value

and unit_statement =
    | Wait
    | Pass
    | Projection
    | Meditate

and directional_statement =
    | Shoot
    | Mine
    | Fireball
    | Move
    | Chop
    | Dispel
    | Disarm
    | ManaDrain
    | Wall
    | Bridge
    | PlantTree
    | Mount
    | Dismount
    | Boat
    | BearTrap

and option_directional_statement =
    | Trench
    | Fortify
    | Collect

and targeting_statement =
    | Bomb
    | Freeze

and value =
    | Reference of target
    | MetaReference of meta_data
    | Increment of target * bool
    | Decrement of target * bool
    | Binary_op of string * value * value
    | Unary_op of string * value
    | Int of int
    | Scan of value * value
    | Look of value * field_prop
    | Direction of direction
    | Random
    | RandomSet of value list
    | FieldProp of value * field_prop
    | Read
    | PagerRead
    | Func of typ * (typ * string) list * statement
    | Call of value * value list
    | Ternary of value * value * value

and meta_data =
    | PlayerX     (* #x *)
    | PlayerY     (* #y *)  
    | PlayerResource of string
    | BoardX      (* #board_x *)  
    | BoardY      (* #board_y *)
    | PlayerID

and variable =
    | Var of typ * string

and file = 
    | File of variable list * statement list

type compile_state = {
    vars: variable list;
    labels: StringSet.t;
    break: string option;
    continue: string option;
    ret_type: typ option;
}

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

let rec type_size t = match t with
    | T_Int 
    | T_Dir 
    | T_Func _
    | T_Field -> 1
    | T_Array(t,s) -> s * (type_size t)

type player_info_field =
    | PlayerName of string
    | PlayerOrigin of int * int
    | PlayerFiles of string list

type team_info_field =
    | TeamName of string
    | TeamColor of int * int * int
    | TeamOrigin of int * int
    | TeamPlayer of player_info

and player_info = PI of { team: int; name: string; origin: int * int; files: string list }
type team_info = TI of { name: string; color: (int*int*int); origin: int * int; players: player_info list }

type resource = string * (int * int)

type exec_mode =
    | AsyncExec
    | SyncExec

type map =
    | EmptyMap of int * int
    | FileMap of string * (int * int)

type setting_overwrites = string * (string * int) list

type game_setup_part =
    | Team of team_info
    | Resources of resource list
    | Themes of StringSet.t
    | Features of StringSet.t
    | Actions of int
    | Steps of int
    | Mode of int
    | Nuke of int
    | ExecMode of exec_mode
    | Seed of int option
    | TimeScale of float
    | Map of map
    | SettingOverwrites of setting_overwrites list
    | Debug of bool

type game_setup = GS of {
    teams: team_info list;
    resources: resource list;
    themes: StringSet.t;
    features: StringSet.t;
    actions: int;
    steps: int;
    mode: int;
    nuke: int;
    exec_mode: exec_mode;
    seed: int option;
    time_scale: float;
    map: map;
    setting_overwrites: setting_overwrites list;
    debug: bool;
}