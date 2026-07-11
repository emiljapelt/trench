open Helpers

let all_features = [
  "random";
  "memory";
  "ipc";
  "loops";
  "control";
  "sugar";
  "func";
  "fork";
  "craft";
  "debug";
  "meta"
] |> StringSet.of_list