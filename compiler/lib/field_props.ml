open Absyn
open Exceptions

let all_field = FieldPropSet.of_list [
  Obstruction_Prop;
  Player_Prop;
  Trapped_Prop;
  Flammable_Prop;
  Cover_Prop;
  Shelter_Prop;
  Meltable_Prop;
  IsEmpty_Prop;
  IsTrench_Prop;
  IsIceBlock_Prop;
  IsTree_Prop;
  IsOcean_Prop;
  IsWall_Prop;
  IsBridge_Prop;
  IsClay_Prop;
  IsMineShaft_Prop;
  IsMountain_Prop;
  Enemy_Prop;
  Ally_Prop;
  Vehicle_Prop;
]


let string_to_field s = match s with
  | "obstruction" -> FieldPropSet.singleton Obstruction_Prop
  | "player" -> FieldPropSet.singleton Player_Prop
  | "trapped" -> FieldPropSet.singleton Trapped_Prop
  | "flammable" -> FieldPropSet.singleton Flammable_Prop
  | "cover" -> FieldPropSet.singleton Cover_Prop
  | "shelter" -> FieldPropSet.singleton Shelter_Prop
  | "meltable" -> FieldPropSet.singleton Meltable_Prop
  | "empty" -> FieldPropSet.singleton IsEmpty_Prop
  | "trench" -> FieldPropSet.singleton IsTrench_Prop
  | "ice_block" -> FieldPropSet.singleton IsIceBlock_Prop
  | "tree" -> FieldPropSet.singleton IsTree_Prop
  | "ocean" -> FieldPropSet.singleton IsOcean_Prop
  | "wall" -> FieldPropSet.singleton IsWall_Prop
  | "bridge" -> FieldPropSet.singleton IsBridge_Prop
  | "clay" -> FieldPropSet.singleton IsClay_Prop
  | "mine_shaft" -> FieldPropSet.singleton IsMineShaft_Prop
  | "mountain" -> FieldPropSet.singleton IsMountain_Prop
  | "enemy" -> FieldPropSet.singleton Enemy_Prop
  | "ally" -> FieldPropSet.singleton Ally_Prop
  | "vehicle" -> FieldPropSet.singleton Vehicle_Prop
  | "all" -> all_field
  | "entity" -> FieldPropSet.of_list [Player_Prop;Vehicle_Prop]
  | _ -> raise_failure ("Unknown FieldPropSet property: "^s)
