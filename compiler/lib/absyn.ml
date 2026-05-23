open Helpers
open Resources
open ProgramRep

type direction =
    | North
    | East
    | South
    | West

type scope =
    | LocalScope
    | GlobalScope

type field_prop = 
    | Obstruction_Prop
    | Player_Prop
    | Trapped_Prop
    | Flammable_Prop
    | Cover_Prop
    | Shelter_Prop
    | Meltable_Prop
    | IsEmpty_Prop
    | IsTrench_Prop
    | IsIceBlock_Prop
    | IsTree_Prop
    | IsOcean_Prop
    | IsWall_Prop
    | IsBridge_Prop
    | IsClay_Prop
    | IsMineShaft_Prop
    | IsMountain_Prop
    | Enemy_Prop
    | Ally_Prop
    | Vehicle_Prop

let prop_index p = match p with
  | Obstruction_Prop ->     0b00000000000000000000000000000001
  | Player_Prop ->          0b00000000000000000000000000000010
  | Trapped_Prop ->         0b00000000000000000000000000000100
  | Flammable_Prop ->       0b00000000000000000000000000001000
  | Cover_Prop ->           0b00000000000000000000000000010000
  | Shelter_Prop ->         0b00000000000000000000000000100000
  | Meltable_Prop ->        0b00000000000000000000000001000000
  | IsEmpty_Prop ->         0b00000000000000000000000010000000
  | IsTrench_Prop ->        0b00000000000000000000000100000000
  | IsIceBlock_Prop ->      0b00000000000000000000001000000000
  | IsTree_Prop ->          0b00000000000000000000010000000000
  | IsOcean_Prop ->         0b00000000000000000000100000000000
  | IsWall_Prop ->          0b00000000000000000001000000000000
  | IsBridge_Prop ->        0b00000000000000000010000000000000
  | IsClay_Prop ->          0b00000000000000000100000000000000
  | IsMineShaft_Prop ->     0b00000000000000001000000000000000
  | IsMountain_Prop ->      0b00000000000000010000000000000000
  | Enemy_Prop ->           0b00000000000000100000000000000000
  | Ally_Prop ->            0b00000000000001000000000000000000
  | Vehicle_Prop ->         0b00000000000010000000000000000000

module FieldProp = struct
  type t = field_prop
  let compare = fun a b -> Int.compare (prop_index a) (prop_index b)
end
module FieldPropSet = Set.Make(FieldProp)

type field = FieldPropSet.t

let field_value = FieldPropSet.to_list >> List.map prop_index >> List.fold_left (+) 0

type typ =
    | T_Int
    | T_Dir
    | T_Field
    | T_Resource
    | T_Array of typ * int
    | T_Tuple of (typ * string option) list
    | T_Func of typ * typ list
    | T_Null

and typ_expr =
    | TE_Int
    | TE_Dir
    | TE_Field
    | TE_Resource
    | TE_Array of typ_expr * expression
    | TE_Tuple of (typ_expr * string option) list
    | TE_Func of typ_expr * typ_expr list

and  statement =
    | Stmt of stmt * int

and stmt =
    | If of  expression *  statement *  statement
    | IfIs of  expression * ( expression *  statement) list *  statement option
    | Block of  statement list
    | While of  expression *  statement *  statement option
    | Continue
    | Break
    | Assign of  expression *  expression
    | Label of string
    | GoTo of string
    | Declare of typ_expr * string
    | DeclareAssign of typ_expr option * string * expression
    | DeclareConst of string * expression
    | Return of expression
    | ExprStmt of expression
    | Repeat of expression option * statement

and expression =
    | Expr of expr * int

and expr =
    | IdentifierAccess of string
    | IndexAccess of expression * range
    | TupleAccess of expression * string
    | Resource of resource
    | Increment of expression * bool
    | Decrement of expression * bool
    | Binary_op of binop * expression * expression
    | Unary_op of unop * expression
    | Int of int
    | Field of field
    | Direction of direction
    | Random
    | RandomAccess of expression
    | Func of func
    | Call of expression *  expression list
    | Ternary of expression * expression * expression
    | Null
    | StructureLiteral of structure_element list
    | SizeOf of expression
    | ASM of typ * instruction list

and structure_element =
    | StructureElement of string option * expression
    | SpreadElement of expression

and func = {
    data : typ_expr * (typ_expr * string) list * statement;
    mutable cache : (typ*string) option;
}

and range =
    | Index of expression
    | Range of expression option * expression option

and binop =
    | Plus
    | Minus
    | Times
    | And
    | Or
    | Equal
    | NotEqual
    | Less
    | LessOrEqual
    | Greater
    | GreaterOrEqual
    | Divide
    | Remainder
    | RightShift
    | LeftShift
    | IsCompare
    | AnyCompare

