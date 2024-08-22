%{
  open Absyn
  open Exceptions
  open Lexing
  open Flags

  (*type var_name_generator = { mutable next : int }
  let vg = ( {next = 0;} )
  let new_var () =
    let number = vg.next in
    let () = vg.next <- vg.next+1 in
    Int.to_string number*)

  let meta_name n fn ln = match n with
    | "x" -> PlayerX
    | "y" -> PlayerY
    | "bombs" -> PlayerBombs
    | "shots" -> PlayerShots
    | "board_x" -> BoardX
    | "board_y" -> BoardY
    | "array_size" ->  GlobalArraySize
    | _ -> raise (Failure(Some fn, Some ln, "Unknown meta reference"))

  let feature l =
    if l > compile_flags.feature_level then raise_failure ("Attempt to access feature level: "^string_of_int l^", while in level: "^string_of_int compile_flags.feature_level)
    else ()

%}
%token <int> CSTINT
%token <string> NAME
%token <string> LABEL
%token <string> PATH
%token LPAR RPAR LBRACE RBRACE LBRAKE RBRAKE
%token PLUS MINUS TIMES EQ NEQ LT GT LTEQ GTEQ
%token LOGIC_AND LOGIC_OR PIPE FSLASH PCT TILDE
%token COMMA SEMI COLON DOT EOF
%token QMARK
%token IF ELSE REPEAT WHILE CONTINUE BREAK
%token GOTO
%token MOVE FORTIFY WAIT PASS TRENCH
%token NORTH EAST SOUTH WEST BOMB SHOOT LOOK SCAN MINE ATTACK
%token HASH INT DIR FIELD FLAGS
%token PLAYER_CAP TRENCH_CAP MINE_CAP DESTROYED_CAP

/*Low precedence*/
%left LOGIC_AND LOGIC_OR
%left EQ NEQ
%left GT LT GTEQ LTEQ
%left PLUS MINUS
%left TIMES FSLASH PCT
%nonassoc TILDE HASH 
%nonassoc SCAN CHECK
/*High precedence*/

%start main
%type <Absyn.file> main
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
  stmt* EOF     { (File ([],$1)) }
;

typ:
  | INT { T_Int }
  | DIR { T_Dir }
  | FIELD { T_Field }
;

block:
  LBRACE stmt* RBRACE    { Block $2 }
;

const_value:
  | CSTINT      { Int $1 }
  | direction   { Direction $1}
  | error { raise (Failure(Some $symbolstartpos.pos_fname, Some $symbolstartpos.pos_lnum, "Expected a constant value")) }
;

simple_value:
  | const_value                        { $1 }
  | QMARK                              { feature 2 ; Random }
  | QMARK LPAR simple_value+ RPAR      { feature 2 ; RandomSet $3 }
  | MINUS simple_value                 { Binary_op ("-", Int 0, $2) } %prec TILDE
  | TILDE simple_value                 { Unary_op ("~", $2) }
  | NAME                               { feature 1 ; Reference(Local $1) }
  | typ LBRAKE value RBRAKE            { feature 1 ; Reference(Global($1,$3)) }
  | HASH NAME                          { MetaReference (meta_name $2 $symbolstartpos.pos_fname $symbolstartpos.pos_lnum) }
  | LPAR value RPAR                    { $2 }
;

flag:
  | PLAYER_CAP { PLAYER }
  | TRENCH_CAP { TRENCH }
  | MINE_CAP   { MINE }
  | DESTROYED_CAP { DESTROYED }
;

value:
  | simple_value        { $1 }
  | SCAN simple_value simple_value          { Scan($2,$3) }
  | LOOK simple_value         { Look $2 }
  | value binop value         { Binary_op ($2, $1, $3) }
  | simple_value DOT flag     { Flag($1, $3) }
;

%inline binop:
  | LOGIC_AND   { "&" }
  | LOGIC_OR    { "|" }
  | EQ          { "="  }
  | NEQ         { "!=" }
  | LTEQ        { "<=" }
  | LT          { "<"  }
  | GTEQ        { ">=" }
  | GT          { ">"  }
  | PLUS        { "+"  }
  | TIMES       { "*"  }
  | MINUS       { "-"  }
  | FSLASH      { "/"  }
  | PCT         { "%"  }
;

stmt:
  | stmt1 { $1 }
  | stmt2 { $1 }
;

stmt2:
  stmt2_inner { Stmt($1,$symbolstartpos.pos_lnum) }
;
stmt2_inner:
  | IF LPAR value RPAR stmt1 ELSE stmt2       { feature 2 ; If ($3, $5, $7) }
  | IF LPAR value RPAR stmt                   { feature 2 ; If ($3, $5, Stmt(Block [], $symbolstartpos.pos_lnum)) }
;

/* No unbalanced if-else */
stmt1:
  stmt1_inner { Stmt($1,$symbolstartpos.pos_lnum) }
;
stmt1_inner: 
  | block                                     { $1 }
  | IF LPAR value RPAR stmt1 ELSE stmt1       { feature 2 ; If ($3, $5, $7) }
  | WHILE LPAR value RPAR stmt1               { feature 3 ; While($3,$5,None) }
  | BREAK SEMI                                { feature 3 ; Break }
  | CONTINUE SEMI                             { feature 3 ; Continue }
  | GOTO NAME SEMI                            { GoTo $2 }
  | LABEL                                     { Label $1 }
  | REPEAT LPAR CSTINT RPAR stmt1             { feature 2 ; Block(List.init $3 (fun _ -> $5)) }
  | non_control_flow_stmt SEMI                { $1 }
;

target:
  | NAME { feature 2 ; Local $1 }
  | typ LBRAKE value RBRAKE { feature 1 ; Global($1,$3) }
; 

non_control_flow_stmt:
  | target EQ value        { feature 1 ; Assign ($1, $3) }
  | target PLUS EQ value   { feature 1 ; Assign ($1, Binary_op("+", Reference $1, $4)) }
  | target MINUS EQ value  { feature 1 ; Assign ($1, Binary_op("-", Reference $1, $4)) }
  | target TIMES EQ value  { feature 1 ; Assign ($1, Binary_op("*", Reference $1, $4)) }
  | target TILDE EQ value  { feature 1 ; Assign ($1, Unary_op("~", $4)) }
  | typ NAME                                  { feature 1 ; Declare($1,$2) }
  | typ NAME EQ value                         { feature 1 ; DeclareAssign($1,$2,$4) }
  | MOVE value                        { Move $2 }
  | SHOOT value                       { Shoot $2 }
  | MINE value                        { Mine $2 }
  | ATTACK value                      { Attack $2 }
  | FORTIFY                           { Fortify None }
  | FORTIFY value                     { Fortify (Some $2) }
  | TRENCH                            { Trench None }
  | TRENCH value                      { Trench (Some $2) }
  | WAIT                              { Wait }
  | PASS                              { Pass }
  | BOMB simple_value simple_value    { Bomb($2, $3) }
;

direction:
  | NORTH   { North }
  | EAST  { East  }
  | SOUTH { South }
  | WEST  { West }
;