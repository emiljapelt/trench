open Exceptions
open Trg
open ToProgramRep
open ProgramRep
open Absyn
open Resources
open Helpers
open Builtins

let check_path path extensions =
  try (
    if not(Sys.file_exists path) then raise_failure ("File does not exist: " ^ path)
    else if not(extensions |> List.exists (fun ext -> String.ends_with ~suffix:ext path)) then raise_failure "Invalid file extension"
    else ()
  ) with
  | ex -> raise ex


let get_line ls l = match List.nth_opt ls l with
  | None -> ""
  | Some ln -> Printf.sprintf "%i | %s\n" l ln

let read_file path =
  let file = open_in path in
  let content = really_input_string (file) (in_channel_length file) in
  let () = close_in_noerr file in
  content

  (* Failures are getting the wrong file *)
let format_failure f = match f with
  | Failure(Some path,None,msg) -> Printf.sprintf "In %s: %s\n" path msg
  | Failure(Some path, Some line, msg) -> (
    let msg = Printf.sprintf "%s in %s" msg path in
    let details = 
        let line_msg = Printf.sprintf ", line %i: \n" line in
        let lines = String.split_on_char '\n' (read_file path) in
        let printer = get_line lines in 
        let is_first_line = line = 1 in
        let is_last_line = line = List.length lines in
        line_msg ^ match is_first_line, is_last_line with
        | true, true   -> printer 1
        | true, false  -> printer 1 ^ printer 2
        | false, true  -> printer (line-1) ^ printer line
        | false, false -> printer (line-1) ^ printer line ^ printer (line+1)
      in
      msg ^ details
  ) 
  | Failure(_,_,msg) -> msg ^ "\n"
  | _ -> "Uncaught error!"

let compress_path path =
  let rec compress parts acc =
    match parts with
    | [] -> List.rev acc
    | h::t when h = "." -> compress t (acc)
    | _::h2::t when h2 = ".." -> compress t acc
    | h::t -> compress t (h::acc) 
  in
  String.concat "/" (compress (String.split_on_char '/' path) [])

let total_path path =
  if path.[0] = '.' then Sys.getcwd () ^ "/" ^ path
  else path

let complete_path base path = compress_path (if path.[0] = '.' then (String.sub base 0 ((String.rindex base '/')+1) ^ path) else path)

(* If the path is relative, change it to be relative to game files directory *)
let fix_path file = 
  if (Filename.is_relative file) then Filename.concat compiler_notes.dir file
  else file


(* Get from game file fields entry, at some point *)
let valid_map_chars = ['\n'; '\r'; '.'; '+'; '~'; 'T'; 'w'; 'C'; 'M';]

let get_map_size map = match map with
  | EmptyMap(x,y)
  | FileMap(_,(x,y)) -> (x,y)

let check_map map = match map with
  | EmptyMap (x,y) -> if x > 0 || y > 0 then map else raise_failure "Map size cannot be negative"
  | FileMap(path, _) -> 
    check_path path [".trm"] ;
    let map_string = read_file path |> String.trim in
    if not(String.for_all (fun c -> List.mem c valid_map_chars) map_string) then raise_failure "Map contained invalid characters"
    else match String.split_on_char '\n' map_string with
    | [] | [""] -> raise_failure "Empty map file"
    | l::ls as lines -> (
      let x = String.length l in
      if not(ls |> List.for_all (fun line -> String.length line = x)) then raise_failure "Not all map lines were the same length"
      else FileMap(String.concat "" lines, (x, List.length lines))
    )

let load_mode = function
  | TRGInt i -> i
  | TRGString "inf" -> 0
  | trg -> expected "mode to be an int or \"inf\"" trg

let load_viewport = function
  | TRGArray[TRGInt w; TRGInt h] -> (w,h) 
  | trg -> expected "viewport to be an array of two ints" trg

let load_seed = function
  | TRGInt i -> Some i
  | TRGNull -> None
  | tn -> expected "seed to be an int" tn

let load_map tn = 
  let map = (match tn with
    | TRGArray[TRGInt w; TRGInt h] -> EmptyMap(w,h)
    | TRGString path -> FileMap(fix_path path, (-1,-1))
    | trg -> expected "map to be a string or an array of two ints" trg
  ) |> check_map
  in
  Flags.set_map_size (get_map_size map); map

