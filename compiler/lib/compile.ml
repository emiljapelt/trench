open Optimize
open Typing
open Exceptions
open ToProgramRep
open ProgramRep
open Absyn
open Transform
open Themes

let check_input input =
  try (
    if not (Sys.file_exists input) then raise_failure ("Input file does not exist: "^input)
    else if String.ends_with ~suffix:".tr" input then ()
    else if String.ends_with ~suffix:".trg" input then ()
    else raise_failure "Invalid input file extension"
  ) with
  | ex -> raise ex


let get_line ls l =
  Printf.sprintf "%i | %s\n" (l+1) (List.nth ls l)

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
        let printer =  get_line lines in match line with
        | 1 -> line_msg ^ printer 0 ^ printer 1
        | n when n = (List.length lines) -> line_msg ^  printer (n-2) ^ printer (n-1)
        | _ ->  line_msg ^ printer (line-2) ^ printer (line-1) ^ printer line
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
  board = (20,20);
  nuke = 0;
  exec_mode = AsyncExec;
  seed = None;
  time_scale = 1.0;
}

let to_game_setup gsps =
  let rec aux gs (GS acc) = match gs with
    | [] -> (GS acc)
    | h::t -> aux t (match h with
      | Team t -> (GS ({acc with teams = t :: acc.teams}))
      | Resources rs -> (GS ({acc with resources = rs}))
      | Themes ts -> (GS ({acc with themes = ts}))
      | Features fs -> (GS ({acc with features = fs}))
      | Actions i -> if i > 0 then (GS ({acc with actions = i})) else raise_failure "Must have some actions per trun"
      | Steps i -> if i > 0 then (GS ({acc with steps = i})) else raise_failure "Must have some steps per trun"
      | Mode i -> (GS ({acc with mode = i}))
      | Board(x,y) -> if x >= 0 && y >= 0 then (GS ({acc with board = (x,y)})) else raise_failure "Board size cannot be negative"
      | Nuke i -> if i >= 0 then (GS ({acc with nuke = i})) else raise_failure "Nuke option size cannot be negative"
      | ExecMode em -> GS ({acc with exec_mode = em})
      | Seed s -> GS ({acc with seed = s})
      | TimeScale f -> if (f >= 0.0) then GS ({acc with time_scale = f}) else raise_failure "Time scale option cannot be negative"
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
  List.length program_list :: List.length regs :: program_list

type compiled_player_info = {
  team: int;
  name: string;
  position: int * int;
  file: string;
  directive: int array;
  directive_len: int;
}

type compiled_game_file = {
  actions: int;
  steps: int;
  mode: int;
  nuke: int;
  board_size: int * int;
  player_count: int;
  player_info: compiled_player_info array;
  team_count: int;
  teams: (string * (int*int*int) * int) array;
  exec_mode: exec_mode;
  resources_count: int;
  resources: (string * int) array;
  seed: int option;
  time_scale: float;
}

let compile_player_file path = try (
  let _ = check_input path in
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
  directive = Array.of_list p;
  directive_len = List.length p;
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
  let checked_teams = gs.teams |> check_team_colors |> add_team_info gs.board in
  let teams = checked_teams |> team_list |> Array.of_list in
  let players = player_list checked_teams in
  {
    actions = gs.actions;
    steps = gs.steps;
    mode = gs.mode;
    board_size = gs.board;
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
  }

let check_resources (GS gs) = 
  let required = all_required_resources gs.themes in
  let given = List.map fst gs.resources in
  if (List.for_all (fun g -> StringSet.mem g required) given 
    && StringSet.cardinal required = List.length given)
  then (GS gs)
  else raise_failure ("Defined resources does not match required resource. \nRequired:\n\t" ^ (String.concat "\n\t" (StringSet.to_list required)) ^ "\nGiven:\n\t" ^ (String.concat "\n\t" given))

let compile_game_file path = try (
  let _ = check_input path in
  Ok(compile Game_parser.main Game_lexer.start [] [] (fun gsp -> gsp |> to_game_setup |> check_resources) format_game_setup path)
) with
| Failure(None,ln,msg) -> Error(format_failure (Failure(Some path, ln, msg)))
| Failure _ as f -> Error(format_failure f)


let _ = Callback.register "compile_game_file" compile_game_file
let _ = Callback.register "compile_player_file" compile_player_file
