
module StringMap = Map.Make(String)
module StringSet = Set.Make(String)

type program_part =
  | CLabel of string
  | CGoTo of string 
  | IfTrue of string
  | Instruction of string

let label_set pp =
  let rec aux pp set = match pp with
    | [] -> set
    | CLabel n :: t -> ( 
      if StringSet.mem n set then failwith ("Duplicate label: "^n)   
      else aux t (StringSet.add n set)
    )
    | _::t -> aux t set
  in
  aux pp StringSet.empty


let rec check_labels_exist pp set = match pp with
  | [] -> ()
  | CGoTo(n)::t
  | IfTrue(n)::t -> if StringSet.mem n set then check_labels_exist t set else failwith ("Undefined label: "^n)
  | _ ::t -> check_labels_exist t set



let pp_size pp = match pp with
  | CLabel _ -> 0
  | Instruction s -> String.length s
  | IfTrue _ -> 5
  | CGoTo _ -> 5

let extract_label_indecies pps =
  let rec aux pps i map = match pps with
    | [] -> map
    | CLabel n :: t -> aux t i (StringMap.add n i map)
    | h::t -> aux t (i + pp_size h) map
  in
  aux pps 0 StringMap.empty

let program_to_string pp =
  check_labels_exist pp (label_set pp);
  let map = extract_label_indecies pp in
  (List.map (
    fun p -> match p with
    | CLabel _ -> ""
    | CGoTo n -> "!"^(Helpers.binary_int_string (StringMap.find n map))
    | IfTrue n -> "?"^(Helpers.binary_int_string (StringMap.find n map))
    | Instruction i -> i
  ) pp
  |> String.concat "")

