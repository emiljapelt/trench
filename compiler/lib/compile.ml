open Optimize
open Typing
open Exceptions
open ToProgramRep
open ProgramRep
open Str
open Absyn

let check_input input =
  try (
    if not (Sys.file_exists input) then raise_failure ("Input file does not exist: "^input)
    else if Str.string_match (regexp {|^\(\.\.?\)?\/?\(\([a-zA-Z0-9_-]+\|\(\.\.?\)\)\/\)*[a-zA-Z0-9_-]+\.trg?$|}) input 0 then ()
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

let format_failure path f = match f with
  | Failure(file_opt, line_opt, expl) -> (
    let msg = Printf.sprintf "%s %s" expl (Option.value file_opt ~default:"") in
    let details = if Option.is_some line_opt then (
        let line_msg = Printf.sprintf ", line %i: \n" (Option.get line_opt) in
        let line = Option.get line_opt in
        let lines = String.split_on_char '\n' (read_file path) in
        let printer =  get_line lines in match line with
        | 1 -> line_msg ^ printer 0 ^ printer 1
        | n when n = (List.length lines)-1 -> line_msg ^  printer (n-2) ^ printer (n-1)
        | _ ->  line_msg ^ printer (line-2) ^ printer (line-1) ^ printer line
      )
      else "\n" in
      msg ^ details
  )
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

let int_to_binary i : string =
  Printf.sprintf "%c%c%c%c"
    (Char.chr (Int.logand 255 (Int.shift_right i (0*8))))
    (Char.chr (Int.logand 255 (Int.shift_right i (1*8))))
    (Char.chr (Int.logand 255 (Int.shift_right i (2*8))))
    (Char.chr (Int.logand 255 (Int.shift_right i (3*8))))


let default_game_setup = GS {
  players = [];
  bombs = 3;
  shots = 10;
  actions = 1;
  steps = 100;
  mode = 0;
  board = (20,20);
  nuke = 0;
  array = 10;
}

let to_game_setup gsps =
  let rec aux gs (GS acc) = match gs with
    | [] -> (GS acc)
    | h::t -> aux t (match h with
      | Player pi -> (GS ({acc with players = pi :: acc.players}))
      | Bombs i -> if i >= 0 then (GS ({acc with bombs = i})) else raise_failure "Cannot have a negative amount of bombs"
      | Shots i -> if i >= 0 then (GS ({acc with shots = i})) else raise_failure "Cannot have a negative amount of shots"
      | Actions i -> if i > 0 then (GS ({acc with actions = i})) else raise_failure "Must have some actions per trun"
      | Steps i -> if i > 0 then (GS ({acc with steps = i})) else raise_failure "Must have some steps per trun"
      | Mode i -> (GS ({acc with mode = i}))
      | Board(x,y) -> if x >= 0 && y >= 0 then (GS ({acc with board = (x,y)})) else raise_failure "Board size cannot be negative"
      | Nuke i -> if i >= 0 then (GS ({acc with nuke = i})) else raise_failure "Nuke option size cannot be negative"
      | GlobalArray i -> if i >= 0 then (GS ({acc with array = i})) else raise_failure "Global array size cannot be negative"
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
    | Failure(p,l,m) -> raise (Failure(p,l,m))
    | _ -> raise (Failure(Some path, Some(lexbuf.lex_curr_p.pos_lnum), "Syntax error"))
    in
    List.iter (fun check -> check result) checks ;
    List.fold_left (fun acc trans -> trans acc) result transforms |> compiler |> stringer
  )
  with 
  | Failure _ as f -> raise f
  | _ -> raise (Failure(Some path, None, "Parser error"))
  

let check_registers_unique (File(regs,_)) =
  let rec aux regs set = match regs with
  | [] -> ()
  | Register(_,n,_)::t -> 
    if StringSet.mem n set 
    then raise_failure ("Duplicate register name: "^n) 
    else aux t (StringSet.add n set)
  in
  aux regs StringSet.empty

let player_to_string (regs,program) = 
  regs_to_string regs^":"^program_to_string program

type compiled_player_info = {
  id: int;
  position: int * int;
  path: string;
  directive: string;
  directive_len: int;
}

type compiled_game_file = {
  bombs: int;
  shots: int;
  actions: int;
  steps: int;
  mode: int;
  nuke: int;
  array: int;
  board_size: int * int;
  player_count: int;
  player_info: compiled_player_info array;
}

let compile_player_file path = try (
  let _ = check_input path in
  Ok(compile Player_parser.main Player_lexer.start [type_check_program;optimize_program] [check_registers_unique] compile_player player_to_string path)
) with
| Failure _ as f -> Error(format_failure path f)

let game_setup_player (PI player) = 
  let p = match compile_player_file player.path with
    | Ok p -> p
    | Error msg -> raise_failure msg
  in
  {
  id = player.id;
  position = player.position;
  path = player.path;
  directive = p;
  directive_len = String.length p;
}

let format_game_setup (GS gs) = {
  bombs = gs.bombs;
  shots = gs.shots;
  actions = gs.actions;
  steps = gs.steps;
  mode = gs.mode;
  board_size = gs.board;
  nuke = gs.nuke;
  array = gs.array;
  player_count = List.length gs.players;
  player_info = Array.of_list (List.map game_setup_player gs.players);
}

let compile_game_file path = try (
  let _ = check_input path in
  Ok(compile Game_parser.main Game_lexer.start [] [] to_game_setup format_game_setup path)
) with
| Failure _ as f -> Error(format_failure path f)


let _ = Callback.register "compile_game_file" compile_game_file
let _ = Callback.register "compile_player_file" compile_player_file
