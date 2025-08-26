%{
  open Absyn
  open Exceptions
  open Lexing
  open Flags
  open Field_props

  (*type var_name_generator = { mutable next : int }
  let vg = ( {next = 0;} )
  let new_var () =
    let number = vg.next in
    let () = vg.next <- vg.next+1 in
    Int.to_string number*)

  let themeing ts =
    if List.exists (fun t -> StringSet.mem t compile_flags.themes) ts 
    then ()
    else raise_failure ("Attempt to access a feature of an inactive theme: " ^ (String.concat ", " ts))

  let features fs = 
    if List.for_all (fun f -> StringSet.mem f compile_flags.features) fs
    then ()
    else raise_failure ("Attempt to access an inactive feature: " ^ (String.concat ", " fs))

  let meta_name n (*fn ln*) = match n with
    | "x" -> PlayerX
    | "y" -> PlayerY
    | "board_x" -> BoardX
    | "board_y" -> BoardY
    | "id" -> PlayerID
    | _ -> PlayerResource(n)
    (*| _ -> raise (Failure(Some fn, Some ln, "Unknown meta reference"))*)

  

%}
%token <int> CSTINT
%token <string> NAME
%token <string> META_NAME
%token LPAR RPAR LBRACE RBRACE LBRAKE RBRAKE
%token PLUS MINUS TIMES EQ NEQ LT GT LTEQ GTEQ
%token LOGIC_AND LOGIC_OR FSLASH PCT EXCLAIM
%token COMMA SEMI COLON EOF
%token QMARK PLUSPLUS MINUSMINUS
%token PAGER_READ PAGER_WRITE PAGER_SET
%token IF ELSE IS REPEAT WHILE CONTINUE BREAK LET
%token GOTO MEDITATE DISPEL DISARM MANA_DRAIN PLANT_TREE BRIDGE
%token MOVE FORTIFY WAIT PASS TRENCH WALL PROJECTION FREEZE FIREBALL BEAR_TRAP
%token NORTH EAST SOUTH WEST BOMB SHOOT LOOK SCAN MINE CHOP COLLECT
%token INT DIR FIELD L_SHIFT R_SHIFT
%token READ WRITE SAY MOUNT DISMOUNT BOAT RETURN

/*Low precedence*/
%left COLON
%left LOGIC_OR
%left LOGIC_AND 
%left EQ NEQ IS
%left GT LT GTEQ LTEQ
%left L_SHIFT R_SHIFT
%left PLUS MINUS 
%left TIMES FSLASH PCT
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

simple_typ:
  | INT { T_Int }
  | DIR { T_Dir }
  | FIELD { T_Field }
  //| LPAR typ RPAR { $2 }
;

typ:
  | typ LBRAKE CSTINT RBRAKE { T_Array($1,$3) }
  | simple_typ COLON LPAR seperated_or_empty(COMMA, simple_typ) RPAR { T_Func($1, $4) }
  | simple_typ { $1 }
;

block:
  LBRACE stmt* RBRACE    { Block $2 }
;

const_value:
  | CSTINT      { Int $1 }
  | direction   { Direction $1 }
;

simple_value:
  | const_value                        { $1 }
  | QMARK                              { features ["random"] ; Random }
  | QMARK LPAR simple_value+ RPAR      { features ["random"] ; RandomSet $3 }
  | MINUS simple_value                 { Binary_op ("-", Int 0, $2) } 
  | EXCLAIM simple_value               { Unary_op ("!", $2) }
  | target                             { Reference($1) }
  | META_NAME                          { MetaReference (meta_name $1) }
  | READ                               { features ["ipc"] ; Read }
  | PAGER_READ                         { features ["ipc"] ; PagerRead }
  | LPAR value RPAR                    { $2 }
;

