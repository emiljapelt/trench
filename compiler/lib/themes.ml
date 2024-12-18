
module StringSet = Set.Make(String)

let required_resources theme = match theme with
  | "basic" -> StringSet.of_list ["bombs"; "shots";]
  | _ -> StringSet.empty

let all_required_resources themes = 
  themes
  |> List.map required_resources
  |> List.fold_left StringSet.union StringSet.empty