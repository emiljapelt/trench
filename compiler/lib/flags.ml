
type flag_record = {
  mutable feature_level : int;
  mutable themes : string list;
}

let compile_flags : flag_record = {
  feature_level = 5;
  themes = [];
} 