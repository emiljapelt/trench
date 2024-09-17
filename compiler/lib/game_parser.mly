%{
  open Absyn
  open Exceptions
  
  let missing_info team name pos file =
    String.concat ", " ([
      if Option.is_none team then Some "team" else None;
      if Option.is_none name then Some "name" else None;
      if Option.is_none pos then Some "position" else None;
      if Option.is_none file then Some "file" else None;
    ] |> List.filter Option.is_some |> List.map Option.get)

  let player_info_fields_to_player_info pifs =
    let rec aux pifs team name pos file = match pifs with
      | [] -> ( match team, name, pos, file with
        | Some team, Some name, Some pos, Some file -> Player(PI{team = team; name = name; position = pos; file = file;})
        | _ -> raise_failure ("Player info missing: " ^ missing_info team name pos file)
      )
      | PlayerTeam i :: t -> aux t (Some i) name pos file
      | PlayerName n :: t -> aux t team (Some n) pos file
      | PlayerPosition(x,y) :: t -> aux t team name (Some (x,y)) file
      | PlayerFile f :: t -> aux t team name pos (Some f)
    in
    aux pifs None None None None

%}
%token <int> CSTINT
%token <string> WORD
%token LPAR RPAR LBRACE RBRACE
%token COMMA SEMI COLON EOF 
%token PLAYER BOMBS SHOTS ACTIONS STEPS MODE BOARD NUKE GLOBAL_ARRAY FEATURE_LEVEL NAME TEAM POSITION FILE

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
  | TEAM COLON CSTINT SEMI  { PlayerTeam $3 }
  | NAME COLON WORD SEMI { PlayerName $3 }
  | POSITION COLON CSTINT COMMA CSTINT SEMI { PlayerPosition($3, $5) }
  | FILE COLON WORD SEMI { PlayerFile $3 }
;

game_setup_part:
  | PLAYER COLON LBRACE player_info+ RBRACE { player_info_fields_to_player_info $4 }
  | BOMBS COLON CSTINT SEMI { Bombs $3 }
  | SHOTS COLON CSTINT SEMI { Shots $3}
  | ACTIONS COLON CSTINT SEMI { Actions $3 }
  | STEPS COLON CSTINT SEMI { Steps $3 }
  | MODE COLON CSTINT SEMI { Mode $3 }
  | BOARD COLON CSTINT COMMA CSTINT SEMI { Board ($3,$5)}
  | NUKE COLON CSTINT SEMI { Nuke $3 }
  | GLOBAL_ARRAY COLON CSTINT SEMI { GlobalArray $3 }
  | FEATURE_LEVEL COLON CSTINT SEMI { FeatureLevel $3 }
;