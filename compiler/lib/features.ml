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
  "meta";
  "asm";
] |> StringSet.of_list