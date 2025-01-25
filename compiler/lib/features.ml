module StringSet = Set.Make(String)

let all_features = [
  "random";
  "memory";
  "comms";
  "loops";
  "control";
  "sugar";
] |> StringSet.of_list