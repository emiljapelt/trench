open Exceptions

module StringMap = Map.Make(String)

type environment = string list

type label_generator = { mutable next : int }


(*let binary_int file w =
  for i = 0 to 3 do
    fprintf file "%c" (Char.chr (Int64.to_int (Int64.logand 255L (Int64.shift_right w (i * 8)))))
  done*)

let binary_int_string i =
  let rec aux i c acc = 
    if c <= 3 then (
      aux i (c+1) (acc ^ ((Int32.to_int(Int32.logand 255l (Int32.shift_right i (c * 8)))) |> Char.chr |> Printf.sprintf "%c"))
    )
    else acc
  in 
  aux (Int32.of_int i) 0 ""

(* Labels *)
let lg = ( {next = 0;} )

let new_label () =
  let number = lg.next in
  let () = lg.next <- lg.next+1 in
  Int.to_string number

(* Lookup *)
let rec lookup f l =
  match l with
  | [] -> None
  | h::t -> ( match f h with
    | None -> lookup f t
    | a -> a
  )

let lookup_i f l =
  let rec aux l i =
    match l with
    | [] -> None
    | h::t -> ( match f i h with
      | None -> aux t (i-1)
      | a -> a
    )
  in
  aux l ((List.length l)-1)

let struct_field field params =
  let rec aux ps c =
    match ps with
    | [] -> raise_failure ("No such field '" ^ field ^ "'")
    | (vmod,ty,n)::t -> if n = field then (vmod,ty,c) else aux t (c+1)
  in
  aux params 0
