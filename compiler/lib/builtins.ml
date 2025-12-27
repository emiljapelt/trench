open Absyn
open ProgramRep
open Flags
open Exceptions

type builtin_var_compile =
  | VarComp of instruction list

type builtin_var_info = {
  themes: string list;
  features: string list;
  typ: typ;
  comp: builtin_var_compile;
}

let builtin_var name themes features typ comp = (name, {
    themes = themes;
    features = features;
    typ = typ;
    comp = VarComp comp
  })


let builtin_var_infos : builtin_var_info StringMap.t = StringMap.of_list [
  builtin_var "id" [] [] T_Int  [Meta_PlayerID];
  builtin_var "x" [] [] T_Int [Meta_PlayerX];
  builtin_var "y" [] [] T_Int [Meta_PlayerY];
  builtin_var "map_width" [] [] T_Int [Meta_BoardX];
  builtin_var "map_height" [] [] T_Int [Meta_BoardY];
  builtin_var "round" [] [] T_Int [Meta_Round];

  builtin_var "wait" [] [] T_Int [Instr_Wait];
  builtin_var "pass" [] [] T_Int [Instr_Pass];
  
  builtin_var "_SUCCESS" [] [] T_Int [Instr_Place; I(1)];
  builtin_var "_ERROR" [] [] T_Int [Instr_Place; I(0)];
  builtin_var "_MISSING_RESOURCE" [] [] T_Int [Instr_Place; I(-1)];
  builtin_var "_OUT_OF_BOUNDS" [] [] T_Int [Instr_Place; I(-2)];
  builtin_var "_INVALID_TARGET" [] [] T_Int [Instr_Place; I(-3)];
  builtin_var "_OUT_OF_RANGE" [] [] T_Int [Instr_Place; I(-4)];
  builtin_var "_OBSTRUCTED" [] [] T_Int [Instr_Place; I(-5)];
  builtin_var "_MISSING_SPACE" [] [] T_Int [Instr_Place; I(-6)];

  builtin_var "move" [] [] (T_Func(T_Int, [T_Dir])) [Instr_Place; I(-5)];
  builtin_var "trench" [] [] (T_Func(T_Int, [T_Dir; T_Int])) [Instr_Place; I(-7)];
  builtin_var "fortify" [] [] (T_Func(T_Int, [T_Dir; T_Int])) [Instr_Place; I(-8)];
  builtin_var "meditate" ["wizardry"] [] (T_Func(T_Int, [])) [Instr_Place; I(-15)];
  builtin_var "projection" ["wizardry"] ["fork"] (T_Func(T_Int, [])) [Instr_Place; I(-12)];
  builtin_var "collect" [] [] (T_Func(T_Int, [T_Dir; T_Int])) [Instr_Place; I(-25)];
  builtin_var "bomb" ["military"] [] (T_Func(T_Int, [T_Dir; T_Int])) [Instr_Place; I(-9)];
  builtin_var "freeze" ["wizardry"] [] (T_Func(T_Int, [T_Dir; T_Int])) [Instr_Place; I(-13)];
  builtin_var "shoot" ["military";"forestry"] [] (T_Func(T_Int, [T_Dir])) [Instr_Place; I(-1)];
  builtin_var "mine" ["military"] [] (T_Func(T_Int, [T_Dir])) [Instr_Place; I(-4)];
  builtin_var "fireball" ["wizardry"] [] (T_Func(T_Int, [T_Dir])) [Instr_Place; I(-14)];
  builtin_var "chop" ["forestry"] [] (T_Func(T_Int, [T_Dir])) [Instr_Place; I(-6)];
  builtin_var "dispel" ["wizardry"] [] (T_Func(T_Int, [T_Dir])) [Instr_Place; I(-16)];
  builtin_var "disarm" ["forestry";"military"] [] (T_Func(T_Int, [T_Dir])) [Instr_Place; I(-17)];
  builtin_var "mana_drain" ["wizardry"] [] (T_Func(T_Int, [T_Dir])) [Instr_Place; I(-18)];
  builtin_var "wall" [] [] (T_Func(T_Int, [T_Dir])) [Instr_Place; I(-22)];
  builtin_var "bridge" [] [] (T_Func(T_Int, [T_Dir])) [Instr_Place; I(-24)];
  builtin_var "plant_tree" ["forestry"] [] (T_Func(T_Int, [T_Dir])) [Instr_Place; I(-23)];
  builtin_var "mount" [] [] (T_Func(T_Int, [T_Dir])) [Instr_Place; I(-27)];
  builtin_var "dismount" [] [] (T_Func(T_Int, [T_Dir])) [Instr_Place; I(-28)];
  builtin_var "boat" [] [] (T_Func(T_Int, [T_Dir])) [Instr_Place; I(-29)];
  builtin_var "bear_trap" ["forestry"] [] (T_Func(T_Int, [T_Dir])) [Instr_Place; I(-30)];
  builtin_var "write" [] ["ipc"] (T_Func(T_Int, [T_Int])) [Instr_Place; I(-10)];
  builtin_var "pager_write" [] ["ipc"] (T_Func(T_Int, [T_Int])) [Instr_Place; I(-21)];
  builtin_var "pager_set" [] ["ipc"] (T_Func(T_Int, [T_Int])) [Instr_Place; I(-19)];
  builtin_var "say" [] [] (T_Func(T_Int, [T_Int])) [Instr_Place; I(-26)];
  builtin_var "pager_read" [] ["ipc"] (T_Func(T_Int, [])) [Instr_Place; I(-20)];
  builtin_var "read" [] ["ipc"] (T_Func(T_Int, [])) [Instr_Place; I(-11)];
  builtin_var "scan" [] [] (T_Func(T_Field, [T_Dir;T_Int])) [Instr_Place; I(-3)];
  builtin_var "look" [] [] (T_Func(T_Int, [T_Dir;T_Prop])) [Instr_Place; I(-2)];
  builtin_var "throw_clay" ["pottery"] [] (T_Func(T_Int, [T_Dir;T_Int])) [Instr_Place; I(-31)];
  builtin_var "clay_golem" ["pottery"] ["fork"] (T_Func(T_Int, [])) [Instr_Place; I(-32)];
  builtin_var "count" [] [] (T_Func(T_Int, [T_Resource])) [Instr_Place; I(-37)];
  builtin_var "drop" [] [] (T_Func(T_Int, [T_Int;T_Resource])) [Instr_Place; I(-33)];
  builtin_var "take" [] [] (T_Func(T_Int, [T_Int;T_Resource])) [Instr_Place; I(-34)];
  builtin_var "mine_shaft" [] [] (T_Func(T_Int, [T_Dir])) [Instr_Place; I(-35)];
  builtin_var "craft" [] ["craft"] (T_Func(T_Int, [T_Resource])) [Instr_Place; I(-36)];
]



let builtin_rename_map = 
  let vars = builtin_var_infos
    |> StringMap.bindings
    |> List.map (fun ii -> (fst ii, fst ii))
  in
  StringMap.of_list vars


let themes ts =
  if List.is_empty ts || List.exists (fun t -> StringSet.mem t compile_flags.themes) ts 
  then ()
  else raise_failure ("Attempt to access a feature of an inactive theme: " ^ (String.concat ", " ts))


let features fs = 
  if List.for_all (fun f -> StringSet.mem f compile_flags.features) fs
  then ()
  else raise_failure ("Attempt to access an inactive feature: " ^ (String.concat ", " fs))


let lookup_builtin_var_info name = 
  match StringMap.find_opt name builtin_var_infos with
  | None -> None
  | Some builtin -> (features builtin.features ; themes builtin.themes ; Some builtin)