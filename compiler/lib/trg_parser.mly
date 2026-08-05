%{
  open Trg
  open Helpers
%}
%token <int> CSTINT
%token <float> CSTFLOAT
%token <string> CSTSTRING
%token <string> CSTNAME
%token LPAR RPAR LBRACE RBRACE LBRAKE RBRAKE DOT MINUS
%token COMMA SEMI COLON EOF TRUE FALSE
/*Low precedence*/

/*High precedence*/

%start main
%type <trg> main
%%

main:
  | obj_entry* EOF { TRGObject(StringMap.of_list $1) }
;

element:
  | CSTSTRING { TRGString $1 }
  | CSTINT { TRGInt $1 }
  | CSTFLOAT { TRGFloat $1 }
  | TRUE { TRGBool true }
  | FALSE { TRGBool false }
  | obj { $1 }
  | array { $1 }
;

obj:
  | LBRACE obj_entry* RBRACE { TRGObject(StringMap.of_list $2) }
;

obj_entry:
  | CSTNAME COLON element SEMI? { ($1,$3) }
;

array:
  | LBRAKE element* RBRAKE { TRGArray $2 }
;