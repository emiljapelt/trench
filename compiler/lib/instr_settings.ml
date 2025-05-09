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
}

type cost_and_duration_setting = {
  cost: int;
  duration: int;
}

type instruction_settings = {
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
  fortify: cost_setting;
  projection: cost_and_duration_setting;
  freeze: freeze_setting; 
}

type 'a overwrite = (string * ('a -> int -> 'a)) list

let rec overwritter (def : 'a overwrite) (setting : 'a) (overwrites : (string * int) list) = match overwrites with
  | [] -> setting
  | (name,value)::t -> (match List.find_opt (fun (n,_) -> n = name ) def with
    | Some(_,f) -> overwritter def (f setting value) t
    | None -> raise_failure name
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
]

let cost_and_duration_setting_overwrite = overwritter [
  ("cost", fun s v -> {s with cost = v} );
  ("duration", fun s v -> {s with duration = v} );
]


let rec overwrite_instruction_settings settings overwrites = 
  match overwrites with
  | [] -> settings
  | (name, setting_overwrites) :: t -> ( 
      try (
        t |> overwrite_instruction_settings (
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
          | "fortify" ->    ({ settings with fortify = cost_setting_overwrite settings.fortify setting_overwrites })
          | "projection" -> ({ settings with projection = cost_and_duration_setting_overwrite settings.projection setting_overwrites })
          | "freeze" ->     ({ settings with freeze = freeze_setting_overwrite settings.freeze setting_overwrites })
          | _ -> raise_failure ("No settings for instruction: '" ^ name ^ "'")
        )
      )
    with 
    | Failure (f,ln,msg) -> raise (Failure(f,ln, "Instruction '" ^ name ^ "' has no such setting: " ^ msg))
  )


let default_settings : instruction_settings = {
  fireball = { range = 5; cost = 10 };
  shoot  = { range = 6 };
  bomb = { range = 4 };
  meditate = { amount = 10 };
  dispel = { cost = 5 };
  mana_drain = { cost = 20 };
  wall = { cost = 10 };
  plant_tree = { delay = 3 };
  bridge = { cost = 20 };
  chop = { sapling_chance = 30; wood_gain = 10; };
  fortify = { cost = 5 };
  projection = { cost = 50; duration = 3 };
  freeze = { cost = 25; duration = 2; range = 5; };
}
