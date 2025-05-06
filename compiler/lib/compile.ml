open Optimize
open Typing
open Exceptions
open ToProgramRep
open ProgramRep
open Absyn
open Transform
open Themes

let check_path path extensions =
  try (
    if not(Sys.file_exists path) then raise_failure ("File does not exist: " ^ path)
    else if not(extensions |> List.exists (fun ext -> String.ends_with ~suffix:ext path)) then raise_failure "Invalid file extension"
    else ()
  ) with
  | ex -> raise ex


let get_line ls l =
  Printf.sprintf "%i | %s\n" l (List.nth ls (l-1))

let read_file path =
  let file = open_in path in
  let content = really_input_string (file) (in_channel_length file) in
  let () = close_in_noerr file in
  content

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

let default_game_setup = GS {
  teams = [];
  themes = StringSet.empty;
  features = StringSet.empty;
  resources = [];
  actions = 1;
  steps = 100;
  mode = 0;
  map = EmptyMap(20,20);
  nuke = 0;
  exec_mode = AsyncExec;
  seed = None;
  time_scale = 1.0;
  shoot_limit = 10;
  throw_limit = 5;
}

let valid_map_chars = ['\n'; '\r'; '.'; '+'; '~'; 'T'; 'w']

let get_map_size map = match map with
  | EmptyMap(x,y)
  | FileMap(_,(x,y)) -> (x,y)

let check_map map = match map with
  | EmptyMap (x,y) -> if x > 0 || y > 0 then map else raise_failure "Map size cannot be negative"
  | FileMap(path,_) -> 
    check_path path [".trm"] ;
    let map_string = read_file path |> String.trim in
    if not(String.for_all (fun c -> List.mem c valid_map_chars) map_string) then raise_failure "Map contained invalid characters"
    else match String.split_on_char '\n' map_string with
    | [] | [""] -> raise_failure "Empty map file"
    | l::ls as lines -> (
      let x = String.length l in
      if not(ls |> List.for_all (fun line -> String.length line = x)) then raise_failure "Not all map lines were the same length"
      else FileMap(String.concat "" lines, (x,List.length lines))
    )

let to_game_setup gsps =
  let rec aux gs (GS acc) = match gs with
    | [] -> (GS acc)
    | h::t -> aux t (match h with
      | Team t -> (GS ({acc with teams = t :: acc.teams}))
      | Resources rs -> (GS ({acc with resources = rs}))
      | Themes ts -> (GS ({acc with themes = ts}))
      | Features fs -> (GS ({acc with features = fs}))
      | Actions i -> if i > 0 then (GS ({acc with actions = i})) else raise_failure "Must have some actions per turn"
      | Steps i -> if i > 0 then (GS ({acc with steps = i})) else raise_failure "Must have some steps per turn"
      | Mode i -> (GS ({acc with mode = i}))
      | Map m -> (GS ({acc with map = check_map m}))
      | Nuke i -> if i >= 0 then (GS ({acc with nuke = i})) else raise_failure "Nuke option size cannot be negative"
      | ExecMode em -> GS ({acc with exec_mode = em})
      | Seed s -> GS ({acc with seed = s})
      | TimeScale f -> if (f >= 0.0) then GS ({acc with time_scale = f}) else raise_failure "Time scale option cannot be negative"
      | ThrowLimit i -> if (i > 0) then GS ({acc with throw_limit = i}) else raise_failure "Throw limit must be positive"
      | ShootLimit i -> if (i > 0) then GS ({acc with shoot_limit = i}) else raise_failure "Shoot limit must be positive"
    )
  in
  aux gsps default_game_setup

let compile parser lexer transforms checks compiler stringer path =
  let path = (compress_path (total_path path)) in
  try (
    let lexbuf = (Lexing.from_string (read_file path)) in
    let result = 
    try 
      parser (lexer path) lexbuf
    with
    | Failure(None,None,m) -> raise (Failure(Some path,Some(lexbuf.lex_curr_p.pos_lnum),m))
    | Failure(p,l,m) -> raise (Failure(p,l,m))
    | _ -> raise (Failure(Some path, Some(lexbuf.lex_curr_p.pos_lnum), "Syntax error"))
    in
    List.iter (fun check -> check result) checks ;
    List.fold_left (fun acc trans -> trans acc) result transforms |> compiler |> stringer
  )
  with 
  | Failure(None,ln,msg) -> raise (Failure(Some path,ln,msg))
  | Failure _ as f -> raise f
  | _ -> raise (Failure(Some path, None, "Parser error"))
  

let check_vars_unique (File(regs,_)) =
  let rec aux regs set = match regs with
  | [] -> ()
  | Var(_,n)::t -> 
    if StringSet.mem n set 
    then raise_failure ("Duplicate register name: "^n) 
    else aux t (StringSet.add n set)
  in
  aux regs StringSet.empty

