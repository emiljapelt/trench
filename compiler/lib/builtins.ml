open Absyn
open ProgramRep
open Flags
open Exceptions

type builtin_info = 
  | BuiltinVar of { 
    typ: typ;
    comp: instruction list;
  }
  | BuiltinFunc of {
    ret: typ;
    canonical: typ list;
    addr: int;
    short_hands: (typ list * (instruction list -> instruction list)) list;
  }

type builtin = {
  themes: string list;
  features: string list;
  info: builtin_info;
}

(*type builtin_var_compile =
  | VarComp of instruction list

type builtin_var_info = {
  themes: string list;
  features: string list;
  typ: typ;
  comp: builtin_var_compile;
}*)

let builtin_var name themes features typ comp = (name, {
    themes = themes;
    features = features;
    info = BuiltinVar {
      typ = typ;
      comp = comp;
    };
  })

let builtin_func name themes features addr ret canon short_hands = (name, {
    themes = themes;
    features = features;
    info = BuiltinFunc {
      addr = addr;
      ret = ret;
      canonical = canon;
      short_hands = short_hands;
    };
  })

let builtin_canonical_type builtin = match builtin.info with
  | BuiltinFunc f -> T_Func(f.ret, f.canonical)
  | BuiltinVar v -> v.typ


let builtin_infos : builtin StringMap.t = StringMap.of_list [
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
  
  (* Test shorthands *)
  builtin_func "shoot" ["military";"forestry"] [] (-1) T_Int[T_Dir] [];
  builtin_func "look" [] [] (-2) T_Int[T_Dir;T_Prop] [];
  builtin_func "scan" [] [] (-3) T_Field[T_Dir;T_Int] [];
  builtin_func "mine" ["military"] [] (-4) T_Int[T_Dir] [];
  builtin_func "move" [] [] (-5) T_Int[T_Dir] [];
  builtin_func "chop" ["forestry"] [] (-6) T_Int[T_Dir] [];
  builtin_func "trench" [] [] (-7) T_Int[T_Dir; T_Int] [
    ([], fun _ -> [Instr_Place; I(0); Instr_Place; I(0); Instr_Place; I(-7); Instr_Place; I(2); Instr_Call]);
    ([T_Dir], fun arg -> arg @ [Instr_Place; I(1); Instr_Place; I(-7); Instr_Place; I(2); Instr_Call]);
  ];
  builtin_func "fortify" [] [] (-8) T_Int[T_Dir; T_Int] [
    ([], fun _ -> [Instr_Place; I(0); Instr_Place; I(0); Instr_Place; I(-8); Instr_Place; I(2); Instr_Call]);
    ([T_Dir], fun arg -> arg @ [Instr_Place; I(1); Instr_Place; I(-8); Instr_Place; I(2); Instr_Call]);
  ];
  builtin_func "bomb" ["military"] [] (-9) T_Int[T_Dir; T_Int] [];
  builtin_func "write" [] ["ipc"] (-10) T_Int[T_Int] [];
  builtin_func "read" [] ["ipc"] (-11) T_Int[] [];
  builtin_func "projection" ["wizardry"] ["fork"] (-12) T_Int[] [];
  builtin_func "freeze" ["wizardry"] [] (-13) T_Int[T_Dir; T_Int] [];
  builtin_func "fireball" ["wizardry"] [] (-14) T_Int[T_Dir] [];
  builtin_func "meditate" ["wizardry"] [] (-15) T_Int[] [];
  builtin_func "dispel" ["wizardry"] [] (-16) T_Int[T_Dir] [];
  builtin_func "disarm" ["forestry";"military"] [] (-17) T_Int[T_Dir] [];
  builtin_func "mana_drain" ["wizardry"] [] (-18) T_Int[T_Dir] [];
  builtin_func "pager_set" [] ["ipc"] (-19) T_Int[T_Int] [];
  builtin_func "pager_read" [] ["ipc"] (-20) T_Int[] [];
  builtin_func "pager_write" [] ["ipc"] (-21) T_Int[T_Int] [];
  builtin_func "wall" [] [] (-22) T_Int[T_Dir] [];
  builtin_func "plant_tree" ["forestry"] [] (-23) T_Int[T_Dir] [];
  builtin_func "bridge" [] [] (-24) T_Int[T_Dir] [];
  builtin_func "collect" [] [] (-25) T_Int[T_Dir; T_Int] [
    ([], fun _ -> [Instr_Place; I(0); Instr_Place; I(0); Instr_Place; I(-25); Instr_Place; I(2); Instr_Call]);
    ([T_Dir], fun arg -> arg @ [Instr_Place; I(1); Instr_Place; I(-25); Instr_Place; I(2); Instr_Call]);
  ];
  builtin_func "say" [] [] (-26) T_Int[T_Int] [];
  builtin_func "mount" [] [] (-27) T_Int[T_Dir] [];
  builtin_func "dismount" [] [] (-28) T_Int[T_Dir] [];
  builtin_func "boat" [] [] (-29) T_Int[T_Dir] [];
  builtin_func "bear_trap" ["forestry"] [] (-30) T_Int[T_Dir] [];
  builtin_func "throw_clay" ["pottery"] [] (-31) T_Int[T_Dir;T_Int] [];
  builtin_func "clay_golem" ["pottery"] ["fork"] (-32) T_Int[] [];
  builtin_func "drop" [] [] (-33) T_Int[T_Int;T_Resource] [];
  builtin_func "take" [] [] (-34) T_Int[T_Int;T_Resource] [];
  builtin_func "mine_shaft" [] [] (-35) T_Int[T_Dir] [];
  builtin_func "craft" [] ["craft"] (-36) T_Int[T_Resource] [];
  builtin_func "count" [] [] (-37) T_Int[T_Resource] [];
]



let builtin_rename_map = 
  let vars = builtin_infos
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

let lookup_builtin_info name = 
  match StringMap.find_opt name builtin_infos with
  | None -> None
  | Some builtin -> (features builtin.features ; themes builtin.themes ; Some builtin)

let type_list_eq tl1 tl2 = 
  List.length tl1 == List.length tl2 && List.for_all (fun (a,b) -> type_eq a b) (List.combine tl1 tl2)

(*
let lookup_builtin_func_info name arg_types = 
  match StringMap.find_opt name builtin_infos with
  | None -> None
  | Some builtin -> 
    match builtin.info with
    | BuiltinVar _ -> None
    | BuiltinFunc bf -> 
    match
      builtin.versions |>
      List.find_opt (fun builtin -> 
        List.length arg_types = List.length builtin.args
        && List.for_all (fun (a,b) -> type_eq a b) (List.combine arg_types builtin.args))
    with 
    | None -> None
    | Some version -> (features builtin.features ; themes builtin.themes ; Some version)
  *)