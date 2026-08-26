open Absyn
open Trg
open Resources
open ProgramRep
open Flags
open Helpers

let themes ts =
  List.is_empty ts || List.exists (fun t -> StringSet.mem t compile_flags.themes) ts

let features fs = 
  List.for_all (fun f -> StringSet.mem f compile_flags.features) fs

let builtin_func ret args addr =
  ASM(TE_Func(ret,args), [Instr_Place; I(addr)])

let structure elements = StructureLiteral(List.map (fun (n,e) -> StructureElement(Some n, Expr(e,0))) elements)

type builtin = {
  name: string;
  expr: expr;
  themes: string list;
  features: string list;
  meta: meta list;
}

and value_link =
  | No
  | Impl
  | Setting of string list

and meta = 
  | Value of string * expr * value_link
  | Structure of string * meta list

let rec translate_meta loc meta =
  let value ns i = 
    let rec aux ns settings = match ns with
      | [] -> load_int settings
      | h::t -> Option.fold ~none:i ~some:(aux t) (find_entry_opt h settings)
    in
    Int (aux ns Flags.compile_flags.settings)
  in
  match meta with
  | Value(n,Int i, Impl) -> (n, value (n::loc) i)
  | Value(n,Int i, Setting s) -> (n, value s i)
  | Value(n,e,_) -> (n,e)
  | Structure(n, entries) -> (n, StructureLiteral(
    entries 
    |> List.map (translate_meta (n::loc))
    |> List.map (fun (n,e) -> StructureElement(Some n, Expr(e,0)))
  ))



(*
type _builtin_info = {
  name: string;
  themes: string list;
  features: string list;
  value: _builtin;
}

and _builtin = 
  | Atom of expr * string option 
  | Structure of _builtin_info list


let _builtins () : _builtin_info list = [
  {
    name = "fireball";
    themes = ["wizardry"]; features = [];
    value = Structure [{
      name = "cast";
      themes = []; features = [];
      value = Atom(builtin_func (TE_Identifier "int") [TE_Identifier "dir"] (-14), None);
    };{
      name = "range";
      themes = []; features = ["meta"];
      value = Atom(Int 5, Some "fireball.range");
    };{
      name = "cost";
      themes = []; features = ["meta"];
      value = Structure [{
        name = "resource";
        themes = []; features = [];
        value = Atom (Resource R_Mana, None);
      };{
        name = "amount";
        themes = []; features = [];
        value = Atom(Int 10, Some "fireball.cost")
      }]
    }]
  }
]

let rec translate_builtin settings builtin = match builtin with
  | Atom(Int i, Some setting) -> Int (settings |> StringMap.find_opt setting |> Option.value ~default:i)
  | Atom(expr, _) -> expr 
  | Structure entries -> StructureLiteral (
    entries 
    |> List.filter_map (translate_builtin_info settings)
    |> List.map (fun (n,e) -> StructureElement(Some n, Expr(e,0))))

and translate_builtin_info settings (info : _builtin_info) = 
  if features info.features && themes info.themes 
  then Some((info.name, translate_builtin settings info.value))
  else None 

let translate_builtins settings = 
  _builtins ()
  |> List.filter_map (translate_builtin_info settings)
  |> List.map (fun (n,e) -> Const(n, Expr(e,0)))
*)