let player_to_program (regs,program) = 
  let program_list = program_to_int_list program in
  List.length program_list :: List.length regs :: program_list |> Array.of_list

type compiled_player_info = {
  team: int;
  name: string;
  position: int * int;
  file: string;
  directive: int array;
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
  resources_count: int;
  resources: (string * (int * int)) array;
  seed: int option;
  time_scale: float;
  throw_limit: int;
  shoot_limit: int;
}

let compile_player_file path = try (
  check_path path [".tr"] ;
  Ok(compile Player_parser.main Player_lexer.start [
    type_check_program;
    rename_variables_of_file;
    optimize_program;
    pull_out_declarations_of_file
  ] [check_vars_unique] compile_player player_to_program path)
) with
| Failure(None,ln,msg) -> Error(format_failure (Failure(Some path, ln, msg)))
| Failure _ as f -> Error(format_failure f)

let game_setup_player (PI player) = 
  let p = match compile_player_file player.file with
    | Ok p -> p
    | Error msg -> raise_failure msg
  in
  {
    team = player.team;
    name = player.name;
    position = player.origin;
    file = player.file;
    directive = p;
  }

let set_themes ts =
  Flags.compile_flags.themes <- ts ; ()

let set_features fs =
  Flags.compile_flags.features <- fs ; ()

let set_resources rs =
  Flags.compile_flags.resources <- List.map fst rs ; ()

module IntSet = Set.Make(Int)

let add_team_info board_size (teams : team_info list) : team_info list =
  let compute_origin (PI player) (TI team) = 
    let result = (fst player.origin + fst team.origin, snd player.origin + snd team.origin) in
    if (
      0 <= fst result && fst result < fst board_size &&
      0 <= snd result && snd result < snd board_size
    ) then result
    else raise_failure ("'" ^ player.name ^ "' of team '" ^ team.name ^ "' would spawn outside the board")
  in
  List.mapi (
    fun i (TI ti) -> (TI({
      ti with players = List.map (fun (PI p) -> PI({p with team = i; origin = compute_origin (PI p) (TI ti)})) ti.players 
    }))
  ) teams

let check_team_colors teams = 
  let check_rgb_value v = 
    if 0 <= v && v <= 255 then ()
    else raise_failure "Not a valid RGB color"
  in
  teams |> List.iter (fun (TI team) -> match team.color with
    | (r,g,b) -> 
      check_rgb_value r ;
      check_rgb_value g ;
      check_rgb_value b ; 
      ()
  ) ; teams

let team_list teams =
  List.map (fun (TI ti) -> (ti.name, ti.color, List.length ti.players)) teams
  
let player_list teams =
  teams
  |> List.map (fun (TI ti) -> ti.players)
  |> List.flatten

let format_game_setup (GS gs) = 
  let checked_teams = gs.teams |> check_team_colors |> add_team_info (get_map_size gs.map) in
  let teams = checked_teams |> team_list |> Array.of_list in
  let players = player_list checked_teams in
  {
    actions = gs.actions;
    steps = gs.steps;
    mode = gs.mode;
    nuke = gs.nuke;
    player_count = List.length players;
    player_info = (set_features gs.features ; set_themes gs.themes ; set_resources gs.resources ; Array.of_list (List.map game_setup_player players));
    team_count = Array.length teams;
    teams = teams;
    exec_mode = gs.exec_mode;
    resources_count = List.length gs.resources;
    resources = Array.of_list gs.resources;
    seed = gs.seed;
    time_scale = gs.time_scale;
    map = gs.map;
    throw_limit = gs.throw_limit;
    shoot_limit = gs.shoot_limit;
  }

let check_resources (GS gs) = 
  let required = all_required_resources gs.themes in
  let given = List.map fst gs.resources in
  if (List.for_all (fun g -> StringSet.mem g required) given 
    && StringSet.cardinal required = List.length given)
  then (GS gs)
  else raise_failure ("Defined resources does not match required resource. \nRequired:\n\t" ^ (String.concat "\n\t" (StringSet.to_list required)) ^ "\nGiven:\n\t" ^ (String.concat "\n\t" given))

let compile_game_file path = try (
  check_path path [".trg"] ;
  Ok(compile Game_parser.main Game_lexer.start [] [] (fun gsp -> gsp |> to_game_setup |> check_resources) format_game_setup path)
) with
| Failure(None,ln,msg) -> Error(format_failure (Failure(Some path, ln, msg)))
| Failure _ as f -> Error(format_failure f)


let _ = Callback.register "compile_game_file" compile_game_file
let _ = Callback.register "compile_player_file" compile_player_file
