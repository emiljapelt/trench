

exception Failure of int option * string  (* file, line, explanation *)

let raise_failure msg = raise (Failure (None, msg))