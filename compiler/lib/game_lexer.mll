{
  open Game_parser
  open Exceptions
  let keyword_table = Hashtbl.create 53
  let () = List.iter (fun (kwd, tok) -> Hashtbl.add keyword_table kwd tok)
                      [ 
                        "player", PLAYER; 
                        "actions", ACTIONS;
                        "steps", STEPS;
                        "mode", MODE;
                        "board", BOARD;
                        "nuke", NUKE;
                        "team", TEAM;
                        "name", NAME;
                        "position", POSITION;
                        "file", FILE;
                        "global_array", GLOBAL_ARRAY;
                        "feature_level", FEATURE_LEVEL;
                        "exec_mode", EXEC_MODE;
                        "sync", SYNC;
                        "async", ASYNC;
                        "themes", THEMES;
                        "resources", RESOURCES;
                        "seed", SEED;
                        "time_scale", TIME_SCALE;
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
    |  '-'? ['0'-'9']+ as lxm { CSTINT (int_of_string lxm) }
    |  '-'? ['0'-'9']+ '.' ['0'-'9']+ as lxm { CSTFLOAT (Float.of_string lxm) }
    |   ['A'-'Z' 'a'-'z' '/' '.']['A'-'Z' 'a'-'z' '0'-'9' '/' '_' '.']* as id
                { try
                    Hashtbl.find keyword_table id
                  with Not_found -> WORD id }
    |   ','           { COMMA }
    |   ':'           { COLON }
    |   ';'           { SEMI }
    |   '('           { LPAR }
    |   ')'           { RPAR }
    |   '{'           { LBRACE }
    |   '}'           { RBRACE }
    |   _             { raise (Failure(Some((Lexing.lexeme_start_p lexbuf).pos_fname), Some((Lexing.lexeme_start_p lexbuf).pos_lnum), ("Unknown token"))) }
    |   eof           { EOF }

and start filename = parse
       "" { set_filename filename lexbuf ; lex lexbuf }
