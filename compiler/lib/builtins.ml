open Absyn
open ProgramRep
open Flags
open Exceptions

type builtin_func_compile =
  | FixedFuncComp of (instruction list -> instruction list)
  | FuncComp of instruction


(* Functions *)

type builtin_func_version = {
  args: typ list;
  ret: typ;
  comp: builtin_func_compile;
}

type builtin_func_info = {
  themes: string list;
  features: string list;
  versions: builtin_func_version list;
}

let builtin_func_infos : builtin_func_info StringMap.t = StringMap.of_list [
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
      comp = FixedFuncComp (fun acc -> Instr_Place :: I(4) :: Instr_Trench :: acc);
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
      comp = FixedFuncComp (fun acc -> Instr_Place :: I(4) :: Instr_Fortify :: acc);
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
      args = [T_Dir];
      ret = T_Int;
      comp = FuncComp Instr_Collect;
    };{
      args = [];
      ret = T_Int;
      comp = FixedFuncComp (fun acc -> Instr_Place :: I(4) :: Instr_Collect :: acc);
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
  ("throw_clay", {
    themes = ["pottery"];
    features = [];
    versions = [{
      args = [T_Dir; T_Int];
      ret = T_Int;
      comp = FuncComp Instr_ThrowClay;
    }];
  });
  ("clay_golem", {
    themes = ["pottery"];
    features = [];
    versions = [{
      args = [];
      ret = T_Int;
      comp = FuncComp Instr_ClayGolem;
    }];
  });
  ("count", {
    themes = [];
    features = [];
    versions = [{
      args = [T_Resource];
      ret = T_Int;
      comp = FuncComp Meta_Resource;
    }];
  });
]


(* Variables *)

type builtin_var_compile =
  | VarComp of instruction

type builtin_var_info = {
  themes: string list;
  features: string list;
  typ: typ;
  comp: builtin_var_compile;
}



let builtin_var_infos : builtin_var_info StringMap.t = StringMap.of_list [
  ("id", {
    themes = []; 
    features = [];
    typ = T_Int;
    comp = VarComp Meta_PlayerID
  });
  ("x", {
    themes = []; 
    features = [];
    typ = T_Int;
    comp = VarComp Meta_PlayerX
  });
  ("y", {
    themes = []; 
    features = [];
    typ = T_Int;
    comp = VarComp Meta_PlayerY
  });
  ("board_width", {
    themes = []; 
    features = [];
    typ = T_Int;
    comp = VarComp Meta_BoardX
  });
  ("board_height", {
    themes = []; 
    features = [];
    typ = T_Int;
    comp = VarComp Meta_BoardY
  });
]





let builtin_rename_map = 
  let vars = builtin_var_infos
    |> StringMap.bindings
    |> List.map (fun ii -> (fst ii, fst ii))
  in
  let funcs = builtin_func_infos
    |> StringMap.bindings
    |> List.map (fun ii -> (fst ii, fst ii))
  in
  StringMap.of_list (vars @ funcs)


let themes ts =
  if List.is_empty ts || List.exists (fun t -> StringSet.mem t compile_flags.themes) ts 
  then ()
  else raise_failure ("Attempt to access a feature of an inactive theme: " ^ (String.concat ", " ts))


let features fs = 
  if List.for_all (fun f -> StringSet.mem f compile_flags.features) fs
  then ()
  else raise_failure ("Attempt to access an inactive feature: " ^ (String.concat ", " fs))


let lookup_builtin_func_info name arg_types = 
  match StringMap.find_opt name builtin_func_infos with
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
  
let lookup_builtin_var_info name = 
  match StringMap.find_opt name builtin_var_infos with
  | None -> None
  | Some builtin -> (features builtin.features ; themes builtin.themes ; Some builtin)