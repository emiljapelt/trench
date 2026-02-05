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
  | "enemy" -> IsEnemy_Prop
  | "ally" -> IsAlly_Prop
  | _ -> raise_failure ("Unknown field property: "^s)


let prop_index p = match p with
  | Obstruction_Prop ->     0b00000000000000000000000000000001
  | Player_Prop ->          0b00000000000000000000000000000010
  | Trapped_Prop ->         0b00000000000000000000000000000100
  | Flammable_Prop ->       0b00000000000000000000000000001000
  | Cover_Prop ->           0b00000000000000000000000000010000
  | Shelter_Prop ->         0b00000000000000000000000000100000
  (* Meltable 0b00000000000000000000000001000000 *)
  | IsEmpty_Prop ->         0b00000000000000000000000010000000
  | IsTrench_Prop ->        0b00000000000000000000000100000000
  | IsIceBlock_Prop ->      0b00000000000000000000001000000000
  | IsTree_Prop ->          0b00000000000000000000010000000000
  | IsOcean_Prop ->         0b00000000000000000000100000000000
  | IsWall_Prop ->          0b00000000000000000001000000000000
  | IsBridge_Prop ->        0b00000000000000000010000000000000
  | IsClay_Prop ->          0b00000000000000000100000000000000
  | IsMineShaft_Prop ->     0b00000000000000001000000000000000
  | IsMountain_Prop ->      0b00000000000000010000000000000000
  | IsEnemy_Prop ->         0b00000000000000100000000000000000
  | IsAlly_Prop ->          0b00000000000001000000000000000000