value:
  | simple_value                       { $1 }
  | SCAN simple_value simple_value     { Scan($2,$3) }
  | LOOK simple_value property         { Look($2,string_to_prop $3) }
  | value binop value                  { Binary_op ($2, $1, $3) }
  | value IS property                  { FieldProp($1, string_to_prop $3) }
  | PLUSPLUS target                    { features ["sugar"] ; Increment($2, true)}
  | target PLUSPLUS                    { features ["sugar"] ; Increment($1, false)}
  | MINUSMINUS target                  { features ["sugar"] ; Decrement($2, true)}
  | target MINUSMINUS                  { features ["sugar"] ; Decrement($1, false)}
  | simple_typ COLON LPAR seperated_or_empty(COMMA,func_arg) RPAR stmt           { features ["func"] ; Func($1, $4, $6) }
  | simple_value LPAR seperated_or_empty(COMMA, value) RPAR { features ["func"] ; Call($1, $3) }
  | simple_value QMARK value COLON value { features ["control";"sugar"] ; Ternary($1,$3,$5) }
;

func_arg:
  | simple_typ NAME { ($1,$2) }
;

%inline binop:
  | LOGIC_AND   { "&"  }
  | LOGIC_OR    { "|"  }
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
  | L_SHIFT     { "<<" }
  | R_SHIFT     { ">>" }
;

stmt:
  | stmt1 { $1 }
  | stmt2 { $1 }
;

stmt2:
  stmt2_inner { Stmt($1,$symbolstartpos.pos_lnum) }
;
stmt2_inner:
  | IF simple_value stmt1 ELSE stmt2         { features ["control"] ; If ($2, $3, $5) }
  | IF simple_value stmt1                    { features ["control"] ; If ($2, $3, Stmt(Block [], $symbolstartpos.pos_lnum)) }
  | IF simple_value alt+                     { features ["control"; "sugar"] ; IfIs($2, $3, None)}
  | IF simple_value alt+ ELSE stmt2          { features ["control"; "sugar"] ; IfIs($2, $3, Some $5) }
;

/* No unbalanced if-else */
stmt1:
  stmt1_inner { Stmt($1,$symbolstartpos.pos_lnum) }
;
stmt1_inner: 
  | block                                     { $1 }
  | IF simple_value stmt1 ELSE stmt1          { features ["control"] ; If ($2, $3, $5) }
  | IF simple_value alt+ ELSE stmt1           { features ["control"; "sugar"] ; IfIs($2, $3, Some $5) }
  | WHILE simple_value stmt1                  { features ["loops"] ; While($2,$3,None) }
  | WHILE simple_value LPAR non_control_flow_stmt RPAR stmt1     { features ["loops"; "sugar"] ; While($2,$6,Some(Stmt($4, $symbolstartpos.pos_lnum))) }
  | BREAK SEMI                                { features ["loops"] ; Break }
  | CONTINUE SEMI                             { features ["loops"] ; Continue }
  | GOTO NAME SEMI                            { GoTo $2 }
  | NAME COLON                                { Label $1 }
  | REPEAT CSTINT stmt1                       { features ["loops"] ; Block(List.init $2 (fun _ -> $3)) }
  | REPEAT LPAR CSTINT RPAR stmt1             { features ["loops"] ; Block(List.init $3 (fun _ -> $5)) }
  | RETURN value SEMI                         { features ["func"] ; Return $2 }
  | non_control_flow_stmt SEMI                { $1 }
;

alt:
  | IS const_value stmt1   { ($2,$3) }
;

target:
  | NAME { features ["memory"] ; Local $1 }
  | target LBRAKE value RBRAKE  { features ["memory"] ; Array($1,$3)}
;

property:
  | NAME      { $1 }
  | TRENCH    { "trench" }
  | WALL      { "wall" }
  | BRIDGE    { "bridge" }
;

