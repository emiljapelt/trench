open Exceptions

module StringSet = Set.Make(String)
module StringMap = Map.Make(String)

let identity a = a

let return a _ = a

type environment = string list

type int_generator = { mutable next : int }

(* Names *)
let ng = ( {next = 0}) 

let curry f a b = f (a,b)
let uncurry f (a,b) = f a b

let (<<) f g x = f(g(x))
let (>>) g f x = f(g(x))


(* Labels *)
type label_context = {
  mutable next: int;
  mutable name: string;
}

let lg = ( {next = 0; name = ""} )

let label_context () = string_of_int lg.next ^ "_" ^ lg.name

let new_label_context name =
  lg.next <- lg.next+1 ; lg.name <- name

let label name = label_context () ^ "_" ^ name


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
