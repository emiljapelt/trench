open Absyn
open Exceptions

let string_to_prop s = match s with
  | "obstruction" -> Obstruction_Prop
  | "player" -> Player_Prop
  | "trapped" -> Trapped_Prop
  | "flammable" -> Flammable_Prop
  | "cover" -> Cover_Prop
  | "shelter" -> Shelter_Prop
  | "empty" -> IsEmpty_Prop
  | "trench" -> IsTrench_Prop
  | "ice_block" -> IsIceBlock_Prop
  | "tree" -> IsTree_Prop
  | "ocean" -> IsOcean_Prop
  | "wall" -> IsWall_Prop
  | "bridge" -> IsBridge_Prop
  | "clay" -> IsClay_Prop
  | "mine_shaft" -> IsMineShaft_Prop
  | "mountain" -> IsMountain_Prop
  | _ -> raise_failure ("Unknown field property: "^s)


let prop_index p = match p with
  | Obstruction_Prop -> 0
  | Player_Prop -> 1
  | Trapped_Prop -> 2
  | Flammable_Prop -> 3
  | Cover_Prop -> 4
  | Shelter_Prop -> 5
  (* Meltable *)
  | IsEmpty_Prop -> 7
  | IsTrench_Prop -> 8
  | IsIceBlock_Prop -> 9
  | IsTree_Prop -> 10
  | IsOcean_Prop -> 11
  | IsWall_Prop -> 12
  | IsBridge_Prop -> 13
  | IsClay_Prop -> 14
  | IsMineShaft_Prop -> 15
  | IsMountain_Prop -> 16