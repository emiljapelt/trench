open Exceptions

type range_setting =  {
  range: int;
}

type cost_setting = {
  cost: int;
}

type range_and_cost_setting = {
  range: int;
  cost: int;
}

type boat_setting = {
  capacity: int;
  cost: int;
  wood_cap: int;
  clay_cap: int;
  ammo_cap: int;
  sapling_cap: int;
  beartrap_cap: int;
  explosive_cap: int;
  metal_cap: int;
}

type amount_setting = {
  amount: int;
} 

type delay_setting = {
  delay: int;
}

type chop_setting = {
  wood_gain: int;
  sapling_chance: int;
}

type freeze_setting = {
  cost: int;
  duration: int;
  range: int;
  refreeze: int;
}

type cost_and_upkeep_setting = {
  cost: int;
  upkeep: int;
}

type program_setting = {
  stack_size: int;
  size_limit: int;
}

type clay_pit_setting = {
  spread_limit: int;
  contain_limit: int;
  collect_max: int;
}

type craft_setting = {
  ammo_per_metal: int;
  beartraps_per_metal: int;
}

type settings = {
  fireball: range_and_cost_setting;
  shoot: range_setting;
  bomb: range_setting;
  meditate: amount_setting;
  dispel: cost_setting;
  mana_drain: cost_setting;
  wall: cost_setting;
  plant_tree: delay_setting;
  bridge: cost_setting;
  chop: chop_setting;
  fortify: range_and_cost_setting;
  projection: cost_and_upkeep_setting;
  freeze: freeze_setting; 
  look: range_setting;
  scan: range_setting;
  boat: boat_setting;
  program: program_setting;
  throw_clay: range_and_cost_setting;
  clay: clay_pit_setting;
  clay_golem: cost_setting;
  mine_shaft: cost_setting;
  craft: craft_setting;
  trench: range_setting;
  collect: range_setting;
}

