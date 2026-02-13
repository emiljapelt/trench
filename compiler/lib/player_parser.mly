%{
  open Absyn
  open Lexing
  open Field_props
  open Resources
  open Builtins


  (*type var_name_generator = { mutable next : int }
  let vg = ( {next = 0;} )
  let new_var () =
    let number = vg.next in
    let () = vg.next <- vg.next+1 in
    Int.to_string number*)

%}
%token <int> CSTINT
%token <string> NAME
%token <string> RESOURCE_NAME
%token <string> FIELD_PROP
%token LPAR RPAR LBRACE RBRACE LBRAKE RBRAKE
%token PLUS MINUS TIMES EQ NEQ LT GT LTEQ GTEQ EQEQ
%token LOGIC_AND LOGIC_OR FSLASH PCT EXCLAIM
%token COMMA SEMI COLON EOF
%token QMARK PLUSPLUS MINUSMINUS
%token IF ELSE IS REPEAT WHILE CONTINUE BREAK LET
%token GOTO RARROW ANY
%token NORTH EAST SOUTH WEST
%token INT DIR FIELD RESOURCE L_SHIFT R_SHIFT
%token RETURN NULL
%token TIMES_EQ MINUS_EQ PLUS_EQ L_SHIFT_EQ R_SHIFT_EQ

// Precedence and assosiativity inspired by https://en.cppreference.com/w/c/language/operator_precedence.html

/*Low precedence*/
%left RARROW
%right QMARK COLON
%left LOGIC_OR
%left LOGIC_AND
%left EQEQ NEQ IS ANY
%left GT LT GTEQ LTEQ
%left L_SHIFT R_SHIFT
%left PLUS MINUS 
%left TIMES FSLASH PCT
%right UNARY
%left LPAR 
%left PLUSPLUS MINUSMINUS LBRAKE
/*High precedence*/

%start main
%type <int Absyn.file> main
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
  stmt* EOF     { (File($1, 0)) }
;

simple_typ:
  | INT { T_Int }
  | DIR { T_Dir }
  | FIELD { T_Field }
  | RESOURCE { T_Resource }
  | LPAR typ RPAR { $2 }
  | typ LPAR seperated_or_empty(COMMA, typ) RPAR { T_Func($1, $3) }
;

typ:
  | typ LBRAKE CSTINT RBRAKE { T_Array($1,$3) }
  | simple_typ { $1 }
;

block:
  LBRACE stmt* RBRACE    { Block $2 }
;

//const_expression:
//  | const_expr { Expr($1, $symbolstartpos.pos_lnum) }
//;

const_expr:
  | CSTINT        { Int $1 }
  | NULL          { Null }
  | direction     { Direction $1 }
  | FIELD_PROP    { Prop(string_to_prop $1) }
  | RESOURCE_NAME { Resource(string_to_resource $1) }
;

simple_expression:
  | simple_expr { Expr($1, $symbolstartpos.pos_lnum) }
;

simple_expr:
  | const_expr                              { $1 }
  | QMARK                                   { features ["random"] ; Random }
  | QMARK LBRAKE simple_expression+ RBRAKE  { features ["random"] ; RandomSet $3 }
  | NAME                                    { features ["memory"] ; VarAccess $1 }
  | LBRAKE seperated_or_empty(COMMA, expression) RBRAKE { ArrayLiteral $2 }
  | LPAR expr RPAR                          { $2 }
;

expression:
  | expr { Expr($1,$symbolstartpos.pos_lnum) }
;

expr:
  | simple_expr                             { $1 }
  | expression LBRAKE expression RBRAKE     { features ["memory"] ; ArrayAccess($1,$3) }
  | MINUS expression                        { Binary_op ("-", Expr(Int 0, $symbolstartpos.pos_lnum), $2) }  %prec UNARY
  | EXCLAIM expression                      { Unary_op ("!", $2) } %prec UNARY
  | expression binop expression                  { Binary_op ($2, $1, $3) }
  | typ COLON LPAR seperated_or_empty(COMMA,func_arg) RPAR block          { features ["func"] ; Func($1, $4, Stmt($6,$symbolstartpos.pos_lnum)) }
  | COLON LPAR seperated_or_empty(COMMA,func_arg) RPAR block              { features ["func";"sugar"] ; Func(T_Int, $3, Stmt($5,$symbolstartpos.pos_lnum)) }
  | typ COLON LPAR seperated_or_empty(COMMA,func_arg) RPAR RARROW expression   { features ["func";"sugar"] ; Func($1, $4, Stmt(Return $7,$symbolstartpos.pos_lnum)) }
  | COLON LPAR seperated_or_empty(COMMA,func_arg) RPAR RARROW expression       { features ["func";"sugar"] ; Func(T_Int, $3, Stmt(Return $6,$symbolstartpos.pos_lnum)) }
  | expression QMARK expression COLON expression      { features ["control";"sugar"] ; Ternary($1,$3,$5) }
  | PLUSPLUS expression                    { features ["sugar"] ; Increment($2, true)} 
  | expression PLUSPLUS                    { features ["sugar"] ; Increment($1, false)}
  | MINUSMINUS expression                  { features ["sugar"] ; Decrement($2, true)} 
  | expression MINUSMINUS                  { features ["sugar"] ; Decrement($1, false)}
  | expression LPAR seperated_or_empty(COMMA, expression) RPAR { Call($1, $3) }
