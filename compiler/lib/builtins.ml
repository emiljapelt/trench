open Absyn
open ProgramRep
open Flags
open Helpers

let themes ts =
  List.is_empty ts || List.exists (fun t -> StringSet.mem t compile_flags.themes) ts

let features fs = 
  List.for_all (fun f -> StringSet.mem f compile_flags.features) fs

let builtin_func ret args addr =
  ASM(T_Func(ret,args), [Instr_Place; I(addr)])

type builtin = {
  name: string;
  expr: expr;
  themes: string list;
  features: string list;
  meta: (string * expr) list;
}

let builtins () : builtin list = [
  {
    name = "x";
    expr = ASM(T_Int, [Instr_Meta; I(0)]);
    themes = []; features = ["meta"];
    meta = [];
  };{
    name = "y";
    expr = ASM(T_Int, [Instr_Meta; I(1)]);
    themes = []; features = ["meta"];
    meta = [];
  };
  {
    name = "id";
    expr = ASM(T_Int, [Instr_Meta; I(2)]);
    themes = []; features = ["meta"];
    meta = [];
  };{
    name = "map_width";
    expr = Int Flags.compile_flags.map_width;
    themes = []; features = ["meta"];
    meta = [];
  };
  {
    name = "map_height";
    expr = Int Flags.compile_flags.map_height;
    themes = []; features = ["meta"];
    meta = [];
  };
  {
    name = "round";
    expr = ASM(T_Int, [Instr_Meta; I(5)]);
    themes = []; features = ["meta"];
    meta = [];
  };{
    name = "actions";
    expr = ASM(T_Int, [Instr_Meta; I(6)]);
    themes = []; features = ["meta"];
    meta = []
  };{
    name = "map";
    expr = StructureLiteral[
      StructureElement(Some "width", Expr(Int Flags.compile_flags.map_width, 0));
      StructureElement(Some "height", Expr(Int Flags.compile_flags.map_height, 0));
    ];
    themes = []; features = ["meta"];
    meta = []
  };{
    name = "player";
    expr = StructureLiteral[
      StructureElement(Some "x", Expr(ASM(T_Int,[Instr_Meta; I(0)]), 0));
      StructureElement(Some "y", Expr(ASM(T_Int,[Instr_Meta; I(1)]), 0));
      StructureElement(Some "id", Expr(ASM(T_Int,[Instr_Meta; I(2)]), 0));
      StructureElement(Some "actions", Expr(ASM(T_Int,[Instr_Meta; I(6)]), 0));
    ];
    themes = []; features = ["meta"];
    meta = []
  };{
    name = "true";
    expr = Int 1;
    themes = []; features = [];
    meta = []
  };{
    name = "false";
    expr = Int 0;
    themes = []; features = [];
    meta = []
  };{
    name = "_SUCCESS";
    expr = Int 1;
    themes = []; features = [];
    meta = []
  };{
    name = "_ERROR";
    expr = Int 0;
    themes = []; features = [];
    meta = []
  };{
    name = "_MISSING_RESOURCE";
    expr = Int (-1);
    themes = []; features = [];
    meta = []
  };{
    name = "_OUT_OF_BOUNDS";
    expr = Int (-2);
    themes = []; features = [];
    meta = []
  };{
    name = "_INVALID_TARGET";
    expr = Int (-3);
    themes = []; features = [];
    meta = []
  };{
    name = "_OUT_OF_RANGE";
    expr = Int (-4);
    themes = []; features = [];
    meta = []
  };{
    name = "_OBSTRUCTED";
    expr = Int (-5);
    themes = []; features = [];
    meta = []
  };{
    name = "_MISSING_SPACE";
    expr = Int (-6);
    themes = []; features = [];
    meta = []
  };{
    name = "shoot";
    expr = builtin_func T_Int [T_Dir] (-1);
    themes = ["military";"forestry"]; features = [];
    meta = [
      "range", Int 6;
      "uses", Resource R_Ammo;
    ]
  };{
    name = "look";
    expr = builtin_func T_Int [T_Dir;T_Field] (-2);
    themes = []; features = [];
    meta = [
      "range", Int (-1);
    ]
  };{
    name = "scan";
    expr = builtin_func T_Field [T_Dir;T_Int] (-3);
    themes = []; features = [];
    meta = [
      "range", Int (-1);
    ]
  };{
    name = "mine";
    expr = builtin_func T_Int [T_Dir] (-4);
    themes = ["military"]; features = []; 
    meta = [
      "cost", Int 1;
      "uses", Resource R_Explosive;
    ]
  };{
    name = "move";
    expr = builtin_func T_Int [T_Dir] (-5);
    themes = []; features = [];
    meta = []
  };{
    name = "chop";
    expr = builtin_func T_Int [T_Dir] (-6);
    themes = []; features = [];
    meta = []
  };{
    name = "trench";
    expr = builtin_func T_Int [T_Dir;T_Int] (-7);
    themes = []; features = [];
    meta = [
      "cost", Int 0;
      "range", Int 1;
      "uses", Resource R_Wood;
    ]
  };{
    name = "fortify";
    expr = builtin_func T_Int [T_Dir;T_Int] (-8);
    themes = []; features = [];
    meta = [
      "cost", Int 5;
      "range", Int 1;
      "uses", Resource R_Wood;
    ]
  };{
    name = "bomb";
    expr = builtin_func T_Int [T_Dir;T_Int] (-9);
    themes = ["military"]; features = [];
    meta = [
      "range", Int 4;
      "cost", Int 1;
      "uses", Resource R_Explosive;
    ]
  };{
    name = "write";
    expr = builtin_func T_Int [T_Int] (-10);
    themes = []; features = [];
    meta = []
  };{
    name = "read";
    expr = builtin_func T_Int [] (-11);
    themes = []; features = [];
    meta = []
  };{
    name = "projection";
    expr = builtin_func T_Int [] (-12);
    themes = ["wizardry"]; features = ["fork"];
    meta = [
      "cost", Int 50;
      "upkeep", Int 10;
      "uses", Resource R_Mana;
    ]
  };{
    name = "freeze";
    expr = builtin_func T_Int [T_Dir;T_Int] (-13);
    themes = ["wizardry"]; features = [];
    meta = [
      "cost", Int 25;
      "duration", Int 2;
      "range", Int 5;
      "uses", Resource R_Mana;
    ]
  };{
    name = "fireball";
    expr = builtin_func T_Int [T_Dir] (-14);
    themes = ["wizardry"]; features = [];
    meta = [
      "range", Int 5;
      "cost", Int 10;
      "uses", Resource R_Mana;
    ]
  };{
    name = "meditate";
    expr = builtin_func T_Int [] (-15);
    themes = ["wizardry"]; features = [];
    meta = [
      "amount", Int 20;
    ]
  };{
    name = "dispel";
    expr = builtin_func T_Int [T_Dir] (-16);
    themes = ["wizardry"]; features = [];
    meta = [
      "cost", Int 5;
      "cost_type", Resource R_Mana;
      "uses", Resource R_Mana;
    ]
  };{
    name = "disarm";
    expr = builtin_func T_Int [T_Dir] (-17);
    themes = ["military";"forestry"]; features = [];
    meta = []
  };{
    name = "mana_drain";
    expr = builtin_func T_Int [T_Dir] (-18);
    themes = ["wizardry"]; features = [];
    meta = [
      "cost", Int 20;
      "uses", Resource R_Mana;
    ]
  };{
    name = "pager_set";
    expr = builtin_func T_Int [T_Int] (-19);
    themes = []; features = ["ipc"];
    meta = []
  };{
    name = "pager_read";
    expr = builtin_func T_Int [] (-20);
    themes = []; features = ["ipc"];
    meta = []
  };{
    name = "pager_write";
    expr = builtin_func T_Int [T_Int] (-21);
    themes = []; features = ["ipc"];
    meta = []
  };{
    name = "wall";
    expr = builtin_func T_Int [T_Dir] (-22);
    themes = []; features = [];
    meta = [
      "cost", Int 10;
      "uses", Resource R_Wood;
    ]
  };{
    name = "plant_tree";
    expr = builtin_func T_Int [T_Dir] (-23);
    themes = ["forestry"]; features = [];
    meta = [
      "delay", Int 3;
      "uses", Resource R_Sapling;
    ]
  };{
    name = "bridge";
    expr = builtin_func T_Int [T_Dir] (-24);
    themes = []; features = [];
    meta = [
      "cost", Int 20;
      "uses", Resource R_Wood;
    ]
  };{
    name = "collect";
    expr = builtin_func T_Int [T_Dir;T_Int] (-25);
    themes = []; features = [];
    meta = [
      "range", Int 1;
    ]
  };{
    name = "say";
    expr = builtin_func T_Int [T_Int] (-26);
    themes = []; features = ["debug"];
    meta = []
  };{
    name = "mount";
    expr = builtin_func T_Int [T_Dir] (-27);
    themes = []; features = [];
    meta = []
  };{
    name = "dismount";
    expr = builtin_func T_Int [T_Dir] (-28);
    themes = []; features = [];
    meta = []
  };{
    name = "boat";
    expr = builtin_func T_Int [T_Dir] (-29);
    themes = []; features = [];
    meta = [
      "cost", Int 30;
      "uses", Resource R_Wood;
      "capacity", Int 4;
      "wood_cap", Int 50;
      "clay_cap", Int 50;
      "ammo_cap", Int 100;
      "sapling_cap", Int 20;
      "beartrap_cap", Int 20;
      "explosive_cap", Int 10;
      "metal_cap", Int 10;
    ]
  };{
    name = "bear_trap";
    expr = builtin_func T_Int [T_Dir] ( -30);
    themes = ["forestry"]; features = [];
    meta = [
      "uses", Resource R_BearTrap;
    ]
  };{
    name = "throw_clay";
    expr = builtin_func T_Int [T_Dir;T_Int] (-31);
    themes = ["pottery"]; features = [];
    meta = [
      "cost", Int 1;
      "range", Int 3;
      "uses", Resource R_Clay;
    ]
  };{
    name = "clay_golem";
    expr = builtin_func T_Int [] (-32);
    themes = ["pottery"]; features = ["fork"];
    meta = [
      "cost", Int 5;
      "uses", Resource R_Clay;
    ]
  };{
    name = "drop";
    expr = builtin_func T_Int [T_Int;T_Resource] (-33);
    themes = []; features = [];
    meta = []
  };{
    name = "take";
    expr = builtin_func T_Int [T_Int;T_Resource] (-34);
    themes = []; features = [];
    meta = []
  };{
    name = "mine_shaft";
    expr = builtin_func T_Int [T_Dir] (-35);
    themes = []; features = [];
    meta = [
      "cost", Int 10;
      "uses", Resource R_Wood;
    ]
  };{
    name = "craft";
    expr = builtin_func T_Int [T_Resource] (-36);
    themes = []; features = [];
    meta = [
      "ammo_per_metal", Int 3;
      "beartraps_per_metal", Int 1;
    ]
  };{
    name = "count";
    expr = builtin_func T_Int [T_Resource] (-37);
    themes = []; features = [];
    meta = []
  };{
    name = "pass";
    expr = builtin_func T_Int [] (-38);
    themes = []; features = [];
    meta = []
  };{
    name = "wait";
    expr = builtin_func T_Int [] (-39);
    themes = []; features = [];
    meta = []
  };{
    name = "obliviate";
    expr = builtin_func T_Int [T_Dir] (-40);
    themes = ["wizardry"]; features = [];
    meta = [
      "cost", Int 20;
      "range", Int 2;
      "uses", Resource R_Mana;
    ]
  };{
    name = "blink";
    expr = builtin_func T_Int [] (-41);
    themes = []; features = [];
    meta = [
      "cost", Int 10;
      "duration", Int 2;
      "uses", Resource R_Mana;
    ]
  };{
    name = "search";
    expr = builtin_func T_Int [T_Resource] (-42);
    themes = []; features = [];
    meta = []
  };
]

let generate_meta_builtin map bs : builtin =
  let entries = List.filter_map (fun b -> if List.is_empty b.meta then None else Some(b.name, b.meta)) bs in
  let value n d : expr = Int (map |> StringMap.find_opt n |> Option.value ~default:d) in
  let elem n e = StructureElement (Some n, Expr(e, 0)) in
  let expr = StructureLiteral(List.map (fun (name, props) -> 
    (
      elem name (StructureLiteral(List.map (fun (prop, default) -> match default with
        | Int i -> elem prop (value (name^"."^prop) i)
        | _ -> elem prop default
      ) props)) 
    )
  ) entries)
  in
  {
    name = "meta";
    expr = expr;
    features = ["meta"];
    themes = [];
    meta = [];
  }

let builtin_types = [
  Type("int", T_Int);
  Type("dir", T_Dir);
  Type("resource", T_Resource);
  Type("field", T_Field);
]
  
let generate_initial_scope () : identifier list =
  let builtins = builtins () in
  let meta = generate_meta_builtin Flags.compile_flags.settings builtins in
  let builtins = (meta :: builtins) 
    |> List.filter (fun b -> themes b.themes && features b.features) in
  List.map (fun b -> Const(b.name, Expr(b.expr,0))) builtins @ builtin_types
  