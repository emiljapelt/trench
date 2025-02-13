open Absyn
open Exceptions

let string_to_prop s = match s with
  | "obstruct" -> Obstruction_Prop
  | "player" -> Player_Prop
  | "trapped" -> Trapped_Prop
  | "flammable" -> Flammable_Prop
  | "cover" -> Cover_Prop
  | "shelter" -> Shelter_Prop
  | _ -> raise_failure ("Unknown field property: "^s)


let prop_index p = match p with
  | Obstruction_Prop -> 0
  | Player_Prop -> 1
  | Trapped_Prop -> 2
  | Flammable_Prop -> 3
  | Cover_Prop -> 4
  | Shelter_Prop -> 5