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
%token MOVE FORTIFY WAIT PASS EXPAND TRENCH
%token NORTH EAST SOUTH WEST BOMB SHOOT CHECK SCAN
%token HASH

/*Low precedence*/
%left LOGIC_AND LOGIC_OR
%left EQ NEQ
%left GT LT GTEQ LTEQ
%left PLUS MINUS
%left TIMES FSLASH PCT
%nonassoc TILDE HASH
/*High precedence*/

%start main
%type <Absyn.file> main
%%
main:
  register_defs stmt_list EOF     { 
    try (File ($1,$2)) 
    with
    | Failure _ as failure -> raise failure
    | _ -> (raise (Failure(Some $symbolstartpos.pos_lnum, "Parser error")))
  }
;

register_defs:
  {[]}
  | LBRAKE registers RBRAKE { $2 }
;

registers:
  {[]}
  | register { [$1] }
  | register COMMA registers { $1 :: $3 }
;

register:
  | NAME { Register($1, Int 0) }
  | NAME EQ const_value { Register($1, $3) }
;

block:
  LBRACE stmt_list RBRACE    { Block $2 }
;

expression:
    NAME                                    { Reference $1 }
  | value                                   { Value $1 }
  | simple_expression                       { $1 }
;

simple_expression:
    NAME                                    { Reference $1 }
  | HASH NAME                               { MetaReference (meta_name $2 $symbolstartpos.pos_lnum) }
  | simple_value                            { Value $1 }
  | LPAR expression RPAR                    { $2 }
;

const_value:
  | CSTINT                                                { Int $1 }
  | error { raise (Failure(Some $symbolstartpos.pos_lnum, "Expected a constant value")) }
;

simple_value:
    LPAR value RPAR                                       { $2 }
  | const_value                                           { $1 }
  | MINUS simple_expression                          { Binary_op ("-", Value (Int 0), $2) } %prec TILDE
  | TILDE simple_expression                          { Unary_op ("~", $2) }
;

value:
    simple_value { $1 }
  | SCAN direction                         { Scan $2 }
  | CHECK direction                        { Check $2 }
  | expression binop expression { Binary_op ($2, $1, $3) }
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
  stmt2_inner { Stmt($1,$symbolstartpos.pos_lnum) }
;
stmt2_inner:
    IF LPAR expression RPAR stmt1 ELSE stmt2       { If ($3, $5, $7) }
  | IF LPAR expression RPAR stmt                   { If ($3, $5, Stmt(Block [], $symbolstartpos.pos_lnum)) }
;

/* No unbalanced if-else */
stmt1:
  stmt1_inner { Stmt($1,$symbolstartpos.pos_lnum) }
;
stmt1_inner: 
    block                                          { $1 }
  | IF LPAR expression RPAR stmt1 ELSE stmt1       { If ($3, $5, $7) }
  | GOTO NAME SEMI                                 { GoTo $2 }
  | LABEL                                  { Label $1 }
  | REPEAT LPAR CSTINT RPAR stmt { Repeat($3, $5) }
  | non_control_flow_stmt SEMI { $1 }
;

non_control_flow_stmt:
    NAME EQ expression        { Assign ($1, $3) }
  | NAME PLUS EQ expression   { Assign ($1, Value(Binary_op("+", Reference $1, $4))) }
  | NAME MINUS EQ expression  { Assign ($1, Value(Binary_op("-", Reference $1, $4))) }
  | NAME TIMES EQ expression  { Assign ($1, Value(Binary_op("*", Reference $1, $4))) }
  | NAME TILDE EQ expression    { Assign ($1, Value(Unary_op("~", $4))) }
  | MOVE direction                         { Move $2 }
  | EXPAND direction                       { Expand $2 }
  | SHOOT direction                        { Shoot $2 }
  | FORTIFY                                { Fortify }
  | TRENCH                                 { Trench }
  | WAIT                                { Wait }
  | PASS                                { Pass }
  | BOMB direction simple_expression    { Bomb($2, $3) }
;

direction:
  NORTH { North }
  | EAST { East  }
  | SOUTH { South }
  | WEST { West }
;