non_control_flow_stmt:
  | target EQ value        { features ["memory"] ; Assign ($1, $3) }
  | target PLUS EQ value   { features ["memory"; "sugar"] ; Assign ($1, Binary_op("+", Reference $1, $4)) }
  | target MINUS EQ value  { features ["memory"; "sugar"] ; Assign ($1, Binary_op("-", Reference $1, $4)) }
  | target TIMES EQ value  { features ["memory"; "sugar"] ; Assign ($1, Binary_op("*", Reference $1, $4)) }
  | target EXCLAIM EQ value  { features ["memory"; "sugar"] ; Assign ($1, Unary_op("!", $4)) }
  | target L_SHIFT EQ value  { features ["memory"; "sugar"] ; Assign ($1, Binary_op("<<", Reference $1, $4)) }
  | target R_SHIFT EQ value  { features ["memory"; "sugar"] ; Assign ($1, Binary_op(">>", Reference $1, $4)) }
  | typ NAME               { features ["memory"] ; Declare($1,$2) }
  | typ NAME EQ value      { features ["memory"] ; DeclareAssign(Some $1, $2, $4) }
  | LET NAME EQ value      { features ["memory"; "sugar"] ; DeclareAssign(None, $2, $4) }
  | target PLUSPLUS        { features ["memory"; "sugar"] ; Assign ($1, Binary_op("+", Reference $1, Int 1)) }
  | PLUSPLUS target        { features ["memory"; "sugar"] ; Assign ($2, Binary_op("+", Reference $2, Int 1)) }
  | target MINUSMINUS      { features ["memory"; "sugar"] ; Assign ($1, Binary_op("-", Reference $1, Int 1)) }
  | MINUSMINUS target      { features ["memory"; "sugar"] ; Assign ($2, Binary_op("-", Reference $2, Int 1)) }
  | MOVE value                        { Directional(Move, $2) }
  | FORTIFY value?                    { OptionDirectional(Fortify, $2) }
  | TRENCH value?                     { OptionDirectional(Trench, $2) }
  | WALL value                        { Directional(Wall, $2) }
  | CHOP value                        { themeing ["forestry"] ; Directional(Chop, $2) }
  | PLANT_TREE value                  { themeing ["forestry"] ; Directional(PlantTree, $2) }
  | WAIT                              { Unit(Wait) }
  | PASS                              { Unit(Pass) }
  | WRITE value                       { features ["ipc"] ; Write $2 }
  | PAGER_WRITE value                 { features ["ipc"] ; PagerWrite $2 }
  | PAGER_SET value                   { features ["ipc"] ; PagerSet $2 }
  | SHOOT value                       { themeing ["military"; "forestry"] ; Directional(Shoot, $2) }
  | MINE value                        { themeing ["military"] ; Directional(Mine, $2) }
  | BOMB simple_value simple_value    { themeing ["military"] ; Targeting(Bomb, $2, $3) }
  | FREEZE simple_value simple_value  { themeing ["wizardry"] ; Targeting(Freeze, $2, $3) }
  | FIREBALL value                    { themeing ["wizardry"] ; Directional(Fireball, $2) }
  | PROJECTION                        { themeing ["wizardry"] ; Unit(Projection) }
  | MEDITATE                          { themeing ["wizardry"] ; Unit(Meditate) }
  | DISPEL value                      { themeing ["wizardry"] ; Directional(Dispel, $2) }
  | DISARM value                      { themeing ["forestry"; "military"] ; Directional(Disarm, $2)}
  | MANA_DRAIN value                  { themeing ["wizardry"] ; Directional(ManaDrain, $2) }
  | BRIDGE value                      { Directional(Bridge, $2) }
  | COLLECT value?                    { OptionDirectional(Collect, $2) }
  | SAY value                         { Say $2 }
  | MOUNT value                       { Directional(Mount, $2) }
  | DISMOUNT value                    { Directional(Dismount, $2) }
  | BOAT value                        { Directional(Boat, $2) }
  | BEAR_TRAP value                   { themeing ["forestry"] ; Directional(BearTrap, $2) }
  | target LPAR seperated_or_empty(COMMA, value) RPAR { features ["func"] ; CallStmt(Reference $1, $3) }
;

direction:
  | NORTH { North }
  | EAST  { East  }
  | SOUTH { South }
  | WEST  { West }
;