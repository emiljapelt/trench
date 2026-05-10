open Absyn
open ProgramRep
open Flags
open Helpers

let themes ts =
  List.is_empty ts || List.exists (fun t -> StringSet.mem t compile_flags.themes) ts

let features fs = 
  List.for_all (fun f -> StringSet.mem f compile_flags.features) fs

let expr_const name typ expr ts fs =
  if features fs && themes ts
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
    func_const "trench" T_Int[T_Dir;T_Int] (-7) [] []; (*shorthand?*)
    func_const "fortify" T_Int[T_Dir;T_Int] (-8) [] []; (*shorthand?*)
    func_const "bomb" T_Int [T_Dir;T_Int] (-9) ["military"] [];
    func_const "write" T_Int [T_Int] (-10) [] [];
    func_const "read" T_Int [] (-11) [] [];
    func_const "projection" T_Int[] (-12) ["wizardry"] ["fork"];
    func_const "freeze" T_Int[T_Dir;T_Int] (-13) ["wizardry"] [];
    func_const "fireball" T_Int[T_Dir] (-14) ["wizardry"] [];
    func_const "meditate" T_Int[] (-15) ["wizardry"] [];
    func_const "dispel" T_Int[T_Dir] (-16) ["wizardry"] [];
    func_const "disarm" T_Int[T_Dir] (-17) ["military";"forestry"] [];
    func_const "mana_drain" T_Int[T_Dir] (-18) ["wizardry"] [];
    func_const "pager_set" T_Int[T_Int] (-19) [] ["ipc"];
    func_const "pager_read" T_Int[] (-20) [] ["ipc"];
    func_const "pager_write" T_Int[T_Int] (-21) [] ["ipc"];
    func_const "wall" T_Int[T_Dir] (-22) [] [];
    func_const "plant_tree" T_Int[T_Dir] (-23) ["forestry"] [];
    func_const "bridge" T_Int[T_Dir] (-24) [] [];
    func_const "collect" T_Int[T_Dir;T_Int] (-25) [] []; (*shorthand?*)
    func_const "say" T_Int[T_Int] (-26) [] ["debug"];
    func_const "mount" T_Int[T_Dir] (-27) [] []; (*shorthand?*)
    func_const "dismount" T_Int[T_Dir] (-28) [] []; (*shorthand?*)
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
    func_const "obliviate" T_Int[T_Dir] (-40) ["wizardry"] [];
    func_const "blink" T_Int[] (-41) ["wizardry"] [];
    func_const "search" T_Int[T_Resource] (-42) [] [];
  ]
  in
  entries 
  |> List.filter Option.is_some
  |> List.map Option.get
