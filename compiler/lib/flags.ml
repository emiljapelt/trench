
open Helpers

type flag_record = {
  mutable features : StringSet.t;
  mutable themes : StringSet.t;
  mutable auto_resize : bool;
  mutable map_width : int;
  mutable map_height : int;
}

let compile_flags : flag_record = {
  features = StringSet.empty;
  themes = StringSet.empty;
  auto_resize = true;
  map_width = -1;
  map_height = -1;
} 

let set_themes ts = compile_flags.themes <- ts ; ()

let set_features fs = compile_flags.features <- fs ; ()

let set_auto_resize v = compile_flags.auto_resize <- v ; ()

let set_map_size (w, h) = 
  compile_flags.map_width <- w ;
  compile_flags.map_height <- h ;
  ()