;

func_arg:
  | typ NAME { ($1,$2) }
;

%inline binop:
  | LOGIC_AND   { "&"  }
  | LOGIC_OR    { "|"  }
  | EQEQ        { "="  }
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
  | L_SHIFT     { "<<" }
  | R_SHIFT     { ">>" }
  | IS          { "is" }
  | ANY         { "any" }
;

stmt:
  | stmt1 { $1 }
  | stmt2 { $1 }
;

stmt2:
  stmt2_inner { Stmt($1,$symbolstartpos.pos_lnum) }
;
stmt2_inner:
  | IF simple_expression stmt1 ELSE stmt2         { features ["control"] ; If ($2, $3, $5) }
  | IF simple_expression stmt1                    { features ["control"] ; If ($2, $3, Stmt(Block [], $symbolstartpos.pos_lnum)) }
  | IF simple_expression alt+                     { features ["control"; "sugar"] ; IfIs($2, $3, None)}
  | IF simple_expression alt+ ELSE stmt2          { features ["control"; "sugar"] ; IfIs($2, $3, Some $5) }
;

/* No unbalanced if-else */
stmt1:
  stmt1_inner { Stmt($1,$symbolstartpos.pos_lnum) }
;
stmt1_inner: 
  | block                                          { $1 }
  | expression SEMI                                { ExprStmt $1}
  | non_control_flow_stmt SEMI                     { $1 }
  | IF simple_expression stmt1 ELSE stmt1          { features ["control"] ; If ($2, $3, $5) }
  | IF simple_expression alt+ ELSE stmt1           { features ["control"; "sugar"] ; IfIs($2, $3, Some $5) }
  | WHILE simple_expression stmt1                  { features ["loops"] ; While($2,$3,None) }
  | WHILE simple_expression COLON LPAR expression RPAR stmt1     { features ["loops"; "sugar"] ; While($2,$7,Some(Stmt(ExprStmt $5, $symbolstartpos.pos_lnum))) }
  | BREAK SEMI                                { features ["loops"] ; Break }
  | CONTINUE SEMI                             { features ["loops"] ; Continue }
  | GOTO NAME SEMI                            { GoTo $2 }
  | NAME COLON                                { Label $1 }
  | REPEAT CSTINT stmt1                       { features ["loops"] ; Block(List.init $2 (fun _ -> $3)) }
  | REPEAT LPAR CSTINT RPAR stmt1             { features ["loops"] ; Block(List.init $3 (fun _ -> $5)) }
  | RETURN expression SEMI                    { features ["func"] ; Return $2 }
;

alt:
  | IS simple_expression stmt1   { ($2,$3) }
;

non_control_flow_stmt:
  | expression EQ expression          { features ["memory"] ; Assign ($1, $3) }
  | expression PLUS_EQ expression     { features ["memory"; "sugar"] ; Assign ($1, Expr(Binary_op("+", $1, $3), $symbolstartpos.pos_lnum)) }
  | expression MINUS_EQ expression    { features ["memory"; "sugar"] ; Assign ($1, Expr(Binary_op("-", $1, $3), $symbolstartpos.pos_lnum)) }
  | expression TIMES_EQ expression    { features ["memory"; "sugar"] ; Assign ($1, Expr(Binary_op("*", $1, $3), $symbolstartpos.pos_lnum)) }
  | expression L_SHIFT_EQ expression  { features ["memory"; "sugar"] ; Assign ($1, Expr(Binary_op("<<", $1, $3), $symbolstartpos.pos_lnum)) }
  | expression R_SHIFT_EQ expression  { features ["memory"; "sugar"] ; Assign ($1, Expr(Binary_op(">>", $1, $3), $symbolstartpos.pos_lnum)) }
  | typ NAME                      { features ["memory"] ; Declare($1,$2) }
  | typ NAME EQ expression        { features ["memory"] ; DeclareAssign(Some $1, $2, $4) }
  | LET NAME EQ expression        { features ["memory"; "sugar"] ; DeclareAssign(None, $2, $4) }
;

direction:
  | NORTH { North }
  | EAST  { East  }
  | SOUTH { South }
  | WEST  { West }
;