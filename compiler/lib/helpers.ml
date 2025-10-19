open Exceptions

module StringMap = Map.Make(String)

type environment = string list

type int_generator = { mutable next : int }

(* Names *)
let ng = ( {next = 0}) 

let rename n =
  let number = ng.next in
  let () = ng.next <- ng.next+1 in
  n ^ "_" ^ Int.to_string number

let reset_rename_generator () = 
  ng.next <- 0

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
