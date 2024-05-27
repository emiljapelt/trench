%{
  open Absyn
  open Exceptions
  open Lexing
  open Typing

  (*type var_name_generator = { mutable next : int }
  let vg = ( {next = 0;} )
  let new_var () =
    let number = vg.next in
    let () = vg.next <- vg.next+1 in
    Int.to_string number*)

  let meta_name n ln = match n with
    | "x" -> PlayerX
    | "y" -> PlayerY
    | "bombs" -> PlayerBombs
    | "shots" -> PlayerShots
    | "board_x" -> BoardX
    | "board_y" -> BoardY
    | _ -> raise (Failure(Some ln, "Unknown meta reference"))

%}
%token <int> CSTINT
%token <string> NAME
%token <string> LABEL
%token LPAR RPAR LBRACE RBRACE LBRAKE RBRAKE
%token PLUS MINUS TIMES EQ NEQ LT GT LTEQ GTEQ
%token LOGIC_AND LOGIC_OR PIPE FSLASH PCT TILDE
%token COMMA SEMI EOF
%token QMARK
%token IF ELSE REPEAT
%token GOTO
%token MOVE FORTIFY WAIT PASS TRENCH
%token NORTH EAST SOUTH WEST BOMB SHOOT CHECK SCAN
%token HASH INT DIR

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
  register_defs stmt* EOF     { 
    try (File ($1,$2)) 
    with
    | Failure _ as failure -> raise failure
    | _ -> (raise (Failure(Some $symbolstartpos.pos_lnum, "Parser error")))
  }
;

register_defs:
  {[]}
  | LBRAKE seperated(COMMA,register) RBRAKE { $2 }
;

register:
  | NAME { Register(T_Int, $1, Int 0) }
  | NAME EQ const_value { Register(type_value [] $3, $1, $3) }
  | INT NAME { Register(T_Int, $2, Int 0) }
  | INT NAME EQ const_value { Register(T_Int, $2, $4) }
  | DIR NAME { Register(T_Dir, $2, Int 0) }
  | DIR NAME EQ const_value { Register(T_Dir, $2, $4) }
;

block:
  LBRACE stmt* RBRACE    { Block $2 }
;

const_value:
  | CSTINT      { Int $1 }
  | direction   { Direction $1}
  | error { raise (Failure(Some $symbolstartpos.pos_lnum, "Expected a constant value")) }
;

simple_value:
  | const_value                        { $1 }
  | QMARK                              { Random }
  | LBRAKE seperated(FSLASH, const_value) RBRAKE         { RandomSet $2 }
  | MINUS simple_value                 { Binary_op ("-", Value (Int 0), $2) } %prec TILDE
  | TILDE simple_value                 { Unary_op ("~", $2) }
  | NAME                               { Reference $1 }
  | HASH NAME                          { MetaReference (meta_name $2 $symbolstartpos.pos_lnum) }
  | LPAR value RPAR                    { $2 }
;

value:
  | simple_value        { $1 }
  | SCAN value          { Scan $2 }
  | CHECK value         { Check $2 }
  | value binop value   { Binary_op ($2, $1, $3) }
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
  | IF LPAR value RPAR stmt1 ELSE stmt2       { If ($3, $5, $7) }
  | IF LPAR value RPAR stmt                   { If ($3, $5, Stmt(Block [], $symbolstartpos.pos_lnum)) }
;

/* No unbalanced if-else */
stmt1:
  stmt1_inner { Stmt($1,$symbolstartpos.pos_lnum) }
;
stmt1_inner: 
  | block                                     { $1 }
  | IF LPAR value RPAR stmt1 ELSE stmt1       { If ($3, $5, $7) }
  | GOTO NAME SEMI                            { GoTo $2 }
  | LABEL                                     { Label $1 }
  | REPEAT LPAR CSTINT RPAR stmt1             { Repeat($3, $5) }
  | non_control_flow_stmt SEMI                { $1 }
;

non_control_flow_stmt:
  | NAME EQ value        { Assign ($1, $3) }
  | NAME PLUS EQ value   { Assign ($1, Value(Binary_op("+", Reference $1, $4))) }
  | NAME MINUS EQ value  { Assign ($1, Value(Binary_op("-", Reference $1, $4))) }
  | NAME TIMES EQ value  { Assign ($1, Value(Binary_op("*", Reference $1, $4))) }
  | NAME TILDE EQ value  { Assign ($1, Value(Unary_op("~", $4))) }
  | MOVE value                        { Move $2 }
  | SHOOT value                       { Shoot $2 }
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