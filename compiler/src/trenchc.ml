
let () = Printexc.record_backtrace true
    
let path = Sys.argv.(1)

let () = 
  if String.ends_with ~suffix:"g" path then 
    match Trenchclib.Compile.compile_game_file path with
    | Ok _ -> Printf.printf "Game file compiled\n" 
    | Error msg -> Printf.printf "%s\n" msg
  else if String.ends_with ~suffix:"r" path then
    match Trenchclib.Compile.compile_player_file path with
    | Ok p -> Printf.printf "%s\n" p
    | Error msg -> Printf.printf "%s\n" msg
  else Printf.printf "Dont know what to do with: %s" path