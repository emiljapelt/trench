
module StringSet = Set.Make(String)

let all_themes = [
  "basic";
  "military";
  "wizardry";
  "forestry";
] |> StringSet.of_list

let required_resources theme = match theme with
  | "basic" -> ["bomb"; "shot";] |> StringSet.of_list
  | _ -> StringSet.empty

let all_required_resources themes = 
  themes
  |> StringSet.to_list
  |> List.map required_resources
  |> List.fold_left StringSet.union StringSet.empty