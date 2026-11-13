open Helpers
open Absyn
open ProgramRep

type compiler_notes = {
  mutable dir: string;
  mutable stack_size: int;
  mutable size_limit: int;
  mutable team_libraries: string option StringMap.t;
  mutable team_system_libraries: string option StringMap.t;
  mutable shared_library: string option; (* Not the right location... ? *) 
  mutable system_library: string option; (* Not the right location... ? *) 
  mutable event_templates : event_template list;
  mutable compilation_cache : (typ * instruction list) IntMap.t;

  mutable syslib : compile_state * instruction list;
}

let compiler_notes = {
  dir = "";
  size_limit = 0;
  stack_size = 0;
  team_libraries = StringMap.empty;
  team_system_libraries = StringMap.empty;
  shared_library = None;
  system_library = None;
  event_templates = [];
  compilation_cache = IntMap.empty;

  syslib = {scopes = { local = None ; global = [] }; size = 0; labels = StringSet.empty; break = None; continue = None; ret_type = None;} , [];
}