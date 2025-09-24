%{
  open Absyn
  open Exceptions
  open Themes
  open Features
  
  let missing_player_info name files =
    String.concat ", " ([
      if Option.is_none name then Some "name" else None;
      if Option.is_none files then Some "files" else None;
    ] |> List.filter Option.is_some |> List.map Option.get)

  let missing_team_info name color players =
    String.concat ", " ([
      if Option.is_none name then Some "name" else None;
      if Option.is_none color then Some "color" else None;
      if List.is_empty players then Some "players" else None;
    ] |> List.filter Option.is_some |> List.map Option.get)

  let player_info_fields_to_player_info pifs =
    let rec aux pifs name origin files = match pifs with
      | [] -> ( match name, origin, files with
        | Some name, origin, Some files -> (PI{team = -1; name = name; origin = origin; files = files;})
        | _ -> raise_failure ("Player info missing: " ^ missing_player_info name files)
      )
      | PlayerName n :: t -> aux t (Some n) origin files
      | PlayerOrigin(x,y) :: t -> aux t name (x,y) files
      | PlayerFiles f :: t -> aux t name origin (Some f)
    in
    aux pifs None (0,0) None

  let team_info_fields_to_team_info tifs =
    let rec aux tifs name color origin players = match tifs with
      | [] -> ( match name, color, origin, players with
        | Some name, Some(r,g,b), origin, players when not(List.is_empty players) -> Team(TI{name = name; color = (r,g,b); origin = origin; players = players;})
        | _ -> raise_failure ("team info missing: " ^ missing_team_info name color players)
      )
      | TeamName n :: t -> aux t (Some n) color origin players
      | TeamColor(r,g,b) :: t -> aux t name (Some(r,g,b)) origin players
      | TeamOrigin(x,y) :: t -> aux t name color (x,y) players
      | TeamPlayer p :: t -> aux t name color origin (p :: players)
    in
    aux tifs None None (0,0) []

%}
%token <int> CSTINT
%token <float> CSTFLOAT
%token <string> WORD
%token LPAR RPAR LBRACE RBRACE OF DOT 
%token COMMA SEMI COLON EOF STAR FEATURES COLOR INFINITE MANUAL TRUE FALSE DEBUG VIEWPORT
%token PLAYER RESOURCES THEMES TIME_SCALE SEED ACTIONS STEPS MODE MAP NUKE NAME TEAM ORIGIN FILES EXEC_MODE SYNC ASYNC SETTINGS
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
  | NAME COLON WORD SEMI? { PlayerName $3 }
  | ORIGIN COLON CSTINT COMMA CSTINT SEMI? { PlayerOrigin($3, $5) }
  | FILES COLON seperated(COMMA, WORD) SEMI? { PlayerFiles $3 }
;

team_info:
  | NAME COLON WORD SEMI? { TeamName $3 }
  | COLOR COLON CSTINT COMMA CSTINT COMMA CSTINT SEMI? { TeamColor ($3,$5,$7) }
  | ORIGIN COLON CSTINT COMMA CSTINT SEMI? { TeamOrigin ($3, $5) }
  | PLAYER COLON LBRACE player_info+ RBRACE { TeamPlayer(player_info_fields_to_player_info $4) }
;

resource:
  | WORD COLON CSTINT OF CSTINT SEMI? { ($1, ($3, $5)) }
  | WORD COLON CSTINT SEMI? { ($1, ($3, -1)) }
;

game_mode:
  | CSTINT { Mode $1 }
  | MANUAL { Mode (-1) }
  | INFINITE { Mode 0 }
;

game_setup_part:
  | TEAM COLON LBRACE team_info+ RBRACE { team_info_fields_to_team_info $4 }
  | RESOURCES COLON LBRACE resource* RBRACE { Resources $4 }
  | THEMES COLON seperated(COMMA, WORD) SEMI? { Themes ($3 |> StringSet.of_list) }
  | THEMES COLON STAR SEMI? { Themes all_themes }
  | FEATURES COLON seperated(COMMA, WORD) SEMI? { Features ($3 |> StringSet.of_list) }
  | FEATURES COLON STAR SEMI? { Features all_features }
  | ACTIONS COLON CSTINT SEMI? { Actions $3 }
  | STEPS COLON CSTINT SEMI? { Steps $3 }
  | MODE COLON game_mode SEMI? { $3 }
  | MAP COLON CSTINT COMMA CSTINT SEMI? { Map(EmptyMap($3,$5)) }
  | MAP COLON WORD SEMI? { Map(FileMap($3,(-1,-1))) }
  | NUKE COLON CSTINT SEMI? { Nuke $3 }
  | EXEC_MODE COLON SYNC SEMI? { ExecMode SyncExec }
  | EXEC_MODE COLON ASYNC SEMI? { ExecMode AsyncExec }
  | SEED COLON CSTINT SEMI? { Seed (Some $3) }
  | TIME_SCALE COLON CSTFLOAT SEMI? { TimeScale $3 }
  | TIME_SCALE COLON CSTINT SEMI? { TimeScale(float_of_int $3) }
  | SETTINGS COLON LBRACE setting_group* RBRACE SEMI? { SettingOverwrites $4 }
  | DEBUG COLON TRUE SEMI? { Debug true }
  | DEBUG COLON FALSE SEMI? { Debug false }
  | VIEWPORT COLON CSTINT COMMA CSTINT SEMI? { Viewport($3,$5) }
;

setting_group:
  | WORD COLON LBRACE setting* RBRACE SEMI? { ($1,$4) }
;

setting:
  | WORD COLON CSTINT SEMI? { ($1,$3) }
;