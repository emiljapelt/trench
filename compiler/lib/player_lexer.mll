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
                        "for", FOR;
                        "break", BREAK;
                        "continue", CONTINUE;
                        "move", MOVE;
                        "trench", TRENCH;
                        "wall", WALL;
                        "fortify", FORTIFY;
                        "scan", SCAN;
                        "look", LOOK;
                        "bomb", BOMB;
                        "wait", WAIT;
                        "pass", PASS;
                        "N", NORTH;
                        "E", EAST;
                        "S", SOUTH;
                        "W", WEST;
                        "goto", GOTO;
                        "shoot", SHOOT;
                        "mine", MINE;
                        "chop", CHOP;
                        "int", INT;
                        "dir", DIR;
                        "field", FIELD;
                        "read", READ;
                        "write", WRITE;
                        "pager_read", PAGER_READ;
                        "pager_write", PAGER_WRITE;
                        "pager_set", PAGER_SET;
                        "projection", PROJECTION;
                        "freeze", FREEZE;
                        "fireball", FIREBALL;
                        "meditate", MEDITATE;
                        "dispel", DISPEL;
                        "disarm", DISARM;
                        "mana_drain", MANA_DRAIN;
                        "plant_tree", PLANT_TREE;
                        "bridge", BRIDGE;
                        "collect", COLLECT;
                        "say", SAY;
                      ] 
  
  let char_of_string s lexbuf = match s with
  | "\'\\n\'" -> '\n'
  | "\'\\t\'" -> '\t'
  | "\'\\\\'" -> '\\'
  | _ when s.[1] = '\\' -> raise (Failure (Some((Lexing.lexeme_start_p lexbuf).pos_fname), Some((Lexing.lexeme_start_p lexbuf).pos_lnum), ("Unknown escape character: " ^ s)))
  | _ -> s.[1]

  let incr_linenum lexbuf = 
    let pos = lexbuf.Lexing.lex_curr_p in
    lexbuf.Lexing.lex_curr_p <- { pos with
      Lexing.pos_lnum = pos.Lexing.pos_lnum + 1;
      Lexing.pos_bol = pos.Lexing.pos_cnum;
    }

  let set_filename filename lexbuf =
    let pos = lexbuf.Lexing.lex_curr_p in
    lexbuf.Lexing.lex_curr_p <- { pos with
      Lexing.pos_fname = filename;
    }
}

rule lex = parse
        [' ' '\t']               { lex lexbuf }
    |   ('\r''\n' | '\n')        { incr_linenum lexbuf ; lex lexbuf }
    |   "//" [^ '\n' '\r']* ('\r''\n' | '\n' | eof)       { incr_linenum lexbuf ; lex lexbuf }
    |   ['0'-'9']+ as lxm { CSTINT (int_of_string lxm) }
    |   ['A'-'Z' 'a'-'z' '''] ['A'-'Z' 'a'-'z' '0'-'9' '_'] * ':' as id { LABEL (String.sub id 0 (String.length id - 1)) }
    |   '#' ['A'-'Z' 'a'-'z' '''] ['A'-'Z' 'a'-'z' '0'-'9' '_'] * as id { META_NAME (String.sub id 1 (String.length id - 1)) }
    |   ['A'-'Z' 'a'-'z' '''] ['A'-'Z' 'a'-'z' '0'-'9' '_'] * as id
                { try
                    Hashtbl.find keyword_table id
                  with Not_found -> NAME id }
    |   '+'           { PLUS }
    |   "++"          { PLUSPLUS }
    |   "--"          { MINUSMINUS }
    |   '*'           { TIMES }
    |   '-'           { MINUS }
    |   '/'           { FSLASH }
    |   '%'           { PCT }
    |   '='           { EQ }
    |   "!="          { NEQ }
    |   "<="          { LTEQ }
    |   "<<"          { L_SHIFT }
    |   "<"           { LT }
    |   ">="          { GTEQ }
    |   ">>"          { R_SHIFT }
    |   ">"           { GT }
    |   "&"           { LOGIC_AND }
    |   "|"           { LOGIC_OR }
    |   '?'           { QMARK }
    |   '~'           { TILDE }
    |   '('           { LPAR }
    |   ')'           { RPAR }
    |   '{'           { LBRACE }
    |   '}'           { RBRACE }
    |   '['           { LBRAKE }
    |   ']'           { RBRAKE }
    |   '.'           { DOT }
    |   ','           { COMMA }
    |   ';'           { SEMI }
    |   _             { raise (Failure(Some((Lexing.lexeme_start_p lexbuf).pos_fname), Some((Lexing.lexeme_start_p lexbuf).pos_lnum), ("Unknown token"))) }
    |   eof           { EOF }

and start filename = parse
       "" { set_filename filename lexbuf ; lex lexbuf }
