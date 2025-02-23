module StringSet = Set.Make(String)

let all_features = [
  "random";
  "memory";
  "ipc";
  "loops";
  "control";
  "sugar";
] |> StringSet.of_list