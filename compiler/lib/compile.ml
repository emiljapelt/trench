open Optimize
open Typing
open Exceptions
open ToProgramRep
open ProgramRep
open Str
open Absyn

let check_input input =
  try (
    if not (Sys.file_exists input) then (Printf.printf "%s\n" input; raise_failure "Input file does not exist")
    else if Str.string_match (regexp {|^\(\.\.?\)?\/?\(\([a-zA-Z0-9_-]+\|\(\.\.?\)\)\/\)*[a-zA-Z0-9_-]+\.trg?$|}) input 0 then ()
    else raise_failure "Invalid input file extension"
  ) with
  | ex -> raise ex

let print_line ls l =
  Printf.printf "%i | %s\n" (l+1) (List.nth ls l)

let read_file path =
  let file = open_in path in
  let content = really_input_string (file) (in_channel_length file) in
  let () = close_in_noerr file in
  content

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
  steps = 1000;
  mode = 0;
  board = (20,20);
  nuke = 0;
}

let to_game_setup gsps =
  let rec aux gs (GS acc) = match gs with
    | [] -> (GS acc)
    | h::t -> aux t (match h with
      | Player pi -> (GS ({acc with players = pi :: acc.players}))
      | Bombs i -> (GS ({acc with bombs = i}))
      | Shots i -> (GS ({acc with shots = i}))
      | Actions i -> (GS ({acc with actions = i}))
      | Steps i -> (GS ({acc with steps = i}))
      | Mode i -> (GS ({acc with mode = i}))
      | Board(x,y) -> (GS ({acc with board = (x,y)}))
      | Nuke i -> (GS ({acc with nuke = i}))
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
    let info = regs_to_string regs^":"^program_to_string program in
    (int_to_binary (String.length info)) ^ "\n" ^ info
    

let compile_player_file path = 
  compile Player_parser.main Player_lexer.start [type_check_program;optimize_program] [check_registers_unique] compile_player player_to_string path

let rec compile_game_file path =
  compile Game_parser.main Game_lexer.start [] [] to_game_setup to_binary path

and players_to_binary ps = 
  let rec aux ps acc = match ps with
    | [] -> String.concat "" acc
    | (PI({id = id; pos = (x,y); file = file}))::t -> aux t (
      let dir = compile_player_file file in
      (Printf.sprintf "%s%s%s%s%s%s%s" 
        (int_to_binary id)
        (int_to_binary x)
        (int_to_binary y)
        (int_to_binary (String.length file))
        (int_to_binary (String.length dir))
        (file)
        (dir)
      )
      ::acc
    )
  in
  aux ps []


(* Binary format: [n] = n bytes; n {...} = repeat n time
  int length;
  int bombs;
  int shots;
  int actions;
  int steps;
  int mode;
  int nuke;
  int board_x;
  int board_y;
  int player_count;
  player_count {
    int id;
    int x;
    int y;
    int pl;
    int dl;
    [pl] p;
    [dl] d;
  }
*)
and to_binary (GS gs) = 
  let binary = Printf.sprintf "%s%s%s%s%s%s%s%s%s%s"
  (int_to_binary gs.bombs)
  (int_to_binary gs.shots)
  (int_to_binary gs.actions)
  (int_to_binary gs.steps)
  (int_to_binary gs.mode)
  (int_to_binary gs.nuke)
  (int_to_binary (fst gs.board))
  (int_to_binary (snd gs.board))
  (int_to_binary (List.length gs.players))
  (players_to_binary gs.players)
  in
  (int_to_binary (String.length binary)) ^ "\n" ^ binary
  

let compile_file path = try (
  let _ = check_input path in
  if (String.ends_with ~suffix:"g" path) then 
    compile_game_file path
  else if (String.ends_with ~suffix:"r" path) then
    compile_player_file path
  else raise_failure "Dont know what to do"
) with 
| Failure(file_opt, line_opt, expl) -> (
  Printf.printf "%s %s" expl (Option.value file_opt ~default:"") ;
    if Option.is_some line_opt then (
      Printf.printf ", line %i: \n" (Option.get line_opt) ;
      let line = Option.get line_opt in
      let lines = String.split_on_char '\n' (read_file path) in
      let printer =  print_line lines in match line with
      | 1 -> printer 0 ; printer 1
      | n when n = (List.length lines)-1 -> printer (n-2) ; printer (n-1)
      | _ ->  printer (line-2) ; printer (line-1) ; printer line
    )
    else Printf.printf "\n" ;
    exit 1
)

