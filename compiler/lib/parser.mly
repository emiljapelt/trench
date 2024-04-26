%{
  open Absyn
  open Exceptions
  open Lexing

  (*type var_name_generator = { mutable next : int }
  let vg = ( {next = 0;} )
  let new_var () =
    let number = vg.next in
    let () = vg.next <- vg.next+1 in
    Int.to_string number*)

%}
%token <int> CSTINT
%token <string> NAME
%token <string> LABEL
%token ASSIGNMENT
%token LPAR RPAR LBRACE RBRACE LBRAKE RBRAKE
%token PLUS MINUS TIMES EQ NEQ LT GT LTEQ GTEQ
%token LOGIC_AND LOGIC_OR PIPE NOT FSLASH PCT TILDE
%token COMMA DOT SEMI COLON EOF
%token QMARK
%token IF ELSE REPEAT
%token BREAK CONTINUE GOTO
%token CONST MOVE FORTIFY EXPAND
%token NORTH EAST SOUTH WEST BOMB CHECK SCAN
%token HASH UNDERSCORE

/*Low precedence*/
%left LOGIC_AND LOGIC_OR
%left EQ NEQ
%left GT LT GTEQ LTEQ
%left PLUS MINUS
%left TIMES FSLASH PCT
%nonassoc NOT TILDE
/*High precedence*/

%start main
%type <Absyn.file> main
%%
main:
  register_defs stmt_list EOF     { 
    try (File ($1,$2)) 
    with
    | Failure _ as failure -> raise failure
    | _ -> (raise (Failure(Some $symbolstartpos.pos_fname, Some $symbolstartpos.pos_lnum, "Parser error")))
  }
;

register_defs:
  LBRAKE registers RBRAKE { $2 }
;

registers:
  {[]}
  | register { [$1] }
  | register COMMA registers { $1 :: $3 }
;

register:
  | NAME { Register(false, $1, Int 0) }
  | NAME ASSIGNMENT const_value { Register(true, $1, $3) }
  | CONST NAME ASSIGNMENT const_value { Register(true, $2, $4) }
;

block:
  LBRACE stmt_list RBRACE    { Block $2 }
;

expression:
    NAME                                    { Reference $1 }
  | value                                   { Value $1 }
;

expression_not_ternary:
    NAME                                    { Reference $1 }
  | value                                   { Value $1 }
  | LPAR expression RPAR                    { $2 }
;

const_value:
  | CSTINT                                                { Int $1 }
  | error { raise (Failure(Some $symbolstartpos.pos_fname, Some $symbolstartpos.pos_lnum, "Expected a constant value")) }
;

simple_value:
    LPAR value RPAR                                       { $2 }
  | const_value                                           { $1 }
  | MINUS expression_not_ternary                          { Binary_op ("-", Value (Int 0), $2) } %prec NOT
  | NOT expression_not_ternary                            { Unary_op ("!", $2) }
;

value:
    simple_value { $1 }
  | expression_not_ternary binop expression_not_ternary { Binary_op ($2, $1, $3) }
;

%inline binop:
    LOGIC_AND   { "&" }
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
  | TILDE       { "~"  }
;

stmt_list:
    { [] }
  | stmt stmt_list     { $1 :: $2 }
;

stmt:
    stmt1 { $1 }
  | stmt2 { $1 }
;

stmt2:
    IF LPAR expression RPAR stmt1 ELSE stmt2       { If ($3, $5, $7) }
  | IF LPAR expression RPAR stmt                   { If ($3, $5, Block []) }
;

stmt1: /* No unbalanced if-else */
    block                                          { $1 }
  | IF LPAR expression RPAR stmt1 ELSE stmt1       { If ($3, $5, $7) }
  | GOTO NAME SEMI                                 { GoTo $2 }
  | LABEL                                  { Label $1 }
  | non_control_flow_stmt SEMI { $1 }
;

non_control_flow_stmt:
    NAME ASSIGNMENT expression        { Assign ($1, $3) }
  | NAME PLUS ASSIGNMENT expression   { Assign ($1, Value(Binary_op("+", Reference $1, $4))) }
  | NAME MINUS ASSIGNMENT expression  { Assign ($1, Value(Binary_op("-", Reference $1, $4))) }
  | NAME TIMES ASSIGNMENT expression  { Assign ($1, Value(Binary_op("*", Reference $1, $4))) }
  | NAME NOT ASSIGNMENT expression    { Assign ($1, Value(Unary_op("!", $4))) }
  | MOVE direction                         { Move $2 }
  | EXPAND direction                       { Expand $2 }
  | FORTIFY                                { Fortify }
  | SCAN direction                         { Scan $2 }
  | CHECK direction                        { Check $2 }
  | BOMB LPAR expression COMMA expression RPAR                   { Bomb($3, $5) }
;

direction:
  NORTH { North }
  | EAST { East  }
  | SOUTH { South }
  | WEST { West }
;