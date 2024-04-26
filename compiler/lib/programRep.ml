
module StringMap = Map.Make(String)
module StringSet = Set.Make(String)

type program_part =
  | CLabel of string
  | CGoTo of string * int option
  | IfTrue of string * int option
  | Instruction of string

let num_digits n = ((log10 (n |> float)) |> Int.of_float) + 1
let pp_size pp = match pp with
  | CLabel _ -> 0
  | Instruction i -> String.length i
  | CGoTo(_,Some i)
  | IfTrue(_,Some i) -> 1 + (num_digits i)
  | _ -> failwith "Unsizeable part"

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
  | CGoTo(n,_)::t
  | IfTrue(n,_)::t -> if StringSet.mem n set then check_labels_exist t set else failwith ("Undefined label: "^n)
  | _ ::t -> check_labels_exist t set

let temp_to_string pp = (List.map (
  fun p -> match p with
  | CLabel n-> n^":"(*failwith "Unextracted label"*)
  | CGoTo(_,Some idx) -> "!"^string_of_int idx
  | IfTrue(_, Some idx) -> "?"^string_of_int idx
  | CGoTo(n,_) -> "!"^n
  | IfTrue(n,_) -> "?"^n
  | Instruction i -> i
) pp
|> String.concat "")

let rec attempt_find name idx pps : int option = match pps with
  | [] -> None
  | CLabel n ::t -> if n = name then Some idx else attempt_find name idx t
  | Instruction instr ::t -> attempt_find name (idx + (String.length instr)) t
  | IfTrue(_, Some i) ::t
  | CGoTo(_, Some i) ::t -> attempt_find name (idx + 1 + num_digits i) t
  | IfTrue(n, None) ::t 
  | CGoTo(n, None) ::t -> 
    if n = name then match attempt_find n 0 t with
      | None -> None
      | Some i -> Some(idx + 1 + i)
    else None

let unresolved pp = match pp with
  | IfTrue(_,None)
  | CGoTo(_,None) -> true
  | _ -> false

let add_digits n =
  let digits = num_digits n in
  if num_digits(digits + n) > digits then n+digits+1
  else n + digits


let reconstruct_with_label_index pp i = match pp with
  | IfTrue(n,_) -> IfTrue(n, i)
  | CGoTo(n,_) -> CGoTo(n, i)
  | _ -> pp


let rec resolve_labels pps : program_part list =
  Printf.printf "resolve %s\n" (temp_to_string pps);
  let rec aux pps labels idx acc = match pps with
    | [] -> List.rev acc
    | pp::t -> ( match pp with
      | CLabel n -> ( match idx with
        | Some i -> aux t (StringMap.add n i labels) idx (pp::acc)
        | None -> aux t labels None (pp::acc)
      )
      | Instruction instr -> (match idx with
        | Some i -> aux t labels (Some(i + String.length instr)) (pp::acc)
        | None -> aux t labels None (pp::acc)
      )
      | CGoTo(n,None)
      | IfTrue(n,None) -> (
        let reconstruct = reconstruct_with_label_index pp in
        match StringMap.find_opt n labels with
        | Some label_idx -> ( 
          let next_idx = match idx with
            | Some i -> Some (i + 1 + num_digits label_idx)
            | None -> None
          in  
          aux t labels next_idx (reconstruct (Some label_idx)::acc)
        )
        | None -> ( match idx with
          | Some i -> (match attempt_find n 0 t with
            | Some found_offset -> 
              let label_idx = i + 1 + found_offset in
              let label_idx = add_digits label_idx in
              aux t (StringMap.add n label_idx labels) (Some (i + 1 + num_digits label_idx)) (reconstruct (Some label_idx)::acc)
            | None -> aux t labels None (reconstruct None::acc)
          )
          | None -> aux t labels None (reconstruct None::acc)
        )
      )
      | _ -> aux t labels idx (pp::acc)
    )
  in
  let pps = aux pps StringMap.empty (Some 0) [] in
  match List.exists unresolved pps with
  | true -> resolve_labels pps
  | false -> pps

let to_string pp =
  check_labels_exist pp (label_set pp);
  let pp = resolve_labels pp in
  Printf.printf "result: %s\n" (temp_to_string pp);
  (*string_of_int start ^*) (List.map (
    fun p -> match p with
    | CLabel _ -> ""
    | CGoTo(_,Some idx) -> "!"^string_of_int idx
    | IfTrue(_,Some idx) -> "?"^string_of_int idx
    | Instruction i -> i
    | CGoTo(n,None)
    | IfTrue(n,None) -> failwith ("Unresolved label: "^n)
  ) pp
  |> String.concat "")