type ('a, 'b) overwrite = (string * ('a -> 'b -> 'a)) list

let rec overwritter (def : ('a, 'b) overwrite) (setting : 'a) (overwrites : (string * 'b) list) = match overwrites with
  | [] -> setting
  | (name,value)::t -> (match List.find_opt (fun (n,_) -> n = name ) def with
    | Some(_,f) -> overwritter def (f setting value) t
    | None -> raise_failure ("No such setting: " ^name)
  )

let range_setting_overwrite = overwritter [
  ("range", fun _ v -> {range = v} );
]

let cost_setting_overwrite = overwritter [
  ("cost", fun _ v -> {cost = v} );
]

let range_and_cost_setting_overwrite = overwritter [
  ("range", fun (s: range_and_cost_setting) v -> {s with range = v} );
  ("cost", fun s v -> {s with cost = v} );
]

let amount_setting_overwrite = overwritter [
  ("amount", fun _ v -> {amount = v} );
]

let delay_setting_overwrite = overwritter [
  ("delay", fun _ v -> {delay = v} );
]

let chop_setting_overwrite = overwritter [
  ("wood_gain", fun s v -> {s with wood_gain = v} );
  ("sapling_chance", fun s v -> {s with sapling_chance = v} );
]

let freeze_setting_overwrite = overwritter [
  ("cost", fun (s: freeze_setting) v -> {s with cost = v} );
  ("duration", fun s v -> {s with duration = v} );
  ("range", fun s v -> {s with range = v} );
  ("refreeze", fun s v -> {s with refreeze = v} );
]

let cost_and_upkeep_setting_overwrite = overwritter [
  ("cost", fun s v -> {s with cost = v} );
  ("upkeep", fun s v -> {s with upkeep = v} );
]

let boat_setting_overwrite = overwritter [
  ("capacity", fun s v -> {s with capacity = v});
  ("cost", fun s v -> {s with cost = v});
  ("wood_cap", fun s v -> {s with wood_cap = v});
  ("clay_cap", fun s v -> {s with clay_cap = v});
  ("ammo_cap", fun s v -> {s with ammo_cap = v});
  ("sapling_cap", fun s v -> {s with sapling_cap = v});
  ("beartrap_cap", fun s v -> {s with beartrap_cap = v});
  ("explosive_cap", fun s v -> {s with explosive_cap = v});
  ("metal_cap", fun s v -> {s with metal_cap = v});
]

let program_setting_overwrite = overwritter [
  ("size_limit", fun s v -> {s with size_limit = v});
  ("stack_size", fun s v -> 
    if v < 0 then raise_failure ("stack_size must be greater than 0, but was " ^ string_of_int v) 
    else {s with stack_size = v}
  )
]

let clay_setting_overwrite = overwritter  [
  ("spread_limit", fun s v -> {s with spread_limit = v});
  ("amount_limit", fun s v -> {s with contain_limit = v});
  ("collect_max", fun s v -> {s with collect_max = v});
]

let craft_setting_overwrite = overwritter  [
  ("ammo_per_metal", fun s v -> {s with ammo_per_metal = v});
  ("beartraps_per_metal", fun s v -> {s with beartraps_per_metal = v});

]

let rec overwrite_settings settings overwrites = 
  match overwrites with
  | [] -> settings
  | (name, setting_overwrites) :: t -> ( 
      try (
        t |> overwrite_settings (
          match name with
          | "fireball" ->   ({ settings with fireball = range_and_cost_setting_overwrite settings.fireball setting_overwrites })
          | "shoot" ->      ({ settings with shoot = range_setting_overwrite settings.shoot setting_overwrites })
          | "bomb" ->       ({ settings with bomb = range_setting_overwrite settings.bomb setting_overwrites })
          | "meditate" ->   ({ settings with meditate = amount_setting_overwrite settings.meditate setting_overwrites })
          | "dispel" ->     ({ settings with dispel = cost_setting_overwrite settings.dispel setting_overwrites })
          | "mana_drain" -> ({ settings with mana_drain = cost_setting_overwrite settings.mana_drain setting_overwrites })
          | "wall" ->       ({ settings with wall = cost_setting_overwrite settings.wall setting_overwrites })
          | "plant_tree" -> ({ settings with plant_tree = delay_setting_overwrite settings.plant_tree setting_overwrites })
          | "bridge" ->     ({ settings with bridge = cost_setting_overwrite settings.bridge setting_overwrites })
          | "chop" ->       ({ settings with chop = chop_setting_overwrite settings.chop setting_overwrites })
          | "fortify" ->    ({ settings with fortify = range_and_cost_setting_overwrite settings.fortify setting_overwrites })
          | "projection" -> ({ settings with projection = cost_and_upkeep_setting_overwrite settings.projection setting_overwrites })
          | "freeze" ->     ({ settings with freeze = freeze_setting_overwrite settings.freeze setting_overwrites })
          | "look" ->       ({ settings with look = range_setting_overwrite settings.look setting_overwrites })
          | "scan" ->       ({ settings with scan = range_setting_overwrite settings.scan setting_overwrites })
          | "boat" ->       ({ settings with boat = boat_setting_overwrite settings.boat setting_overwrites })
          | "program" ->    ({ settings with program = program_setting_overwrite settings.program setting_overwrites })
          | "throw_clay" -> ({ settings with throw_clay = range_and_cost_setting_overwrite settings.throw_clay setting_overwrites })
          | "clay" ->       ({ settings with clay = clay_setting_overwrite settings.clay setting_overwrites })
          | "clay_golem" -> ({ settings with clay_golem = cost_setting_overwrite settings.clay_golem setting_overwrites })
          | "mine_shaft" -> ({ settings with mine_shaft = cost_setting_overwrite settings.mine_shaft setting_overwrites })
          | "craft" ->      ({ settings with craft = craft_setting_overwrite settings.craft setting_overwrites })
          | "trench" ->     ({ settings with trench = range_setting_overwrite settings.trench setting_overwrites })
          | "collect" ->     ({ settings with collect = range_setting_overwrite settings.collect setting_overwrites })
          | _ -> raise_failure ("No setting group: '" ^ name ^ "'")
        )
      )
    with 
    | Failure (f,ln,msg) -> raise (Failure(f,ln,msg))
  )


let default_settings : settings = {
  fireball = { range = 5; cost = 10 };
  shoot  = { range = 6 };
  bomb = { range = 4 };
  meditate = { amount = 20 };
  dispel = { cost = 5 };
  mana_drain = { cost = 20 };
  wall = { cost = 10 };
  plant_tree = { delay = 3 };
  bridge = { cost = 20 };
  chop = { sapling_chance = 30; wood_gain = 10; };
  fortify = { cost = 5; range = 1; };
  projection = { cost = 50; upkeep = 10 };
  freeze = { cost = 25; duration = 2; range = 5; refreeze = 0; };
  look = { range = -1; };
  scan = { range = -1; };
  boat = { cost = 30; capacity = 4; wood_cap = 50; clay_cap = 50; ammo_cap = 100; sapling_cap = 20; beartrap_cap = 20; explosive_cap = 10; metal_cap = 10 };
  program = {
    stack_size = 1000;
    size_limit = 0;
  };
  throw_clay = { range = 3; cost = 1 };
  clay = { spread_limit = 1; contain_limit = 100; collect_max = 5; };
  clay_golem = { cost = 5; };
  mine_shaft = { cost = 10 };
  craft = { ammo_per_metal = 3; beartraps_per_metal = 1; };
  trench = { range = 1; };
  collect = { range = 1; };
}