let builtins () : builtin list = [
  {
    name = "x";
    expr = ASM(TE_Identifier "int", [Instr_Meta; I(0)]);
    themes = []; features = ["meta"];
    meta = [];
  };{
    name = "y";
    expr = ASM(TE_Identifier "int", [Instr_Meta; I(1)]);
    themes = []; features = ["meta"];
    meta = [];
  };
  {
    name = "id";
    expr = ASM(TE_Identifier "int", [Instr_Meta; I(2)]);
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
    expr = ASM(TE_Identifier "int", [Instr_Meta; I(5)]);
    themes = []; features = ["meta"];
    meta = [];
  };{
    name = "actions";
    expr = ASM(TE_Identifier "int", [Instr_Meta; I(6)]);
    themes = []; features = ["meta"];
    meta = []
  };{
    name = "map";
    expr = structure[
      ("width", Int Flags.compile_flags.map_width);
      ("height", Int Flags.compile_flags.map_height);
    ];
    themes = []; features = ["meta"];
    meta = []
  };{
    name = "player";
    expr = structure[
      ("x", ASM(TE_Identifier "int",[Instr_Meta; I(0)]));
      ("y", ASM(TE_Identifier "int",[Instr_Meta; I(1)]));
      ("id", ASM(TE_Identifier "int",[Instr_Meta; I(2)]));
      ("actions", ASM(TE_Identifier "int",[Instr_Meta; I(6)]));
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
    expr = builtin_func (TE_Identifier "int") [TE_Identifier "dir"] (-1);
    themes = ["military";"forestry"]; features = [];
    meta = [
      Value ("range", Int 6, Impl);
      Structure ("cost", [
        Value ("resource", Resource R_Ammo, No);
        Value ("amount", Int 1, No);
      ])
    ]
  };{
    name = "look";
    expr = builtin_func (TE_Identifier "int") [TE_Identifier "dir";TE_Identifier "field"] (-2);
    themes = []; features = [];
    meta = [
      Value ("range", Int (-1), Impl);
    ]
  };{
    name = "scan";
    expr = builtin_func (TE_Identifier "field") [TE_Identifier "dir";TE_Identifier "int"] (-3);
    themes = []; features = [];
    meta = [
      Value ("range", Int (-1), Impl);
    ]
  };{
    name = "mine";
    expr = builtin_func (TE_Identifier "int") [TE_Identifier "dir"] (-4);
    themes = ["military"]; features = []; 
    meta = [
      Structure ("cost", [
        Value ("resource", Resource R_Explosive, No);
        Value ("amount", Int 1, Setting ["mine";"cost"])
      ])
    ]
  };{
    name = "move";
    expr = builtin_func (TE_Identifier "int") [TE_Identifier "dir"] (-5);
    themes = []; features = [];
    meta = []
  };{
    name = "chop";
    expr = builtin_func (TE_Identifier "int") [TE_Identifier "dir"] (-6);
    themes = []; features = [];
    meta = []
  };{
    name = "trench";
    expr = builtin_func (TE_Identifier "int") [TE_Identifier "dir";TE_Identifier "int"] (-7);
    themes = []; features = [];
    meta = [
      Value ("range", Int 1, Impl);
      Structure ("cost", [
        Value ("resource", Resource R_Wood, No);
        Value ("amount", Int 0, Setting ["trench";"cost"]);
      ])
    ]
  };{
    name = "fortify";
    expr = builtin_func (TE_Identifier "int") [TE_Identifier "dir";TE_Identifier "int"] (-8);
    themes = []; features = [];
    meta = [
      Value ("range", Int 1, Impl);
      Structure ("cost", [
        Value ("resource", Resource R_Wood, No);
        Value ("amount", Int 5, Setting ["fortify";"cost"]);
      ])
    ]
  };{
    name = "bomb";
    expr = builtin_func (TE_Identifier "int") [TE_Identifier "dir";TE_Identifier "int"] (-9);
    themes = ["military"]; features = [];
    meta = [
      Value ("range", Int 4, Impl);
      Structure ("cost", [
        Value ("resource", Resource R_Explosive, No);
        Value ("amount", Int 1, Setting ["bomb";"cost"]);
      ])
    ]
  };{
    name = "write";
    expr = builtin_func (TE_Identifier "int") [TE_Identifier "int"] (-10);
    themes = []; features = [];
    meta = []
  };{
    name = "read";
    expr = builtin_func (TE_Identifier "int") [] (-11);
    themes = []; features = [];
    meta = []
  };{
    name = "projection";
    expr = builtin_func (TE_Identifier "int") [] (-12);
    themes = ["wizardry"]; features = ["fork"];
    meta = [
      Value ("upkeep", Int 10, Impl);
      Structure ("cost", [
        Value ("resource", Resource R_Mana, No);
        Value ("amount", Int 50, Setting ["projection";"cost"]);
      ])
    ]
  };{
    name = "freeze";
    expr = builtin_func (TE_Identifier "int") [TE_Identifier "dir";TE_Identifier "int"] (-13);
    themes = ["wizardry"]; features = [];
    meta = [
      Value ("duration", Int 2, Impl);
      Value ("range", Int 5, Impl);
      Structure ("cost", [
        Value ("resource", Resource R_Mana, No);
        Value ("amount", Int 25, Setting ["freeze";"cost"]);
      ])
    ]
  };{
    name = "fireball";
    expr = builtin_func (TE_Identifier "int") [TE_Identifier "dir"] (-14);
    themes = ["wizardry"]; features = [];
    meta = [
      Value ("range", Int 5, Impl);
      Structure ("cost", [  
        Value ("resource", Resource R_Mana, No);
        Value ("amount", Int 10, Setting ["fireball";"cost"]);
      ])
    ]
  };{
    name = "meditate";
    expr = builtin_func (TE_Identifier "int") [] (-15);
    themes = ["wizardry"]; features = [];
    meta = [
      Value ("amount", Int 20, Impl);
    ]
  };{
    name = "dispel";
    expr = builtin_func (TE_Identifier "int") [TE_Identifier "dir"] (-16);
    themes = ["wizardry"]; features = [];
    meta = [
      Structure ("cost", [  
        Value ("resource", Resource R_Mana, No);
        Value ("amount", Int 5, Setting ["dispel";"cost"]);
      ])
    ]
  };{
    name = "disarm";
    expr = builtin_func (TE_Identifier "int") [TE_Identifier "dir"] (-17);
    themes = ["military";"forestry"]; features = [];
    meta = []
  };{
    name = "mana_drain";
    expr = builtin_func (TE_Identifier "int") [TE_Identifier "dir"] (-18);
    themes = ["wizardry"]; features = [];
    meta = [
      Structure ("cost", [  
        Value ("resource", Resource R_Mana, No);
        Value ("amount", Int 20, Setting ["mana_drain";"cost"]);
      ])
    ]
  };{
    name = "pager_set";
    expr = builtin_func (TE_Identifier "int") [TE_Identifier "int"] (-19);
    themes = []; features = ["ipc"];
    meta = []
  };{
    name = "pager_read";
    expr = builtin_func (TE_Identifier "int") [] (-20);
    themes = []; features = ["ipc"];
    meta = []
  };{
    name = "pager_write";
    expr = builtin_func (TE_Identifier "int") [TE_Identifier "int"] (-21);
    themes = []; features = ["ipc"];
    meta = []
  };{
    name = "wall";
    expr = builtin_func (TE_Identifier "int") [TE_Identifier "dir"] (-22);
    themes = []; features = [];
    meta = [
      Structure ("cost", [  
        Value ("resource", Resource R_Wood, No);
        Value ("amount", Int 10, Setting ["wall";"cost"]);
      ])
    ]
  };{
    name = "plant_tree";
    expr = builtin_func (TE_Identifier "int") [TE_Identifier "dir"] (-23);
    themes = ["forestry"]; features = [];
    meta = [
      Value ("delay", Int 3, Impl);
      Structure ("cost", [  
        Value ("resource", Resource R_Sapling, No);
        Value ("amount", Int 1, Setting ["plant_tree";"cost"]);
      ])
    ]
  };{
    name = "bridge";
    expr = builtin_func (TE_Identifier "int") [TE_Identifier "dir"] (-24);
    themes = []; features = [];
    meta = [
      Structure ("cost", [  
        Value ("resource", Resource R_Wood, No);
        Value ("amount", Int 20, Setting ["bridge";"cost"]);
      ])
    ]
  };{
    name = "collect";
    expr = builtin_func (TE_Identifier "int") [TE_Identifier "dir";TE_Identifier "int"] (-25);
    themes = []; features = [];
    meta = [
      Value ("range", Int 1, Setting ["collect";"range"]);
    ]
  };{
    name = "say";
    expr = builtin_func (TE_Identifier "int") [TE_Identifier "int"] (-26);
    themes = []; features = ["debug"];
    meta = []
  };{
    name = "mount";
    expr = builtin_func (TE_Identifier "int") [TE_Identifier "dir"] (-27);
    themes = []; features = [];
    meta = []
  };{
    name = "dismount";
    expr = builtin_func (TE_Identifier "int") [TE_Identifier "dir"] (-28);
    themes = []; features = [];
    meta = []
  };{
    name = "boat";
    expr = builtin_func (TE_Identifier "int") [TE_Identifier "dir"] (-29);
    themes = []; features = [];
    meta = [
      Structure ("cost", [  
        Value ("resource", Resource R_Wood, No);
        Value ("amount", Int 30, Setting ["boat";"cost"]);
      ]);
      Value ("capacity", Int 4, Impl);
      Value ("wood_cap", Int 50, Impl);
      Value ("clay_cap", Int 50, Impl);
      Value ("ammo_cap", Int 100, Impl);
      Value ("sapling_cap", Int 20, Impl);
      Value ("beartrap_cap", Int 20, Impl);
      Value ("explosive_cap", Int 10, Impl);
      Value ("metal_cap", Int 10, Impl);
    ]
  };{
    name = "bear_trap";
    expr = builtin_func (TE_Identifier "int") [TE_Identifier "dir"] ( -30);
    themes = ["forestry"]; features = [];
    meta = [
      Structure ("cost", [  
        Value ("resource", Resource R_BearTrap, No);
        Value ("amount", Int 1, Setting ["bear_trap";"cost"]);
      ])
    ]
  };{
    name = "throw_clay";
    expr = builtin_func (TE_Identifier "int") [TE_Identifier "dir";TE_Identifier "int"] (-31);
    themes = ["pottery"]; features = [];
    meta = [
      Value ("range", Int 3, Impl);
      Structure ("cost", [  
        Value ("resource", Resource R_Clay, No);
        Value ("amount", Int 1, Setting ["throw_clay";"cost"]);
      ])
    ]
  };{
    name = "clay_golem";
    expr = builtin_func (TE_Identifier "int") [] (-32);
    themes = ["pottery"]; features = ["fork"];
    meta = [
      Structure ("cost", [  
        Value ("resource", Resource R_Clay, No);
        Value ("amount", Int 5, Setting ["clay_golem";"cost"]);
      ])
    ]
  };{
    name = "drop";
    expr = builtin_func (TE_Identifier "int") [TE_Identifier "int";TE_Identifier "resource"] (-33);
    themes = []; features = [];
    meta = []
  };{
    name = "take";
    expr = builtin_func (TE_Identifier "int") [TE_Identifier "int";TE_Identifier "resource"] (-34);
    themes = []; features = [];
    meta = []
  };{
    name = "mine_shaft";
    expr = builtin_func (TE_Identifier "int") [TE_Identifier "dir"] (-35);
    themes = []; features = [];
    meta = [
      Structure ("cost", [
        Value ("resource", Resource R_Wood, No);
        Value ("amount", Int 10, Setting ["mine_shaft";"cost"]);
      ])
    ]
  };{
    name = "craft";
    expr = builtin_func (TE_Identifier "int") [TE_Identifier "resource"] (-36);
    themes = []; features = [];
    meta = [
      Value ("ammo_per_metal", Int 3, Impl);
      Value ("beartraps_per_metal", Int 1, Impl);
    ]
  };{
    name = "count";
    expr = builtin_func (TE_Identifier "int") [TE_Identifier "resource"] (-37);
    themes = []; features = [];
    meta = []
  };{
    name = "pass";
    expr = builtin_func (TE_Identifier "int") [] (-38);
    themes = []; features = [];
    meta = []
  };{
    name = "wait";
    expr = builtin_func (TE_Identifier "int") [] (-39);
    themes = []; features = [];
    meta = []
  };{
    name = "obliviate";
    expr = builtin_func (TE_Identifier "int") [TE_Identifier "dir"] (-40);
    themes = ["wizardry"]; features = [];
    meta = [
      Value ("range", Int 2, Impl);
      Structure ("cost", [  
        Value ("resource", Resource R_Mana, No);
        Value ("amount", Int 20, Setting ["obliviate";"cost"]);
      ])
    ]
  };{
    name = "blink";
    expr = builtin_func (TE_Identifier "int") [] (-41);
    themes = []; features = [];
    meta = [
      Value ("duration", Int 2, Impl);
      Structure ("cost", [  
        Value ("resource", Resource R_Mana, No);
        Value ("amount", Int 10, Setting ["blink";"cost"]);
      ]) 
    ]
  };{
    name = "search";
    expr = builtin_func (TE_Identifier "int") [TE_Identifier "resource"] (-42);
    themes = []; features = [];
    meta = []
  };
]

let generate_resource_meta () =
  let resources = Flags.compile_flags.resources |> ResourceMap.to_list in
  StructureLiteral (List.map (fun (r, (_, m)) -> StructureElement(Some (resource_to_string r), Expr(Int m,0))) resources)

let generate_meta_builtin bs : builtin =
  let entries = List.filter_map (fun b -> if List.is_empty b.meta then None else Some(b.name, b.meta)) bs in
  let translated = List.map (fun (n, metas) -> (n, List.map (translate_meta [n]) metas)) entries in
  let elements = List.map (fun (name,metas) -> StructureElement(Some name, Expr(StructureLiteral(List.map (fun (entry,expr) -> StructureElement(Some entry, Expr(expr,0))) metas),0))) translated in
  let resource_element = StructureElement(Some "resource", Expr(generate_resource_meta (), 0)) in
  {
    name = "meta";
    expr = StructureLiteral(resource_element :: elements);
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
  
(*let use_modern = false*)

let generate_initial_scope () : identifier list =

  (*if use_modern then Builtins_modern.generate_initial_scope () else*)
  
  let builtins = builtins () in
  let meta = generate_meta_builtin builtins in
  let builtins = (meta :: builtins) 
    |> List.filter (fun b -> themes b.themes && features b.features) in
  List.map (fun b -> Const(b.name, Expr(b.expr,0))) builtins
  

let get_syscalls () = [ (* EXPERIMENTAL *)
  Const("set_color", Expr(builtin_func (TE_Identifier "int") [TE_Array(TE_Identifier "int", Expr(Int 2,0)); TE_Identifier "int"; TE_Identifier "int"] (-43), 0));
  Const("set_symbol", Expr(builtin_func (TE_Identifier "int") [TE_Array(TE_Identifier "int", Expr(Int 2,0)); TE_Identifier "int"] (-44), 0));
  Const("render", Expr(builtin_func (TE_Identifier "int") [TE_Identifier "int"] (-45), 0));
]