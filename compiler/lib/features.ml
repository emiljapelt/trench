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
  "experimental";
] |> StringSet.of_list

let non_default_features = [
  "debug";
  "asm";
  "experimental";
] |> StringSet.of_list