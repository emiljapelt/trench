open Absyn
open ProgramRep
open Flags
open Exceptions

type builtin_compile =
  | FixedComp of (instruction list -> instruction list)
  | FuncComp of instruction

type builtin_info = {
  name: string;
  args: typ list;
  ret: typ;
  themes: string list;
  features: string list;
  comp: builtin_compile;
}

let instruction_infos : builtin_info list = [
  {
    name = "move";
    args = [T_Dir];
    ret = T_Int;
    themes = []; 
    features = [];
    comp = FuncComp Instr_Move;
  };
  {
    name = "trench";
    args = [T_Dir];
    ret = T_Int;
    themes = []; 
    features = [];
    comp = FuncComp Instr_Trench;
  };
  {
    name = "trench";
    args = [];
    ret = T_Int;
    themes = []; 
    features = [];
    comp = FixedComp (fun acc -> Instr_Place :: I(4) :: Instr_Trench :: acc);
  };
  {
    name = "fortify";
    args = [T_Dir];
    ret = T_Int;
    themes = []; 
    features = [];
    comp = FuncComp Instr_Fortify;
  };
  {
    name = "fortify";
    args = [];
    ret = T_Int;
    themes = []; 
    features = [];
    comp = FixedComp (fun acc -> Instr_Place :: I(4) :: Instr_Fortify :: acc);
  };
  {
    name = "wait";
    args = [];
    ret = T_Int;
    themes = []; 
    features = [];
    comp = FuncComp Instr_Wait;
  };
  {
    name = "pass";
    args = [];
    ret = T_Int;
    themes = []; 
    features = [];
    comp = FuncComp Instr_Pass;
  };
  {
    name = "meditate";
    args = [];
    ret = T_Int;
    themes = ["wizardry"];
    features = [];
    comp = FuncComp Instr_Meditate;
  };
  {
    name = "projection";
    args = [];
    ret = T_Int;
    themes = ["wizardry"];
    features = [];
    comp = FuncComp Instr_Projection;
  };
  {
    name = "collect";
    args = [];
    ret = T_Int;
    themes = []; 
    features = [];
    comp = FuncComp Instr_Collect;
  };
  {
    name = "collect";
    args = [];
    ret = T_Int;
    themes = []; 
    features = [];
    comp = FixedComp (fun acc -> Instr_Place :: I(4) :: Instr_Collect :: acc);
  };
  {
    name = "bomb";
    args = [T_Dir; T_Int];
    ret = T_Int;
    themes = ["military"];
    features = [];
    comp = FuncComp Instr_Bomb;
  };
  {
    name = "freeze";
    args = [T_Dir; T_Int];
    ret = T_Int;
    themes = ["wizardry"];
    features = [];
    comp = FuncComp Instr_Freeze;
  };
  {
    name = "shoot";
    args = [T_Dir];
    ret = T_Int;
    themes = ["military"; "forestry"];
    features = [];
    comp = FuncComp Instr_Shoot;
  };
  {
    name = "mine";
    args = [T_Dir];
    ret = T_Int;
    themes = ["military"];
    features = [];
    comp = FuncComp Instr_Mine;
  };
  {
    name = "fireball";
    args = [T_Dir];
    ret = T_Int;
    themes = ["wizardry"];
    features = [];
    comp = FuncComp Instr_Fireball;
  };
  {
    name = "chop";
    args = [T_Dir];
    ret = T_Int;
    themes = ["forestry"];
    features = [];
    comp = FuncComp Instr_Chop;
  };
  {
    name = "dispel";
    args = [T_Dir];
    ret = T_Int;
    themes = ["wizardry"];
    features = [];
    comp = FuncComp Instr_Dispel;
  };
  {
    name = "disarm";
    args = [T_Dir];
    ret = T_Int;
    themes = ["forestry"; "military"];
    features = [];
    comp = FuncComp Instr_Disarm;
  };
  {
    name = "mana_drain";
    args = [T_Dir];
    ret = T_Int;
    themes = ["wizardry"];
    features = [];
    comp = FuncComp Instr_ManaDrain;
  };
  {
    name = "wall";
    args = [T_Dir];
    ret = T_Int;
    themes = []; 
    features = [];
    comp = FuncComp Instr_Wall;
  };
  {
    name = "bridge";
    args = [T_Dir];
    ret = T_Int;
    themes = []; 
    features = [];
    comp = FuncComp Instr_Bridge;
  };
  {
    name = "plant_tree";
    args = [T_Dir];
    ret = T_Int;
    themes = ["forestry"];
    features = [];
    comp = FuncComp Instr_PlantTree;
  };
  {
    name = "mount";
    args = [T_Dir];
    ret = T_Int;
    themes = []; 
    features = [];
    comp = FuncComp Instr_Mount;
  };
  {
    name = "dismount";
    args = [T_Dir];
    ret = T_Int;
    themes = [];
    features = [];
    comp = FuncComp Instr_Dismount;
  };
  {
    name = "boat";
    args = [T_Dir];
    ret = T_Int;
    themes = [];
    features = [];
    comp = FuncComp Instr_Boat;
  };
  {
    name = "bear_trap";
    args = [T_Dir];
    ret = T_Int;
    themes = ["forestry"];
    features = [];
    comp = FuncComp Instr_BearTrap;
  };
  {
    name = "write";
    args = [T_Int];
    ret = T_Int;
    themes = [];
    features = ["ipc"];
    comp = FuncComp Instr_Write;
  };
  {
    name = "pager_write";
    args = [T_Int];
    ret = T_Int;
    themes = [];
    features = ["ipc"];
    comp = FuncComp Instr_PagerWrite;
  };
  {
    name = "pager_set";
    args = [T_Int];
    ret = T_Int;
    themes = [];
    features = ["ipc"];
    comp = FuncComp Instr_PagerSet;
  };
  {
    name = "say";
    args = [T_Int];
    ret = T_Int;
    themes = [];
    features = [];
    comp = FuncComp Instr_Say;
  };
  {
    name = "pager_read";
    args = [];
    ret = T_Int;
    themes = [];
    features = [];
    comp = FuncComp Instr_PagerRead;
  };
  {
    name = "read";
    args = [];
    ret = T_Int;
    themes = [];
    features = ["ipc"];
    comp = FuncComp Instr_Read;
  };
  {
    name = "scan";
    args = [T_Dir; T_Int];
    ret = T_Field;
    themes = [];
    features = [];
    comp = FuncComp Instr_Scan;
  };
  {
    name = "look";
    args = [T_Dir; T_Prop];
    ret = T_Int;
    themes = [];
    features = [];
    comp = FuncComp Instr_Look;
  };
]

let instruction_rename_map = 
  instruction_infos
  |> List.map (fun ii -> (ii.name, ii.name))
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
  let builtin_opt = 
    instruction_infos
    |> List.find_opt (fun ii -> 
      ii.name = name
      && List.length arg_types = List.length ii.args
      && List.for_all (fun (a,b) -> type_eq a b) (List.combine arg_types ii.args)
    )
  in match builtin_opt with
  | Some builtin -> (features builtin.features ; themes builtin.themes ; Some builtin)
  | None -> None