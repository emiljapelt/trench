open Absyn
open Trg
open Resources
open ProgramRep
open Flags
open Helpers

let satisfy_themes ts =
  List.is_empty ts || List.exists (fun t -> StringSet.mem t compile_flags.themes) ts

let satisfy_features fs = 
  List.for_all (fun f -> StringSet.mem f compile_flags.features) fs

let builtin_func ret args addr =
  ASM(TE_Func(ret,args), [Instr_Place; I(addr)])

type builtin_value = {
  name: string;
  expr: expr;
  link: string list;
  themes: string list;
  features: string list;
}

type builtin_structure = { 
  name: string;
  elements: builtin list;
  themes: string list;
  features: string list;
}

and builtin = 
  | Value of builtin_value
  | Structure of builtin_structure

let themes_of = function
  | Value v -> v.themes
  | Structure s -> s.themes 

let features_of = function
  | Value v -> v.features
  | Structure s -> s.features 

let rec translate_builtin builtin = 
  let value ns i = 
    let rec aux ns settings = match ns with
      | [] -> load_int settings
      | h::t -> Option.fold ~none:i ~some:(aux t) (find_entry_opt h settings)
    in
    Int (aux ns Flags.compile_flags.settings)
  in
  if not(satisfy_themes(themes_of builtin) && satisfy_features(features_of builtin)) 
  then None
  else match builtin with 
  | Value v -> (match v.expr, v.link with
    | Int i, s -> Some (v.name, value s i)
    | _,_ -> Some (v.name, v.expr)
  )
  | Structure s -> Some(s.name, StructureLiteral(
    s.elements
    |> List.filter_map (translate_builtin)
    |> List.map (fun (n,e) -> StructureElement(Some n, Expr(e,0)))
  ))

let value = {
  name = "";
  expr = Null;
  link = [];
  themes = [];
  features = [];
}

let structure = {
  name = "";
  elements = [];
  themes = [];
  features = [];
}


let builtins () : builtin list = [
  Value {value with
    name = "x";
    expr = ASM(TE_Identifier "int", [Instr_Meta; I(0)]);
    features = ["meta"]
  };

  Value {value with
    name = "y";
    expr = ASM(TE_Identifier "int", [Instr_Meta; I(1)]);
    features = ["meta"]
  };

  Value {value with
    name = "id";
    expr = ASM(TE_Identifier "int", [Instr_Meta; I(1)]);
    features = ["meta"]
  };

  Value {value with
    name = "map_width";
    expr = Int Flags.compile_flags.map_width;
    features = ["meta"];
  };

  Value {value with
    name = "map_height";
    expr = Int Flags.compile_flags.map_height;
    features = ["meta"];
  };

  Value {value with
    name = "round";
    expr = ASM(TE_Identifier "int", [Instr_Meta; I(5)]);
    features = ["meta"];
  };

  Value {value with
    name = "actions";
    expr = ASM(TE_Identifier "int", [Instr_Meta; I(6)]);
    features = ["meta"];
  };
  
  Structure {structure with
    name = "map";
    elements = [
      Value {value with
        name = "width"; 
        expr = Int Flags.compile_flags.map_width;
      };
      Value {value with
        name = "height";
        expr = Int Flags.compile_flags.map_height;
      };
    ];
    features = ["meta"];
  };
  
  Structure {structure with
    name = "player";
    elements = [
      Value {value with 
        name = "x";
        expr = ASM(TE_Identifier "int",[Instr_Meta; I(0)]);
      };
      Value {value with 
        name = "y";
        expr = ASM(TE_Identifier "int",[Instr_Meta; I(1)]);
      };
      Value {value with 
        name = "id";
        expr = ASM(TE_Identifier "int",[Instr_Meta; I(2)]);
      };
      Value {value with 
        name = "actions";
        expr = ASM(TE_Identifier "int",[Instr_Meta; I(6)]);
      };
    ];
    features = ["meta"];
  };
  
  Value {value with
    name = "true";
    expr = Int 1;
  };

  Value {value with
    name = "false";
    expr = Int 0;
  };

  Structure{structure with
    name = "gun";
    themes = ["military";"forestry"];
    elements = [
      Value {value with 
        name = "shoot";
        expr = builtin_func (TE_Identifier "int") [TE_Identifier "dir"] (-1);
      };
      Value {value with 
        name = "range";
        features = ["meta"];
        expr = Int 6;
        link = ["shoot";"range"];
      };
      Structure {structure with
        name = "cost";
        features = ["meta"];
        elements = [
          Value {value with 
            name = "resource";
            expr = Resource R_Ammo;
          };
          Value {value with 
            name = "amount";
            expr = Int 1;
            link = ["shoot";"cost"]
          };
        ];
      };
    ]
  };

  Value {value with
    name = "say";
    features = ["debug"];
    expr = builtin_func (TE_Identifier "int") [TE_Identifier "int"] (-26);
  }
]


