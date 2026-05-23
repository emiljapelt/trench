
open Helpers

type flag_record = {
  mutable features : StringSet.t;
  mutable themes : StringSet.t;
  mutable auto_resize_array : bool;
}

let compile_flags : flag_record = {
  features = StringSet.empty;
  themes = StringSet.empty;
  auto_resize_array = true;
} 