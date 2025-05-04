open Absyn
open Exceptions

let string_to_prop s = match s with
  | "obstruction" -> Obstruction_Prop
  | "player" -> Player_Prop
  | "trapped" -> Trapped_Prop
  | "flammable" -> Flammable_Prop
  | "cover" -> Cover_Prop
  | "shelter" -> Shelter_Prop
  | "walkable" -> Walkable_Prop
  | "empty" -> IsEmpty_Prop
  | "trench" -> IsTrench_Prop
  | "ice_block" -> IsIceBlock_Prop
  | "tree" -> IsTree_Prop
  | "ocean" -> IsOcean_Prop
  | "wall" -> IsWall_Prop
  | "bridge" -> IsBridge_Prop
  | _ -> raise_failure ("Unknown field property: "^s)


let prop_index p = match p with
  | Obstruction_Prop -> 0
  | Player_Prop -> 1
  | Trapped_Prop -> 2
  | Flammable_Prop -> 3
  | Cover_Prop -> 4
  | Shelter_Prop -> 5
  | Walkable_Prop -> 7
  | IsEmpty_Prop -> 8
  | IsTrench_Prop -> 9
  | IsIceBlock_Prop -> 10
  | IsTree_Prop -> 11
  | IsOcean_Prop -> 12
  | IsWall_Prop -> 13
  | IsBridge_Prop -> 14