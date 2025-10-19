
module StringSet = Set.Make(String)

let all_themes = [
  "military";
  "wizardry";
  "forestry";
  "pottery";
] |> StringSet.of_list

let required_resources theme = match theme with
  | "military" -> ["explosive"; "ammo";] |> StringSet.of_list
  | "wizardry" -> ["mana";] |> StringSet.of_list
  | "forestry" -> ["sapling";] |> StringSet.of_list
  | "pottery" -> ["clay";] |> StringSet.of_list
  | _ -> StringSet.empty

let default_resources = [
  "wood";
] |> StringSet.of_list

let all_required_resources themes = 
  themes
  |> StringSet.to_list
  |> List.map required_resources
  |> List.cons default_resources
  |> List.fold_left StringSet.union StringSet.empty