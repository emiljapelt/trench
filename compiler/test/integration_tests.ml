open Test_lib

let tests = [
  (
    "compile_game_file", fun () ->
    match Trenchclib.Compile.compile_game_file "./compiler/test/integration/integration.trg" with
    | Error msg -> [msg]
    | Ok compiled -> 
      if compiled.player_count <> 1 then ["Expected 1 player to be compiled"]
      else 
        let player = Array.get compiled.player_info 0 in
        let (team_name,_,_) = Array.get compiled.teams player.team in
        (
          match Trenchclib.Compile.compile_player_file player.file team_name with
          | Error msg -> [msg]
          | Ok program -> (
            let expected = 6 in
            match Array.to_list program with
            | _::_::dec::_ -> if dec = expected then [] else [fail "Program declares global variable space of %i expected %i" dec expected]
            | _ -> ["Something is very wrong"]
          ) 
        )
  )
] 