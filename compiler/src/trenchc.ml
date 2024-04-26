open Trenchclib.ToProgramRep
open Str
open Trenchclib.Exceptions

let () = Printexc.record_backtrace true


let check_input input =
  try (
    if not (Sys.file_exists input) then (Printf.printf "%s\n" input; raise_failure "Input file does not exist")
    else if Str.string_match (regexp {|^\(\.\.?\)?\/?\(\([a-zA-Z0-9_-]+\|\(\.\.?\)\)\/\)*[a-zA-Z0-9_-]+\.tr$|}) input 0 then ()
    else raise_failure "Invalid input file extension"
  ) with
  | ex -> raise ex

let print_line ls l =
  Printf.printf "%i | %s\n" (l+1) (List.nth ls l)

let read_file path =
  let file = open_in path in
  let content = really_input_string (file) (in_channel_length file) in
  let () = close_in_noerr file in
  content
    
let () = try (
  let _ = check_input Sys.argv.(1) in
  let program = compile Sys.argv.(1) (fun file -> Trenchclib.Parser.main (Trenchclib.Lexer.start file) (Lexing.from_string (read_file file))) in
  Printf.printf "%s\n%!" (Trenchclib.ProgramRep.to_string program)
) with 
| Failure(file_opt, line_opt, expl) -> (
  Printf.printf "%s" expl ;
  if Option.is_some file_opt then (
    Printf.printf " in:\n%s" (Option.get file_opt) ;
    if Option.is_some line_opt then (
      Printf.printf ", line %i: \n" (Option.get line_opt) ;
      let line = Option.get line_opt in
      let lines = String.split_on_char '\n' (read_file (Option.get file_opt)) in
      let printer =  print_line lines in match line with
      | 1 -> printer 0 ; printer 1
      | n when n = (List.length lines)-1 -> printer (n-2) ; printer (n-1)
      | _ ->  printer (line-2) ; printer (line-1) ; printer line
    )
    else Printf.printf "\n" ;
  )
  else Printf.printf "\n" ;
)