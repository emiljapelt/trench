
open Helpers

type flag_record = {
  mutable features : StringSet.t;
  mutable themes : StringSet.t;
  mutable auto_resize : bool;
}

let compile_flags : flag_record = {
  features = StringSet.empty;
  themes = StringSet.empty;
  auto_resize = true;
} 