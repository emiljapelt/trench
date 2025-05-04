
module StringSet = Set.Make(String)

let () = Printexc.record_backtrace true    

let handle_cmd_line argv = 
  Array.fold_left (fun _ arg -> match arg with 
    | _ -> Some arg
  ) None argv

let () = Trenchclib.Compile.set_features Trenchclib.Features.all_features
let () = Trenchclib.Compile.set_themes Trenchclib.Themes.all_themes
let () = Trenchclib.Compile.set_resources (Trenchclib.Themes.all_themes |> Trenchclib.Themes.all_required_resources |> StringSet.to_list |> List.map (fun a -> (a,())))

let () = match handle_cmd_line Sys.argv with
  | None -> (Printf.printf "No argument given" ; exit 1)
  | Some path -> 
    if String.ends_with ~suffix:"g" path then 
      match Trenchclib.Compile.compile_game_file path with
      | Ok _ -> Printf.printf "Game file compiled\n" 
      | Error msg -> (Printf.printf "%s\n" msg ; exit 1)
    else if String.ends_with ~suffix:"r" path then
      match Trenchclib.Compile.compile_player_file path with
      | Ok p -> Array.iter (Printf.printf "%i ") p
      | Error msg -> (Printf.printf "%s\n" msg ; exit 1)
    else (Printf.printf "Dont know what to do with: %s\n" path ; exit 1)