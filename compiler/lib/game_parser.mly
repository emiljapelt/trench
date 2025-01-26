%{
  open Absyn
  open Exceptions
  open Themes
  open Features
  
  let missing_player_info name pos file =
    String.concat ", " ([
      if Option.is_none name then Some "name" else None;
      if Option.is_none pos then Some "position" else None;
      if Option.is_none file then Some "file" else None;
    ] |> List.filter Option.is_some |> List.map Option.get)

  let missing_team_info name color players =
    String.concat ", " ([
      if Option.is_none name then Some "name" else None;
      if Option.is_none color then Some "color" else None;
      if List.is_empty players then Some "players" else None;
    ] |> List.filter Option.is_some |> List.map Option.get)

  let player_info_fields_to_player_info pifs =
    let rec aux pifs name pos file = match pifs with
      | [] -> ( match name, pos, file with
        | Some name, Some pos, Some file -> (PI{team = -1; name = name; position = pos; file = file;})
        | _ -> raise_failure ("Player info missing: " ^ missing_player_info name pos file)
      )
      | PlayerName n :: t -> aux t (Some n) pos file
      | PlayerPosition(x,y) :: t -> aux t name (Some (x,y)) file
      | PlayerFile f :: t -> aux t name pos (Some f)
    in
    aux pifs None None None

  let team_info_fields_to_team_info tifs =
    let rec aux tifs name color players = match tifs with
      | [] -> ( match name, color, players with
        | Some name, Some(r,g,b), players when not(List.is_empty players) -> Team(TI{name = name; color = (r,g,b); players = players;})
        | _ -> raise_failure ("team info missing: " ^ missing_team_info name color players)
      )
      | TeamName n :: t -> aux t (Some n) color players
      | TeamColor(r,g,b) :: t -> aux t name (Some (r,g,b)) players
      | TeamPlayer p :: t -> aux t name color (p :: players)
    in
    aux tifs None None []

%}
%token <int> CSTINT
%token <float> CSTFLOAT
%token <string> WORD
%token LPAR RPAR LBRACE RBRACE
%token COMMA SEMI COLON EOF STAR FEATURES COLOR
%token PLAYER RESOURCES THEMES TIME_SCALE SEED ACTIONS STEPS MODE BOARD NUKE NAME TEAM POSITION FILE EXEC_MODE SYNC ASYNC

/*Low precedence*/

/*High precedence*/

%start main
%type <Absyn.game_setup_part list> main
%%

%public seperated_or_empty(S,C):
  | {[]}
  | C  {[$1]}
  | C S seperated(S,C) {$1::$3}
;

%public seperated(S,C):
  | C  {[$1]}
  | C S seperated(S,C) {$1::$3}
;

main:
  game_setup_part* EOF { $1 }
;

player_info:
  | NAME COLON WORD SEMI { PlayerName $3 }
  | POSITION COLON CSTINT COMMA CSTINT SEMI { PlayerPosition($3, $5) }
  | FILE COLON WORD SEMI { PlayerFile $3 }
;

team_info:
  | NAME COLON WORD SEMI { TeamName $3 }
  | COLOR COLON CSTINT COMMA CSTINT COMMA CSTINT SEMI { TeamColor ($3,$5,$7) }
  | PLAYER COLON LBRACE player_info+ RBRACE { TeamPlayer(player_info_fields_to_player_info $4) }
;

resource:
  | WORD COLON CSTINT SEMI { ($1, $3) }
;

game_setup_part:
  | TEAM COLON LBRACE team_info+ RBRACE { team_info_fields_to_team_info $4 }
  | RESOURCES COLON LBRACE resource* RBRACE { Resources $4 }
  | THEMES COLON seperated(COMMA, WORD) SEMI { Themes ($3 |> StringSet.of_list) }
  | THEMES COLON STAR SEMI { Themes all_themes }
  | FEATURES COLON seperated(COMMA, WORD) SEMI { Features ($3 |> StringSet.of_list) }
  | FEATURES COLON STAR SEMI { Features all_features }
  | ACTIONS COLON CSTINT SEMI { Actions $3 }
  | STEPS COLON CSTINT SEMI { Steps $3 }
  | MODE COLON CSTINT SEMI { Mode $3 }
  | BOARD COLON CSTINT COMMA CSTINT SEMI { Board ($3,$5) }
  | NUKE COLON CSTINT SEMI { Nuke $3 }
  | EXEC_MODE COLON SYNC SEMI { ExecMode SyncExec }
  | EXEC_MODE COLON ASYNC SEMI { ExecMode AsyncExec }
  | SEED COLON CSTINT SEMI { Seed (Some $3) }
  | TIME_SCALE COLON CSTFLOAT SEMI { TimeScale $3 }
;