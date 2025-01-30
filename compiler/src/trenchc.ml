
let () = Printexc.record_backtrace true
    

let handle_cmd_line argv = 
  Array.fold_left (fun _ arg -> match arg with 
    | _ -> Some arg
  ) None argv

let () = Trenchclib.Compile.set_features Trenchclib.Features.all_features
let () = Trenchclib.Compile.set_themes Trenchclib.Themes.all_themes

let () = match handle_cmd_line Sys.argv with
  | None -> Printf.printf "No argument given"
  | Some path -> 
    if String.ends_with ~suffix:"g" path then 
      match Trenchclib.Compile.compile_game_file path with
      | Ok _ -> Printf.printf "Game file compiled\n" 
      | Error msg -> Printf.printf "%s\n" msg
    else if String.ends_with ~suffix:"r" path then
      match Trenchclib.Compile.compile_player_file path with
      | Ok p -> List.iter (Printf.printf "%i ") p
      | Error msg -> Printf.printf "%s\n" msg
    else Printf.printf "Dont know what to do with: %s\n" path