open Absyn
open ProgramRep
open Flags
open Exceptions
open Helpers

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


(* Preparing to remove everything 'builtin' related *)
let builtin_infos : builtin StringMap.t = StringMap.of_list [
  (*builtin_var "x" [] [] T_Int [Instr_Meta; I(0)];
  builtin_var "y" [] [] T_Int [Instr_Meta; I(1)];
  builtin_var "id" [] [] T_Int  [Instr_Meta; I(2)];
  builtin_var "map_width" [] [] T_Int [Instr_Meta; I(3)];
  builtin_var "map_height" [] [] T_Int [Instr_Meta; I(4)];
  builtin_var "round" [] [] T_Int [Instr_Meta; I(5)];
  builtin_var "actions" [] [] T_Int [Instr_Meta; I(6)];

  builtin_var "true" [] [] T_Int [Instr_Place; I(1)];
  builtin_var "false" [] [] T_Int [Instr_Place; I(0)];

  builtin_var "_SUCCESS" [] [] T_Int [Instr_Place; I(1)];
  builtin_var "_ERROR" [] [] T_Int [Instr_Place; I(0)];
  builtin_var "_MISSING_RESOURCE" [] [] T_Int [Instr_Place; I(-1)];
  builtin_var "_OUT_OF_BOUNDS" [] [] T_Int [Instr_Place; I(-2)];
  builtin_var "_INVALID_TARGET" [] [] T_Int [Instr_Place; I(-3)];
  builtin_var "_OUT_OF_RANGE" [] [] T_Int [Instr_Place; I(-4)];
  builtin_var "_OBSTRUCTED" [] [] T_Int [Instr_Place; I(-5)];
  builtin_var "_MISSING_SPACE" [] [] T_Int [Instr_Place; I(-6)];
  
  
  builtin_func "shoot" ["military";"forestry"] [] (-1) T_Int[T_Dir] [];
  builtin_func "look" [] [] (-2) T_Int[T_Dir;T_Field] [];
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
  builtin_func "say" [] ["debug"] (-26) T_Int[T_Int] [];
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

  builtin_func "pass" [] [] (-38) T_Int[] [];
  builtin_func "wait" [] [] (-39) T_Int[] [];

  builtin_func "obliviate" ["wizardry"] [] (-40) T_Int[T_Dir] [];
  builtin_func "blink" ["wizardry"] [] (-41) T_Int[] [];

  builtin_func "search" [] [] (-42) T_Int[T_Resource] [];*)
]

let _themes ts =
  List.is_empty ts || List.exists (fun t -> StringSet.mem t compile_flags.themes) ts

let _features fs = 
  List.for_all (fun f -> StringSet.mem f compile_flags.features) fs


let expr_const name typ expr themes features =
  if _features features && _themes themes
  then Some(Const(typ, name, Expr(expr,0)))
  else None

let asm_const name typ asm themes features =
  expr_const name typ (ASM(typ, asm)) themes features

let func_const name ret args addr themes features =
  let typ = (T_Func(ret, args)) in
  expr_const name typ (ASM(typ, [Instr_Place; I addr])) themes features




let generate_initial_scope () : identifier list =
  let entries = [
    asm_const "x" T_Int [Instr_Meta; I(0)] [] [];
    asm_const "y" T_Int [Instr_Meta; I(1)] [] [];
    asm_const "id" T_Int [Instr_Meta; I(2)] [] [];
    asm_const "map_width" T_Int [Instr_Meta; I(3)] [] [];
    asm_const "map_height" T_Int [Instr_Meta; I(4)] [] [];
    asm_const "round" T_Int [Instr_Meta; I(5)] [] [];
    asm_const "actions" T_Int [Instr_Meta; I(6)] [] [];

    expr_const "true" T_Int (Int 1) [] [];
    expr_const "false" T_Int (Int 0) [] [];

    expr_const "_SUCCESS" T_Int (Int 1) [] [];
    expr_const "_ERROR" T_Int (Int 0) [] [];
    expr_const "_MISSING_RESOURCE" T_Int (Int (-1)) [] [];
    expr_const "_OUT_OF_BOUNDS" T_Int (Int (-2)) [] [];
    expr_const "_INVALID_TARGET" T_Int (Int (-3)) [] [];
    expr_const "_OUT_OF_RANGE" T_Int (Int (-4)) [] [];
    expr_const "_OBSTRUCTED" T_Int (Int (-5)) [] [];
    expr_const "_MISSING_SPACE" T_Int (Int (-6)) [] [];

    func_const "shoot" T_Int [T_Dir] (-1) ["military";"forestry"] [];
    func_const "look" T_Int [T_Dir;T_Field] (-2) [] [];
    func_const "scan" T_Field [T_Dir;T_Int] (-3) [] [];
    func_const "mine" T_Int [T_Dir] (-4) ["military"] [];
    func_const "move" T_Int [T_Dir] (-5) [] [];
    func_const "chop" T_Int[T_Dir] (-6) ["forestry"] [];
    (* trench (-7) *)
    (* fortify (-8) *)
    func_const "bomb" T_Int [T_Dir;T_Int] (-9) ["military"] [];
    func_const "write" T_Int [T_Int] (-10) [] [];
    func_const "read" T_Int [] (-11) [] [];
    func_const "projection" T_Int[] (-12) ["wizardry"] ["form"];
    func_const "freeze" T_Int[T_Dir;T_Int] (-13) ["wizardry"] [];
    func_const "fireball" T_Int[T_Dir] (-14) ["wizardry"] [];
    func_const "meditate" T_Int[] (-15) ["wizardry"] [];
    func_const "dispel" T_Int[T_Dir] (-16) ["wizardry"] [];
    func_const "disarm" T_Int[T_Dir] (-17) ["military";"forestry"] [];
    func_const "mana_drain" T_Int[T_Dir] (-18) ["wizardry"] [];
    func_const "pager_set" T_Int[T_Int] (-19) [] ["ipc"];
    func_const "pager_read" T_Int[] (-20) ["ipc"] [];
    func_const "pager_write" T_Int[T_Int] (-21) ["ipc"] [];
    func_const "wall" T_Int[T_Dir] (-22) [] [];
    func_const "plant_tree" T_Int[T_Dir] (-23) ["forestry"] [];
    func_const "bridge" T_Int[T_Dir] (-24) [] [];
    (*func_const "collect" [] (-25) [] [];*)
    func_const "say" T_Int[T_Int] (-26) [] ["debug"];
    func_const "mount" T_Int[T_Dir] (-27) [] []; (*No dir shorthand?*)
    func_const "dismount" T_Int[T_Dir] (-28) [] []; (*No dir shorthand?*)
    func_const "boat" T_Int[T_Dir] (-29) [] [];
    func_const "bear_trap" T_Int[T_Dir] (-30) ["forestry"] [];
    func_const "throw_clay" T_Int[T_Dir;T_Int] (-31) ["pottery"] [];
    func_const "clay_golem" T_Int[] (-32) ["pottery"] ["fork"];
    func_const "drop" T_Int[T_Int;T_Resource] (-33) [] [];
    func_const "take" T_Int[T_Int;T_Resource] (-34) [] [];
    func_const "mine_shaft" T_Int[T_Dir] (-35) [] [];
    func_const "craft" T_Int[T_Resource] (-36) [] [];
    func_const "count" T_Int[T_Resource] (-37) [] [];
    func_const "pass" T_Int[] (-38) [] [];
    func_const "wait" T_Int[] (-39) [] [];
  ]
  in
  entries 
  |> List.filter Option.is_some
  |> List.map Option.get

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