let load_time_scale tn = 
  let scale = match tn with
    | TRGFloat f -> f
    | TRGInt i -> float_of_int i
    | _ -> expected "timescale to be an int or float" tn
  in
  if scale >= 0.0 then scale else expected "timescale to be non-negative" tn

(* How to remove a single feature... [*, -debug] *)
let load_features = function
  | TRGBool true -> Features.all_features
  | TRGBool false -> StringSet.empty
  | TRGArray fs -> fs |> List.filter is_string |> List.map load_string |> StringSet.of_list
  | _ -> raise_failure "Could not load feature set"

(* relevant after the rework? *)
let load_themes = function
  | TRGBool true -> Themes.all_themes
  | TRGBool false -> StringSet.empty
  | TRGArray fs -> fs |> List.filter is_string |> List.map load_string |> StringSet.of_list
  | _ -> raise_failure "Could not load feature set"

(* Load which resources actually exist from trg *)
(* Likely only doable when trg library is implemented... *)
let load_resource_info = function
  | TRGArray[TRGInt init; TRGInt max] -> (init, max)
  | TRGInt i -> (i, -1)
  | trg -> expected ("resource to be an int or an array of two ints") trg

let load_resource r trg = 
  (r, trg |> find_entry (resource_to_string r) ~default:(TRGArray[TRGInt(0); TRGInt(-1)]) |> load_resource_info)

let load_resources tn = [
  load_resource R_Explosive;
  load_resource R_Ammo;
  load_resource R_Mana;
  load_resource R_Sapling;
  load_resource R_Clay;
  load_resource R_Wood;
  load_resource R_BearTrap;
  load_resource R_Metal;
] 
|> List.map (fun f -> f tn)
|> ResourceMap.of_list

let load_color = function
  | TRGArray[TRGInt r; TRGInt g; TRGInt b] -> 
    let rgb_value v = 
      if 0 <= v && v <= 255 then v
      else raise_failure ("Not a valid RGB value: "^string_of_int v)
    in
    (rgb_value r, rgb_value g, rgb_value b)
  | tn -> expected "an RGB color" tn

let load_origin = function
  | TRGArray[TRGInt x; TRGInt y] -> (x,y)
  | tn -> expected "a coordinate" tn

let load_files = function
  | TRGArray files -> files |> List.filter is_string |> List.map (load_string >> fix_path)
  | tn -> expected "an array of paths" tn

let compute_origin name player team = 
    let result = (fst player + fst team, snd player + snd team) in
    if (
      0 <= fst result && fst result < (Flags.compile_flags.map_width) &&
      0 <= snd result && snd result < (Flags.compile_flags.map_height)
    ) then result
    else raise_failure ("'" ^ name ^ "' would spawn outside the map ")

let load_player team_id team_origin player =
  let name = find_entry "name" player ~default:TRGNull |> load_string in
  let origin = find_entry "origin" player ~default:(TRGArray[TRGInt 0; TRGInt 0]) |> load_origin in
  PI {
    team = team_id;
    name = name;
    origin = compute_origin name origin team_origin;
    files = find_entry "files" player ~default:TRGNull |> load_files
  }

let load_players team_id team_origin = function
  | TRGArray players -> List.map (load_player team_id team_origin) players
  | tn -> expected "an array of player definitions" tn

let load_team id team = 
  let origin = find_entry "origin" team ~default:TRGNull |> load_origin in
  TI {
    name = find_entry "name" team ~default:TRGNull |> load_string;
    color = find_entry "color" team ~default:TRGNull |> load_color;
    origin = origin;
    players = find_entry "players" team ~default:TRGNull |> load_players id origin;
    system_library = find_entry "system_library" team ~default:TRGNull |> optional_load load_string;
    library = find_entry "library" team ~default:TRGNull |> optional_load load_string;
  }

let load_teams trg = 
  let teams = match trg with
    | TRGArray [] -> raise_failure "There must exist atleast one team"
    | TRGArray teams -> List.mapi load_team teams
    | trg -> expected "an array of team definitions" trg
  in
  Helpers.compiler_notes.team_libraries <- StringMap.of_list (List.map (fun (TI team) -> (team.name, team.library)) teams); teams

