{
  open Player_parser
  open Exceptions
  let keyword_table = Hashtbl.create 53
  let () = List.iter (fun (kwd, tok) -> Hashtbl.add keyword_table kwd tok)
                      [ 
                        "if", IF;
                        "else", ELSE;
                        "is", IS;
                        "repeat", REPEAT;
                        "while", WHILE;
                        "break", BREAK;
                        "continue", CONTINUE;
                        "N", NORTH;
                        "E", EAST;
                        "S", SOUTH;
                        "W", WEST;
                        "goto", GOTO;
                        "type", TYPE;
                        "let", LET;
                        "return", RETURN;
                        "null", NULL;
                        "any", ANY;
                        "const", CONST;
                      ] 
  
  let char_of_string s lexbuf = match s with
  | "\'\\n\'" -> '\n'
  | "\'\\t\'" -> '\t'
  | "\'\\\\'" -> '\\'
  | _ when s.[1] = '\\' -> raise (Failure (Some((Lexing.lexeme_start_p lexbuf).pos_fname), Some((Lexing.lexeme_start_p lexbuf).pos_lnum), ("Unknown escape character: " ^ s)))
  | _ -> s.[1]

  let set_filename filename lexbuf =
    let pos = lexbuf.Lexing.lex_curr_p in
    lexbuf.Lexing.lex_curr_p <- { pos with
      Lexing.pos_fname = filename;
    }
}

rule lex = parse
        [' ' '\t']               { lex lexbuf }
    |   ('\r''\n' | '\n')        { Lexing.new_line lexbuf ; lex lexbuf }
    |   "//" [^ '\n' '\r']* ('\r''\n' | '\n' | eof)       { Lexing.new_line lexbuf ; lex lexbuf }
    |   "/*"  { comment lexbuf ; lex lexbuf }
    |   ['0'-'9']+ as lxm { CSTINT (int_of_string lxm) }
    |   '#' ['A'-'Z' 'a'-'z' ''' '_'] ['A'-'Z' 'a'-'z' '0'-'9' '_'] * as id { RESOURCE_NAME (String.sub id 1 (String.length id - 1)) }
    |   '@' ['A'-'Z' 'a'-'z' ''' '_'] ['A'-'Z' 'a'-'z' '0'-'9' '_'] * as id { FIELD_PROP (String.sub id 1 (String.length id - 1)) }
    |   ['A'-'Z' 'a'-'z' ''' '_'] ['A'-'Z' 'a'-'z' '0'-'9' '_'] * as id
                { try
                    Hashtbl.find keyword_table id
                  with Not_found -> NAME id }
    |   "+="          { PLUS_EQ }
    |   "-="          { MINUS_EQ }
    |   "*="          { TIMES_EQ }
    |   "<<="         { L_SHIFT_EQ }
    |   ">>="         { R_SHIFT_EQ }
    |   "++"          { PLUSPLUS }
    |   '+'           { PLUS }
    |   "--"          { MINUSMINUS }
    |   '-'           { MINUS }
    |   '*'           { TIMES }
    |   '\\'          { BSLASH }
    |   '/'           { FSLASH }
    |   '.'           { DOT }
    |   ".."          { DOTDOT }
    |   '%'           { PCT }
    |   '='           { EQ }
    |   "=="          { EQEQ }
    |   "!="          { NEQ }
    |   "<="          { LTEQ }
    |   "<<"          { L_SHIFT }
    |   "<"           { LT }
    |   ">="          { GTEQ }
    |   ">>"          { R_SHIFT }
    |   ">"           { GT }
    |   "=>"          { RARROW }
    |   "&&"          { LOGIC_AND }
    |   "||"          { LOGIC_OR }
    |   "|"           { PIPE }
    |   '?'           { QMARK }
    |   '!'           { EXCLAIM }
    |   '('           { LPAR }
    |   ')'           { RPAR }
    |   '{'           { LBRACE }
    |   '}'           { RBRACE }
    |   '['           { LBRAKE }
    |   ']'           { RBRAKE }
    |   ','           { COMMA }
    |   ';'           { SEMI }
    |   ':'           { COLON }
    |   _             { raise (Failure(Some((Lexing.lexeme_start_p lexbuf).pos_fname), Some((Lexing.lexeme_start_p lexbuf).pos_lnum), ("Unknown token"))) }
    |   eof           { EOF }

and start filename = parse
       "" { set_filename filename lexbuf ; lex lexbuf }

and comment = parse
  | "/*"    { comment lexbuf; comment lexbuf }
  | "*/"    { () }
  |   ('\r''\n' | '\n')        { Lexing.new_line lexbuf ; comment lexbuf }
  | (eof | '\026')   { raise (Failure(Some((Lexing.lexeme_start_p lexbuf).pos_fname), Some((Lexing.lexeme_start_p lexbuf).pos_lnum), ("Unterminated comment"))) }
  | _ { comment lexbuf }