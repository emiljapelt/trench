open Absyn
open ProgramRep
open Flags
open Exceptions

type builtin_compile =
  | FixedComp of (instruction list -> instruction list)
  | FuncComp of instruction

type builtin_version = {
  args: typ list;
  ret: typ;
  comp: builtin_compile;
}

type builtin_info = {
  themes: string list;
  features: string list;
  versions: builtin_version list;
}

let instruction_infos : builtin_info StringMap.t = StringMap.of_list [
  ("move", {
    themes = []; 
    features = [];
    versions = [{
      args = [T_Dir];
      ret = T_Int;
      comp = FuncComp Instr_Move;
    }];
  });
  ("trench", {
    themes = []; 
    features = [];
    versions = [{
      args = [T_Dir];
      ret = T_Int;
      comp = FuncComp Instr_Trench;
    };{
      args = [];
      ret = T_Int;
      comp = FixedComp (fun acc -> Instr_Place :: I(4) :: Instr_Trench :: acc);
    }]
  });
  ("fortify", {
    themes = []; 
    features = [];
    versions = [{
      args = [T_Dir];
      ret = T_Int;
      comp = FuncComp Instr_Fortify;
    };{
      args = [];
      ret = T_Int;
      comp = FixedComp (fun acc -> Instr_Place :: I(4) :: Instr_Fortify :: acc);
    };]
  });
  ("wait", {
    themes = []; 
    features = [];
    versions = [{
      args = [];
      ret = T_Int;
      comp = FuncComp Instr_Wait;
    }];
  });
  ("pass", {
    themes = []; 
    features = [];
    versions = [{
      args = [];
      ret = T_Int;
      comp = FuncComp Instr_Pass;
    }];
  });
  ("meditate", {
    themes = ["wizardry"];
    features = [];
    versions = [{
      args = [];
      ret = T_Int;
      comp = FuncComp Instr_Meditate;
    }];
  });
  ("projection", {
    themes = ["wizardry"];
    features = [];
    versions = [{
      args = [];
      ret = T_Int;
      comp = FuncComp Instr_Projection;
    }];
  });
  ("collect", {
    themes = []; 
    features = [];
    versions = [{
      args = [];
      ret = T_Int;
      comp = FuncComp Instr_Collect;
    };{
      args = [];
      ret = T_Int;
      comp = FixedComp (fun acc -> Instr_Place :: I(4) :: Instr_Collect :: acc);
    }];
  });
  ("bomb", {
    themes = ["military"];
    features = [];
    versions = [{
      args = [T_Dir; T_Int];
      ret = T_Int;
      comp = FuncComp Instr_Bomb;
    }];
  });
  ("freeze", {
    themes = ["wizardry"];
    features = [];
    versions = [{
      args = [T_Dir; T_Int];
      ret = T_Int;
      comp = FuncComp Instr_Freeze;
    }];
  });
  ("shoot", {
    themes = ["military"; "forestry"];
    features = [];
    versions = [{
      args = [T_Dir];
      ret = T_Int;
      comp = FuncComp Instr_Shoot;
    }];
  });
  ("mine", {
    themes = ["military"];
    features = [];
    versions = [{
      args = [T_Dir];
      ret = T_Int;
      comp = FuncComp Instr_Mine;
    }];
  });
  ("fireball", {
    themes = ["wizardry"];
    features = [];
    versions = [{
      args = [T_Dir];
      ret = T_Int;
      comp = FuncComp Instr_Fireball;
    }];
  });
  ("chop", {
    themes = ["forestry"];
    features = [];
    versions = [{
      args = [T_Dir];
      ret = T_Int;
      comp = FuncComp Instr_Chop;
    }];
  });
  ("dispel", {
    themes = ["wizardry"];
    features = [];
    versions = [{
      args = [T_Dir];
      ret = T_Int;
      comp = FuncComp Instr_Dispel;
    }];
  });
  ("disarm", {
    themes = ["forestry"; "military"];
    features = [];
    versions = [{
      args = [T_Dir];
      ret = T_Int;
      comp = FuncComp Instr_Disarm;
    }];
  });
  ("mana_drain", {
    themes = ["wizardry"];
    features = [];
    versions = [{
      args = [T_Dir];
      ret = T_Int;
      comp = FuncComp Instr_ManaDrain;
    }];
  });
  ("wall", {
    themes = []; 
    features = [];
    versions = [{
      args = [T_Dir];
      ret = T_Int;
      comp = FuncComp Instr_Wall;
    }];
  });
  ("bridge", {
    themes = []; 
    features = [];
    versions = [{
      args = [T_Dir];
      ret = T_Int;
      comp = FuncComp Instr_Bridge;
    }];
  });
  ("plant_tree", {
    themes = ["forestry"];
    features = [];
    versions = [{
      args = [T_Dir];
      ret = T_Int;
      comp = FuncComp Instr_PlantTree;
    }];
  });
  ("mount", {
    themes = []; 
    features = [];
    versions = [{
      args = [T_Dir];
      ret = T_Int;
      comp = FuncComp Instr_Mount;
    }];
  });
  ("dismount", {
    themes = [];
    features = [];
    versions = [{
      args = [T_Dir];
      ret = T_Int;
      comp = FuncComp Instr_Dismount;
    }];
  });
  ("boat", {
    themes = [];
    features = [];
    versions = [{
      args = [T_Dir];
      ret = T_Int;
      comp = FuncComp Instr_Boat;
    }];
  });
  ("bear_trap", {
    themes = ["forestry"];
    features = [];
    versions = [{
      args = [T_Dir];
      ret = T_Int;
      comp = FuncComp Instr_BearTrap;
    }];
  });
  ("write", {
    themes = [];
    features = ["ipc"];
    versions = [{
      args = [T_Int];
      ret = T_Int;
      comp = FuncComp Instr_Write;
    }];
  });
  ("pager_write", {
    themes = [];
    features = ["ipc"];
    versions = [{
      args = [T_Int];
      ret = T_Int;
      comp = FuncComp Instr_PagerWrite;
    }];
  });
  ("pager_set", {
    themes = [];
    features = ["ipc"];
    versions = [{
      args = [T_Int];
      ret = T_Int;
      comp = FuncComp Instr_PagerSet;
    }];
  });
  ("say", {
    themes = [];
    features = [];
    versions = [{
      args = [T_Int];
      ret = T_Int;
      comp = FuncComp Instr_Say;
    }];
  });
  ("pager_read", {
    themes = [];
    features = [];
    versions = [{
      args = [];
      ret = T_Int;
      comp = FuncComp Instr_PagerRead;
    }];
  });
  ("read", {
    themes = [];
    features = ["ipc"];
    versions = [{
      args = [];
      ret = T_Int;
      comp = FuncComp Instr_Read;
    }];
  });
  ("scan", {
    themes = [];
    features = [];
    versions = [{
      args = [T_Dir; T_Int];
      ret = T_Field;
      comp = FuncComp Instr_Scan;
    }];
  });
  ("look", {
    themes = [];
    features = [];
    versions = [{
      args = [T_Dir; T_Prop];
      ret = T_Int;
      comp = FuncComp Instr_Look;
    }];
  });
]

let instruction_rename_map = 
  instruction_infos
  |> StringMap.bindings
  |> List.map (fun ii -> (fst ii, fst ii))
  |> StringMap.of_list


let themes ts =
  if List.is_empty ts || List.exists (fun t -> StringSet.mem t compile_flags.themes) ts 
  then ()
  else raise_failure ("Attempt to access a feature of an inactive theme: " ^ (String.concat ", " ts))


let features fs = 
  if List.for_all (fun f -> StringSet.mem f compile_flags.features) fs
  then ()
  else raise_failure ("Attempt to access an inactive feature: " ^ (String.concat ", " fs))


let lookup_builtin_info name arg_types = 
  match StringMap.find_opt name instruction_infos with
  | None -> None
  | Some builtin -> 
    match
      builtin.versions |>
      List.find_opt (fun builtin -> 
        List.length arg_types = List.length builtin.args
        && List.for_all (fun (a,b) -> type_eq a b) (List.combine arg_types builtin.args))
    with 
    | None -> None
    | Some version -> (features builtin.features ; themes builtin.themes ; Some version)
  