open Helpers
open Exceptions

type trg =
| TRGNull
| TRGString of string
| TRGInt of int
| TRGFloat of float
| TRGBool of bool
| TRGObject of trg_object
| TRGArray of trg_array

and trg_object = trg StringMap.t

and trg_array = trg list

(* Wrapper for carrying path info and such? *)
type trg_wrapper = {
  value: trg;
  path: string list;
}

let wrap trg = { value = trg ; path = [] }

let rec tn_string tn = match tn with
    | TRGString s -> "\""^s^"\""
    | TRGInt i -> string_of_int i
    | TRGFloat f -> string_of_float f
    | TRGBool b -> string_of_bool b
    | TRGObject e -> "{"^(e |> StringMap.to_list |> List.map (fun (n,e) -> n ^ ": " ^ tn_string e) |> String.concat ", ")^"}"
    | TRGArray l -> "["^(l |> List.length |> string_of_int)^"]"
    | TRGNull -> "null"

let pretty_print trg = 
  let rec aux i trg = match trg with
    | TRGString s -> "\""^s^"\""
    | TRGInt i -> string_of_int i
    | TRGFloat f -> string_of_float f
    | TRGBool b -> string_of_bool b
    | TRGNull -> "null"
    | TRGObject e -> 
      let indent = List.init i (return "  ") |> String.concat "" in
      "{\n"^(e |> StringMap.to_list |> List.map (fun (n,e) -> indent ^ n ^ ": " ^ aux (i+1) e) |> String.concat "\n")^indent^"\n}"
    | TRGArray l -> "["^(l |> List.map (aux i) |> String.concat " ")^"]"
  in
  aux 1 trg

let location tw = tw.path |> List.rev |> String.concat "."

let expected e tw = 
  raise_failure ("Expected "^e^" but got '"^tn_string tw.value^"' at "^location tw)

let entry name tw = match tw.value with
  | TRGObject entries -> { value = entries |> StringMap.find_opt name |> Option.fold ~none:TRGNull ~some:identity ; path = name::tw.path }
  | _ -> expected "an object" tw

let map f tw = match tw.value with
  | TRGArray a -> a |> List.mapi (fun i e -> { value = e ; path = (string_of_int i)::tw.path }) |> List.map f
  | _ -> expected "an array" tw

let mapi f tw = match tw.value with
  | TRGArray a -> a |> List.mapi (fun i e -> { value = e ; path = (string_of_int i)::tw.path }) |> List.mapi f
  | _ -> expected "an array" tw

let filter p tw = match tw.value with
  | TRGArray a -> { value = TRGArray(a |> List.mapi (fun i e -> { value = e ; path = (string_of_int i)::tw.path }) |> List.filter p |> List.map (fun a -> a.value)); path = tw.path} 
  | _ -> expected "an array" tw

let default d tw = match tw.value with
  | TRGNull -> { value = d ; path = []  }
  | _ -> tw

let is_int tw = match tw.value with
  | TRGInt _ -> true
  | _ -> false

let load_int tw = match tw.value with
  | TRGInt i -> i
  | _ -> expected "an int" tw

let optional_load load tw =
  try Some(load tw) with _ -> None



let is_float tw = match tw.value with
  | TRGFloat _ -> true
  | _ -> false    

let load_float tw = match tw.value with
  | TRGFloat f -> f
  | _ -> expected "a float" tw


let is_bool tw = match tw.value with
  | TRGBool _ -> true
  | _ -> false

let load_bool tw = match tw.value with
  | TRGBool b -> b
  | _ -> expected "a boolean" tw


let is_string tw = match tw.value with
  | TRGString _ -> true
  | _ -> false

let load_string tw = match tw.value with
  | TRGString s -> s
  | _ -> expected "a string" tw


let is_null tw = match tw.value with 
  | TRGNull -> true
  | _ -> false
  
let is_object tw = match tw.value with
  | TRGObject _ -> true
  | _ -> false

let is_array tw = match tw.value with
  | TRGArray _ -> true
  | _ -> false