let load_game o : game_setup = 
  Flags.set_auto_resize (find_entry "auto_resize" o ~default:(TRGBool true) |> load_bool);
  Flags.set_features (find_entry "features" o ~default:(TRGBool false) |> load_features);
  Flags.set_themes (find_entry "themes" o ~default:(TRGBool false) |> load_themes);

  Helpers.compiler_notes.system_library <- find_entry "system_library" o ~default:TRGNull |> optional_load load_string;
  Helpers.compiler_notes.shared_library <- find_entry "shared_library" o ~default:TRGNull |> optional_load load_string;
  Helpers.compiler_notes.size_limit <- find_entry "program_size_limit" o ~default:(TRGInt 1000) |> load_int;
  Helpers.compiler_notes.stack_size <- find_entry "stack_size" o ~default:(TRGInt 1000) |> load_int;

  GS {
    teams = find_entry "teams" o ~default:TRGNull |> load_teams;
    resources = find_entry "resources" o ~default:(TRGArray[]) |> load_resources;
    actions = find_entry "actions" o ~default:(TRGInt 1) |> load_int;
    steps = find_entry "steps" o ~default:(TRGInt 100) |> load_int;
    mode = find_entry "mode" o ~default:(TRGInt 0) |> load_mode;
    nuke = find_entry "nuke" o ~default:(TRGInt 0) |> load_int; (* Remove??? *)
    exec_mode = DefaultExec;
    seed = find_entry "seed" o ~default:TRGNull |> load_seed;
    time_scale = find_entry "time_scale" o ~default:(TRGFloat 1.0) |> load_time_scale;
    map = find_entry "map" o ~default:TRGNull |> load_map;
    setting_overwrites = [];
    debug = find_entry "debug" o ~default:(TRGBool false) |> load_bool;
    viewport = find_entry "viewport" o ~default:(TRGArray[TRGInt 20; TRGInt 20]) |> load_viewport;
    auto_start = find_entry "auto_start" o ~default:(TRGBool true) |> load_bool;
  }

let parse parser lexer from str ~default =
  match str with 
  | None -> default
  | Some str ->
  try (
    let lexbuf = Lexing.from_string str in
    try 
      parser (lexer from) lexbuf
    with
    | Failure(None,None,m) -> raise (Failure(Some from,Some(lexbuf.lex_curr_p.pos_lnum),m))
    | Failure(p,l,m) -> raise (Failure(p,l,m))
    | _ -> raise (Failure(Some from, Some(lexbuf.lex_curr_p.pos_lnum), "Syntax error")) 
  )
  with 
  | Failure(None,ln,msg) -> raise (Failure(Some from, ln, msg))
  | Failure _ as f -> raise f
  (*| _ -> raise (Failure(Some path, None, "Parser error"))*)

let parse_file parser lexer path ~default =
  match path with 
  | None -> default
  | Some path ->
  let path = (compress_path (total_path path)) in
  try (
    parse parser lexer path (Some (read_file path)) ~default:default
  )
  with 
  | Failure(None,ln,msg) -> raise (Failure(Some path,ln,msg))
  | Failure _ as f -> raise f
  (*| _ -> raise (Failure(Some path, None, "Parser error"))*)

let player_to_program program = 
  let size_limit = Helpers.compiler_notes.size_limit in
  let program = program_to_int_list program in
  if size_limit > 0 && List.length program - 1 > size_limit then raise_failure ("Program too large" ^ string_of_int size_limit ^ " " ^ string_of_int (List.length program - 1))
  else List.length program :: program |> Array.of_list 


type compiled_player_info = {
  team: int;
  name: string;
  position: int * int;
  file: string;
  extra_files_count: int;
  extra_files: string array;
}

type compiled_game_file = {
  actions: int;
  steps: int;
  mode: int;
  nuke: int;
  map: map;
  player_count: int;
  player_info: compiled_player_info array;
  team_count: int;
  teams: (string * (int*int*int) * int) array;
  exec_mode: exec_mode;
  resources: (int * int * int) array;
  seed: int option;
  time_scale: float;
  settings_count: int;
  settings: setting array;
  debug: bool;
  viewport: int * int;
  auto_start: bool;
}

let compile_program state (File(program,i)) =
  compile_stmts {state with labels = available_labels (Stmt(Block program, i))} program

let scope_ s = 
  List.map identifier_name s |> String.concat " "

let empty_file = File([],0)  (* Not a good solution *)

