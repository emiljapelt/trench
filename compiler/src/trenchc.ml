open Trenchclib.Trg
open Trenchclib.Helpers

let () = Printexc.record_backtrace true    

(* Need support for "library" files *)

let handle_cmd_line argv = 
  Array.fold_left (fun _ arg -> match arg with 
    | _ -> Some arg
  ) None argv


let player_to_trg teams (player : Trenchclib.Compile.compiled_player_info) = 
  let (team_name,_,_) = Array.get teams player.team in
  match Trenchclib.Compile.compile_player_file player.file team_name with
  | Error msg -> (Printf.printf "%s\n" msg ; exit 1)
  | Ok program ->
    TRGObject ([
      ("name", TRGString player.name);
      ("team", TRGString team_name);
      ("file", TRGString player.file);
      ("program", TRGArray (Array.map (fun i -> TRGInt i) program |> Array.to_list));
    ] |> Trenchclib.Helpers.StringMap.of_list)

let game_to_trg (game : Trenchclib.Compile.compiled_game_file) =
  TRGObject ([
    ("seed", Option.fold game.seed ~none:TRGNull ~some:(fun i -> TRGInt i));
    ("players", TRGArray(Array.map (player_to_trg game.teams) game.player_info |> Array.to_list));
    ("feed", TRGBool game.feed);
    ("feed_width", TRGInt game.feed_width);
    ("features", TRGArray(Trenchclib.Flags.compile_flags.features |> StringSet.to_list |> List.map (fun f -> TRGString f)));
    ("themes", TRGArray(Trenchclib.Flags.compile_flags.themes |> StringSet.to_list |> List.map (fun f -> TRGString f)));
  ] |> Trenchclib.Helpers.StringMap.of_list)

let print_compiled_game = game_to_trg >> pretty_print >> Printf.printf "%s\n"



let () = match handle_cmd_line Sys.argv with
  | None -> (Printf.printf "No argument given" ; exit 1)
  | Some path -> (match Filename.extension path with
    | ".tr" -> (
      let () = Trenchclib.Flags.set_features Trenchclib.Features.all_features in
      let () = Trenchclib.Flags.set_themes Trenchclib.Themes.all_themes in
      match Trenchclib.Compile.compile_player_file path "" with
      | Ok p -> Array.iter (Printf.printf "%i ") p
      | Error msg -> (Printf.printf "%s\n" msg ; exit 1)
    )
    | ".trg" -> (match Trenchclib.Compile.compile_game_file path with
      | Ok game -> ( Printf.printf "Game file compiled\n" ; print_compiled_game game )
      | Error msg -> (Printf.printf "%s\n" msg ; exit 1))
    | _ -> (Printf.printf "Dont know what to do with: %s\n" path ; exit 1)
  )