type test = string * (unit -> string list)
type test_group = string * test list

type test_result = string * string list
type test_group_result = test_result list

let fail = Printf.sprintf

let run ((name, tests) : test_group) = 
  let rec aux tests acc = match tests with
  | [] -> acc
  | (n,f)::t -> 
    let res = f () in
    aux t (
      if res = [] then acc else ((name^"."^n, f ()) :: acc)
    )
  in
  let print results =
    if results = [] then Printf.printf "No failures\n"
    else List.iter (fun (name, errors) -> Printf.printf "%s:\n%s" name (errors |> List.map (Printf.sprintf "\t%s\n") |> String.concat "")) results
  in
  aux tests [] |> print 