(*let builtins () : builtin list = [
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
    expr = builtin_func (TE_Identifier "int") [T_Dir] (-1);
    themes = ["military";"forestry"]; features = [];
    meta = [
      Value ("range", Int 6, Impl);
      Structure ("cost", [
        Value ("resource", Resource R_Ammo, No);
        Value ("amount", Int 1, No);
      ])
    ]
  };{
    name = "gun";
    expr = structure
    themes = ["military";"forestry"]; features = []; meta = [];
  };{
    name = "look";
    expr = builtin_func (TE_Identifier "int") [T_Dir;TE_Identifier "field"] (-2);
    themes = []; features = [];
    meta = [
      Value ("range", Int (-1), Impl);
    ]
  };{
    name = "scan";
    expr = builtin_func TE_Identifier "field" [T_Dir;TE_Identifier "int"] (-3);
    themes = []; features = [];
    meta = [
      Value ("range", Int (-1), Impl);
    ]
  };{
    name = "mine";
    expr = builtin_func (TE_Identifier "int") [T_Dir] (-4);
    themes = ["military"]; features = []; 
    meta = [
      Structure ("cost", [
        Value ("resource", Resource R_Explosive, No);
        Value ("amount", Int 1, Setting "mine.cost")
      ])
    ]
  };{
    name = "move";
    expr = builtin_func (TE_Identifier "int") [T_Dir] (-5);
    themes = []; features = [];
    meta = []
  };{
    name = "chop";
    expr = builtin_func (TE_Identifier "int") [T_Dir] (-6);
    themes = []; features = [];
    meta = []
  };{
    name = "trench";
    expr = builtin_func (TE_Identifier "int") [T_Dir;TE_Identifier "int"] (-7);
    themes = []; features = [];
    meta = [
      Value ("range", Int 1, Impl);
      Structure ("cost", [
        Value ("resource", Resource R_Wood, No);
        Value ("amount", Int 0, Setting "trench.cost");
      ])
    ]
  };{
    name = "fortify";
    expr = builtin_func (TE_Identifier "int") [T_Dir;TE_Identifier "int"] (-8);
    themes = []; features = [];
    meta = [
      Value ("range", Int 1, Impl);
      Structure ("cost", [
        Value ("resource", Resource R_Wood, No);
        Value ("amount", Int 5, Setting "fortify.cost");
      ])
    ]
  };{
    name = "bomb";
    expr = builtin_func (TE_Identifier "int") [T_Dir;TE_Identifier "int"] (-9);
    themes = ["military"]; features = [];
    meta = [
      Value ("range", Int 4, Impl);
      Structure ("cost", [
        Value ("resource", Resource R_Explosive, No);
        Value ("amount", Int 1, Setting "bomb.cost");
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
        Value ("amount", Int 50, Setting "projection.cost");
      ])
    ]
  };{
    name = "freeze";
    expr = builtin_func (TE_Identifier "int") [T_Dir;TE_Identifier "int"] (-13);
    themes = ["wizardry"]; features = [];
    meta = [
      Value ("duration", Int 2, Impl);
      Value ("range", Int 5, Impl);
      Structure ("cost", [
        Value ("resource", Resource R_Mana, No);
        Value ("amount", Int 25, Setting "freeze.cost");
      ])
    ]
  };{
    name = "fireball";
    expr = builtin_func (TE_Identifier "int") [T_Dir] (-14);
    themes = ["wizardry"]; features = [];
    meta = [
      Value ("range", Int 5, Impl);
      Structure ("cost", [  
        Value ("resource", Resource R_Mana, No);
        Value ("amount", Int 10, Setting "fireball.cost");
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
    expr = builtin_func (TE_Identifier "int") [T_Dir] (-16);
    themes = ["wizardry"]; features = [];
    meta = [
      Structure ("cost", [  
        Value ("resource", Resource R_Mana, No);
        Value ("amount", Int 5, Setting "dispel.cost");
      ])
    ]
  };{
    name = "disarm";
    expr = builtin_func (TE_Identifier "int") [T_Dir] (-17);
    themes = ["military";"forestry"]; features = [];
    meta = []
  };{
    name = "mana_drain";
    expr = builtin_func (TE_Identifier "int") [T_Dir] (-18);
    themes = ["wizardry"]; features = [];
    meta = [
      Structure ("cost", [  
        Value ("resource", Resource R_Mana, No);
        Value ("amount", Int 20, Setting "mana_drain.cost");
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
    expr = builtin_func (TE_Identifier "int") [T_Dir] (-22);
    themes = []; features = [];
    meta = [
      Structure ("cost", [  
        Value ("resource", Resource R_Wood, No);
        Value ("amount", Int 10, Setting "wall.cost");
      ])
    ]
  };{
    name = "plant_tree";
    expr = builtin_func (TE_Identifier "int") [T_Dir] (-23);
    themes = ["forestry"]; features = [];
    meta = [
      Value ("delay", Int 3, Impl);
      Structure ("cost", [  
        Value ("resource", Resource R_Sapling, No);
        Value ("amount", Int 1, Setting "plant_tree");
      ])
    ]
  };{
    name = "bridge";
    expr = builtin_func (TE_Identifier "int") [T_Dir] (-24);
    themes = []; features = [];
    meta = [
      Structure ("cost", [  
        Value ("resource", Resource R_Wood, No);
        Value ("amount", Int 20, Setting "bridge.cost");
      ])
    ]
  };{
    name = "collect";
    expr = builtin_func (TE_Identifier "int") [T_Dir;TE_Identifier "int"] (-25);
    themes = []; features = [];
    meta = [
      Value ("range", Int 1, Setting "collect.range");
    ]
  };{
    name = "say";
    expr = builtin_func (TE_Identifier "int") [TE_Identifier "int"] (-26);
    themes = []; features = ["debug"];
    meta = []
  };{
    name = "mount";
    expr = builtin_func (TE_Identifier "int") [T_Dir] (-27);
    themes = []; features = [];
    meta = []
  };{
    name = "dismount";
    expr = builtin_func (TE_Identifier "int") [T_Dir] (-28);
    themes = []; features = [];
    meta = []
  };{
    name = "boat";
    expr = builtin_func (TE_Identifier "int") [T_Dir] (-29);
    themes = []; features = [];
    meta = [
      Structure ("cost", [  
        Value ("resource", Resource R_Wood, No);
        Value ("amount", Int 30, Setting "boat.cost");
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
    expr = builtin_func (TE_Identifier "int") [T_Dir] ( -30);
    themes = ["forestry"]; features = [];
    meta = [
      Structure ("cost", [  
        Value ("resource", Resource R_BearTrap, No);
        Value ("amount", Int 1, Setting "bear_trap.cost");
      ])
    ]
  };{
    name = "throw_clay";
    expr = builtin_func (TE_Identifier "int") [T_Dir;TE_Identifier "int"] (-31);
    themes = ["pottery"]; features = [];
    meta = [
      Value ("range", Int 3, Impl);
      Structure ("cost", [  
        Value ("resource", Resource R_Clay, No);
        Value ("amount", Int 1, Setting "throw_clay.cost");
      ])
    ]
  };{
    name = "clay_golem";
    expr = builtin_func (TE_Identifier "int") [] (-32);
    themes = ["pottery"]; features = ["fork"];
    meta = [
      Structure ("cost", [  
        Value ("resource", Resource R_Clay, No);
        Value ("amount", Int 5, Setting "clay_golem.cost");
      ])
    ]
  };{
    name = "drop";
    expr = builtin_func (TE_Identifier "int") [TE_Identifier "int";T_Resource] (-33);
    themes = []; features = [];
    meta = []
  };{
    name = "take";
    expr = builtin_func (TE_Identifier "int") [TE_Identifier "int";T_Resource] (-34);
    themes = []; features = [];
    meta = []
  };{
    name = "mine_shaft";
    expr = builtin_func (TE_Identifier "int") [T_Dir] (-35);
    themes = []; features = [];
    meta = [
      Structure ("cost", [
        Value ("resource", Resource R_Wood, No);
        Value ("amount", Int 10, Setting "mine_shaft.cost");
      ])
    ]
  };{
    name = "craft";
    expr = builtin_func (TE_Identifier "int") [T_Resource] (-36);
    themes = []; features = [];
    meta = [
      Value ("ammo_per_metal", Int 3, Impl);
      Value ("beartraps_per_metal", Int 1, Impl);
    ]
  };{
    name = "count";
    expr = builtin_func (TE_Identifier "int") [T_Resource] (-37);
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
    expr = builtin_func (TE_Identifier "int") [T_Dir] (-40);
    themes = ["wizardry"]; features = [];
    meta = [
      Value ("range", Int 2, Impl);
      Structure ("cost", [  
        Value ("resource", Resource R_Mana, No);
        Value ("amount", Int 20, Setting "obliviate.cost");
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
        Value ("amount", Int 10, Setting "blink.cost");
      ]) 
    ]
  };{
    name = "search";
    expr = builtin_func (TE_Identifier "int") [T_Resource] (-42);
    themes = []; features = [];
    meta = []
  };
]*)

let resource_meta () =
  let resources = Flags.compile_flags.resources |> ResourceMap.to_list in
  StructureLiteral (List.map (fun (r, (_, m)) -> StructureElement(Some (resource_to_string r), Expr(Int m,0))) resources)

let builtin_types = [
  Type("int", T_Int);
  Type("dir", T_Dir);
  Type("resource", T_Resource);
  Type("field", T_Field);
]
  
let generate_initial_scope () : identifier list =
  let builtins = ("resource", resource_meta ()) :: List.filter_map translate_builtin (builtins ()) in
  List.map (fun (name,expr) -> Const(name, Expr(expr,0))) builtins @ builtin_types
  