(* Figure out the order :( *)
let compile_player team file  =
  let syscalls = [] in
  let team_sys_lib = StringMap.find_opt team Helpers.compiler_notes.team_system_libraries |> Option.join in
  let team_lib = StringMap.find_opt team Helpers.compiler_notes.team_libraries |> Option.join in

  let init_state = {scopes = { local = syscalls ; global = None }; size = 0; labels = StringSet.empty; break = None; continue = None; ret_type = None;} in

  let (system_state, system_instrs) = parse_file Tr_parser.main Tr_lexer.start Helpers.compiler_notes.system_library ~default:empty_file |> compile_program init_state in
  let (team_system_state, team_system_instrs) = parse_file Tr_parser.main Tr_lexer.start team_sys_lib ~default:empty_file |> compile_program system_state in
  let (shared_state, shared_instrs) = parse_file Tr_parser.main Tr_lexer.start Helpers.compiler_notes.shared_library ~default:empty_file |> compile_program team_system_state in

  let visible = (List.length shared_state.scopes.local) - (List.length team_system_state.scopes.local) in

  let shared = shared_state.scopes.local |> List.take visible in
  let hidden = shared_state.scopes.local |> List.drop visible |> List.map remove_identifier_name in

  let state = {scopes = { local = generate_initial_scope () @ shared @ hidden ; global = None }; size = team_system_state.size; labels = StringSet.empty; break = None; continue = None; ret_type = None;} in

  let (team_state, team_instrs) = parse_file Tr_parser.main Tr_lexer.start team_lib ~default:empty_file |> compile_program state in
  let (state, instrs) = compile_program team_state file in


  (* Declare per block instead? Decreases stack size, increases program size, Could remove state.size *)
  Instr_Declare :: I(state.size) :: (system_instrs @ shared_instrs @ team_system_instrs @ team_instrs @ instrs) |> Optimize.optimize_instruction_list

let compile_player_file path team = try (
  check_path path [".tr"] ;
  parse_file Tr_parser.main Tr_lexer.start (Some path) ~default:empty_file
  |> compile_player team
  |> player_to_program
  |> Result.ok
) with
| Failure(None,ln,msg) -> Error(format_failure (Failure(Some path, ln, msg)))
| Failure _ as f -> Error(format_failure f)

let game_setup_player (PI player) = 
  if (List.length player.files < 1) then raise_failure ("Player with no files: " ^ player.name)
  else
  {
    team = player.team;
    name = player.name;
    position = player.origin;
    file = List.hd player.files;
    extra_files_count = List.length player.files - 1;
    extra_files = List.tl player.files |> List.rev |> Array.of_list;
  }  

let team_list teams =
  List.map (fun (TI ti) -> (ti.name, ti.color, List.length ti.players)) teams
  
let player_list teams =
  teams
  |> List.map (fun (TI ti) -> ti.players)
  |> List.flatten

let format_game_setup (GS gs) = 
  let teams = gs.teams |> team_list |> Array.of_list in
  let players = player_list gs.teams in
  {
    actions = gs.actions;
    steps = gs.steps;
    mode = gs.mode;
    nuke = gs.nuke;
    player_count = List.length players;
    player_info = Array.of_list (List.map game_setup_player players);
    team_count = Array.length teams;
    teams = teams;
    exec_mode = gs.exec_mode;
    resources = gs.resources |> ResourceMap.bindings |> List.map (fun (name,(start,max)) -> (resource_value name, start, max)) |> Array.of_list;
    seed = gs.seed;
    time_scale = gs.time_scale;
    map = gs.map;
    settings_count = List.length gs.setting_overwrites;
    settings = Array.of_list gs.setting_overwrites;
    debug = gs.debug;
    viewport = gs.viewport;
    auto_start = gs.auto_start;
  }

let compile_game_file path = try (
  check_path path [".trg"];
  let game_file_dir = Filename.dirname path in
  compiler_notes.dir <- game_file_dir;
  parse_file Trg_parser.main Trg_lexer.start (Some path) ~default:TRGNull
  |> load_game 
  |> format_game_setup
  |> Result.ok
) with
| Failure(None,ln,msg) -> Error(format_failure (Failure(Some path, ln, msg)))
| Failure _ as f -> Error(format_failure f)


let _ = Callback.register "compile_game_file" compile_game_file
let _ = Callback.register "compile_player_file" (fun path team -> compile_player_file path team |> Result.map (fun program -> program |> Array.map Int32.of_int |> Bigarray.Array1.of_array Int32 C_layout))
