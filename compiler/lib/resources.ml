open Exceptions

type resource =
    | R_Explosive
    | R_Ammo
    | R_Mana
    | R_Sapling
    | R_Clay
    | R_Wood

let string_to_resource s = match s with
  | "explosive" -> R_Explosive
  | "ammo" -> R_Ammo
  | "mana" -> R_Mana
  | "sapling" -> R_Sapling
  | "clay" -> R_Clay
  | "wood" -> R_Wood
  | _ -> raise_failure ("Unknown resource: " ^ s)

let resource_value r = match r with
  | R_Explosive -> 0
  | R_Ammo -> 1
  | R_Mana -> 2
  | R_Sapling -> 3
  | R_Clay -> 4
  | R_Wood -> 5


module ResourceMap = Map.Make(struct
  type t = resource
  let compare r0 r1 = Stdlib.compare (resource_value r0) (resource_value r1)
end)

let default_resources : (int*int) ResourceMap.t = ResourceMap.of_list [
  R_Explosive, (10, 10) ;
  R_Ammo, (100, 100) ;
  R_Mana, (100, 100) ;
  R_Sapling, (0, 10) ;
  R_Clay, (0, 100) ;
  R_Wood, (0, 50) ;
]

