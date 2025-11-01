open Exceptions

type resource =
    | R_Explosive
    | R_Ammo
    | R_Mana
    | R_Sapling
    | R_Clay
    | R_Wood
    | R_BearTrap

let string_to_resource s = match s with
  | "explosive" -> R_Explosive
  | "ammo" -> R_Ammo
  | "mana" -> R_Mana
  | "sapling" -> R_Sapling
  | "clay" -> R_Clay
  | "wood" -> R_Wood
  | "bear_trap" -> R_BearTrap
  | _ -> raise_failure ("Unknown resource: " ^ s)

let resource_value r = match r with
  | R_Explosive -> 0
  | R_Ammo -> 1
  | R_Mana -> 2
  | R_Sapling -> 3
  | R_Clay -> 4
  | R_Wood -> 5
  | R_BearTrap -> 6


module ResourceMap = Map.Make(struct
  type t = resource
  let compare r0 r1 = Stdlib.compare (resource_value r0) (resource_value r1)
end)

let default_resources : (int*int) ResourceMap.t = ResourceMap.of_list [
  R_Explosive,  (0, -1) ;
  R_Ammo,       (0, -1) ;
  R_Mana,       (0, -1) ;
  R_Sapling,    (0, -1) ;
  R_Clay,       (0, -1) ;
  R_Wood,       (0, -1) ;
  R_BearTrap,   (0, -1) ;
]

