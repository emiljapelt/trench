
module StringMap = Map.Make(String)

type program_part =
  | CLabel of string
  | CGoTo of string
  | IfTrue of string
  | Instruction of string

let extract_labels pp =
  let rec aux pp i acc start lmap = match pp with
    | [] -> ( match start with
      | None -> failwith "No starting label found"
      | Some i -> (List.rev acc, i, lmap)
    )
    | CLabel n :: t -> ( 
      if StringMap.mem n lmap then failwith ("Duplicate label: "^n)   
      else match n, start with
      | "start", None -> aux t i acc (Some i) (StringMap.add n i lmap)
      | _ , _ -> aux t i acc start (StringMap.add n i lmap)
    )
    | h::t -> aux t (i+1) (h::acc) start lmap
  in
  aux pp 0 [] None StringMap.empty

let to_string pp =
  let (pp, start, lmap) = extract_labels pp in
  string_of_int start ^ (List.map (
    fun p -> match p with
    | CLabel _ -> failwith "Unextracted label"
    | CGoTo l -> (match StringMap.find_opt l lmap with
      | Some i -> "!"^string_of_int i
      | None -> failwith ("Undefined label:"^l)
    )
    | IfTrue l -> (match StringMap.find_opt l lmap with
      | Some i -> "?"^string_of_int i
      | None -> failwith ("Undefined label:"^l)
    )
    | Instruction i -> i
  ) pp
  |> String.concat "")