and unop =
    | Negate

and identifier =
    | Var of typ * string
    | Const of typ * string * expression (* typ needed ??? *)

and  file = 
    | File of statement list * int

type scopes = {
    local: identifier list;
    global: identifier list option;
}

type compile_state = {
    scopes: scopes;
    labels: StringSet.t;
    break: string option;
    continue: string option;
    ret_type: typ option;
    size: int
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


let rec type_string t = 
    let entry_string typ name = type_string typ ^ match name with
    | None -> ""
    | Some n -> " "^n
    in    
    match t with
    | T_Int -> "int"
    | T_Dir -> "dir"
    | T_Field -> "field"
    | T_Resource -> "resource"
    | T_Array(t,i) -> (type_string t) ^ "[" ^ string_of_int i ^"]"
    | T_Tuple(ts) -> "[" ^ (ts |> List.map (uncurry entry_string) |> String.concat ", ")  ^ "]"
    | T_Func(r,args) -> (type_string r) ^ "(" ^ (args |> List.map type_string |> String.concat ", ")  ^ ")"
    | T_Null -> "null"

let rec type_size t = match t with
    | T_Int 
    | T_Dir 
    | T_Resource
    | T_Func _
    | T_Null
    | T_Field -> 1
    | T_Array(t,s) -> s * (type_size t)
    | T_Tuple(ts) -> List.fold_left (fun acc (t, _) -> acc + type_size t) 0 ts

let rec type_eq t1 t2 = match t1,t2 with
  | T_Int, T_Int
  | T_Dir, T_Dir
  | T_Resource, T_Resource
  | T_Field, T_Field -> true
  | T_Array(st1,size1), T_Array(st2,size2) -> size1 = size2 && type_eq st1 st2
  | T_Tuple(ts1), T_Tuple(ts2) -> List.length ts1 = List.length ts2 && List.combine ts1 ts2 |> List.for_all (fun ((t1,_),(t2,_)) -> type_eq t1 t2)
  | T_Func(ret, params), T_Func(ret', params') -> 
    List.length params = List.length params' 
    && List.combine params params' |> List.for_all (fun (p,p') -> type_eq p p')
    && type_eq ret ret'  
  | _ -> false

let type_list_eq tl1 tl2 = 
  List.length tl1 == List.length tl2 && List.for_all (fun (a,b) -> type_eq a b) (List.combine tl1 tl2)

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

type exec_mode =
    | DefaultExec

type map =
    | EmptyMap of int * int
    | FileMap of string * (int * int)

type setting = (string * int)

type string_set_part =
    | Add of string
    | Remove of string
    | All

type game_setup_part =
    | Team of team_info
    | Resources of (string * (int * int)) list
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
    | SettingOverwrites of setting list
    | Debug of bool
    | Viewport of int * int
    | AutoStart of bool
    | AutoResize of bool

type game_setup = GS of {
    teams: team_info list;
    resources: (int*int) ResourceMap.t;
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
    setting_overwrites: setting list;
    debug: bool;
    viewport: int * int;
    auto_start: bool;
}

let resolve_string_set all parts =
    let rec aux ps acc = match ps with
        | [] -> acc
        | Add n :: t -> aux t (StringSet.add n acc)
        | Remove n :: t -> aux t (StringSet.remove n acc)
        | All :: t -> aux t (StringSet.union all acc)
    in
    aux parts StringSet.empty

let available_labels stmt =
  let rec aux (Stmt(stmt,_)) set = match stmt with 
    | Label n -> StringSet.union set (StringSet.singleton n)
    | If(_,t,f) -> StringSet.union (aux t set) (aux f set)
    | IfIs(_,alts,Some el) -> List.fold_left (fun acc (_,s) -> aux s acc) (aux el set) alts 
    | IfIs(_,alts,None) -> List.fold_left (fun acc (_,s) -> aux s acc) set alts 
    | Block(stmts) -> List.fold_left (fun acc s -> aux s acc) set stmts
    | While(_,stmt,_) -> aux stmt set
    | _ -> set
  in
  aux stmt StringSet.empty

let identifier_name id = match id with
  | Var(_,n) 
  | Const(_,n,_) -> n

let is_bound name scopes =
  match List.find_opt (fun id -> identifier_name id = name) scopes.local with
  | Some _ -> true
  | None -> (
    match Option.map (fun scope -> List.find_opt (fun id -> identifier_name id = name) scope) scopes.global |> Option.join with
    | Some _ -> true
    | _ -> false
  )