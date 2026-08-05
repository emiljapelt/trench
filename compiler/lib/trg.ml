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

let rec tn_string tn = match tn with
    | TRGString s -> "\""^s^"\""
    | TRGInt i -> string_of_int i
    | TRGFloat f -> string_of_float f
    | TRGBool b -> string_of_bool b
    | TRGObject e -> "{"^(e |> StringMap.to_list |> List.map (fun (n,e) -> n ^ ": " ^ tn_string e) |> String.concat ", ")^"}"
    | TRGArray l -> "["^(l |> List.length |> string_of_int)^"]"
    | TRGNull -> "null"

let expected e tn = 
  raise_failure ("Expected '"^e^"' but got '"^tn_string tn^"'")

let find_entry_opt name = function
  | TRGObject entries -> StringMap.find_opt name entries
  | _ -> None

let find_entry name ~default tn =
  find_entry_opt name tn |> Option.value ~default:default




let is_int = function
  | TRGInt _ -> true
  | _ -> false

let load_int = function
  | TRGInt i -> i
  | tn -> expected "int" tn

let load_int_entry name ~default trg = 
    Option.fold ~none:default ~some:load_int (find_entry_opt name trg)



let is_float = function
  | TRGFloat _ -> true
  | _ -> false    

let load_float = function
  | TRGFloat f -> f
  | t -> expected "float" t

let load_float_entry name trg ~default = 
    Option.fold ~none:default ~some:load_float (find_entry_opt name trg)



let is_bool = function
  | TRGBool _ -> true
  | _ -> false

let load_bool tn = match tn with
  | TRGBool b -> b
  | _ -> expected "bool" tn

let load_bool_entry name trg ~default = 
    Option.fold ~none:default ~some:load_bool (find_entry_opt name trg)
  


let is_string = function
  | TRGString _ -> true
  | _ -> false

let load_string = function
  | TRGString s -> s
  | tn -> expected "string" tn

let load_string_entry name trg ~default = 
    Option.fold ~none:default ~some:load_string (find_entry_opt name trg)



let is_null = (=) TRGNull
  
let is_object = function 
  | TRGObject _ -> true
  | _ -> false

let is_array = function 
  | TRGArray _ -> true
  | _ -> false