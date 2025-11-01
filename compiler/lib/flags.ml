
module StringSet = Set.Make(String)

type flag_record = {
  mutable features : StringSet.t;
  mutable themes : StringSet.t;
}

let compile_flags : flag_record = {
  features = StringSet.empty;
  themes = StringSet.empty;
} 