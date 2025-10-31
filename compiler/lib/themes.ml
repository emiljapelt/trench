
module StringSet = Set.Make(String)

let all_themes = [
  "military";
  "wizardry";
  "forestry";
  "pottery";
] |> StringSet.of_list