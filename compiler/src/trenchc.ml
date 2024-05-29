
let () = Printexc.record_backtrace true
    
let () = Printf.printf "%s\n%!" (Trenchclib.Compile.compile_file Sys.argv.(1))