%{
  open Absyn
%}
%token <int> CSTINT
%token <string> PATH
%token LPAR RPAR
%token COMMA SEMI COLON EOF
%token PLAYER BOMBS SHOTS ACTIONS STEPS MODE BOARD NUKE GLOBAL_ARRAY FEATURE_LEVEL

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

game_setup_part:
  | PLAYER COLON CSTINT COMMA LPAR CSTINT COMMA CSTINT RPAR COMMA PATH SEMI { Player(PI {id = $3; position = ($6,$8); path = $11}) }
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