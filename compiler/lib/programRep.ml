open Absyn

module StringMap = Map.Make(String)
module StringSet = Set.Make(String)

type instruction =
  | I of int
  | Label of string
  | LabelRef of string
  | Meta_PlayerX
  | Meta_PlayerY
  | Meta_BoardX
  | Meta_BoardY
  | Meta_Resource
  | Meta_PlayerID
  | Instr_Add
  | Instr_Sub
  | Instr_Mul
  | Instr_And
  | Instr_Or
  | Instr_Eq
  | Instr_Not
  | Instr_Lt
  | Instr_Div
  | Instr_Mod
  | Instr_Scan
  | Instr_Random
  | Instr_RandomSet
  | Instr_Place
  | Instr_Access
  | Instr_Swap
  | Instr_Copy
  | Instr_DecStack
  | Instr_FieldProp
  | Instr_Assign
  | Instr_GoToIf
  | Instr_GoTo
  | Instr_Move
  | Instr_Chop
  | Instr_Trench
  | Instr_Fortify
  | Instr_Bomb
  | Instr_Shoot
  | Instr_Wait
  | Instr_Pass
  | Instr_Look
  | Instr_Mine
  | Instr_Read
  | Instr_Write
  | Instr_Projection
  | Instr_Freeze
  | Instr_Fireball
  | Instr_Meditate
  | Instr_Dispel
  | Instr_Disarm
  | Instr_ManaDrain
  | Instr_PagerSet
  | Instr_PagerRead
  | Instr_PagerWrite
  | Instr_Wall
  | Instr_PlantTree
  | Instr_Bridge
  | Instr_Collect
  | Instr_Say
  | Instr_Mount
  | Instr_Dismount
  | Instr_Boat
  | Instr_BearTrap
  | Instr_Call
  | Instr_Return
  | Instr_Declare
  | Instr_GlobalAccess
  | Instr_GlobalAssign
  | Instr_Index

let instruction_to_int label_map instr = match instr with
    | I i -> Some i
    | Label _ -> None
    | LabelRef l -> StringMap.find_opt l label_map
    | Meta_PlayerX -> Some 0
    | Meta_PlayerY -> Some 1
    | Meta_BoardX -> Some 2
    | Meta_BoardY -> Some 3
    | Meta_Resource -> Some 4
    | Meta_PlayerID -> Some 5
    | Instr_Add -> Some 6
    | Instr_Sub -> Some 7
    | Instr_Mul -> Some 8
    | Instr_And -> Some 9 
    | Instr_Or -> Some 10
    | Instr_Eq -> Some 11
    | Instr_Not -> Some 12
    | Instr_Lt -> Some 13
    | Instr_Div -> Some 14
    | Instr_Mod -> Some 15
    | Instr_Scan -> Some 16
    | Instr_Random -> Some 17
    | Instr_RandomSet -> Some 18
    | Instr_Place -> Some 19
    | Instr_Access -> Some 20
    | Instr_Swap -> Some 21
    | Instr_Copy -> Some 22
    | Instr_DecStack -> Some 23
    | Instr_FieldProp -> Some 24
    | Instr_Assign -> Some 25
    | Instr_GoToIf -> Some 26
    | Instr_GoTo -> Some 27
    | Instr_Move -> Some 28
    | Instr_PlantTree -> Some 29
    | Instr_Trench -> Some 30
    | Instr_Fortify -> Some 31
    | Instr_Bomb -> Some 32
    | Instr_Shoot -> Some 33
    | Instr_Wait -> Some 34
    | Instr_Pass -> Some 35
    | Instr_Look -> Some 36
    | Instr_Mine -> Some 37
    | Instr_Chop -> Some 38
    | Instr_Read -> Some 39
    | Instr_Write -> Some 40
    | Instr_Projection -> Some 41
    | Instr_Freeze -> Some 42
    | Instr_Fireball -> Some 43
    | Instr_Meditate -> Some 44
    | Instr_Dispel -> Some 45
    | Instr_Disarm -> Some 46
    | Instr_ManaDrain -> Some 47
    | Instr_PagerSet -> Some 48
    | Instr_PagerWrite -> Some 49
    | Instr_PagerRead -> Some 50
    | Instr_Wall -> Some 51
    | Instr_Bridge -> Some 52
    | Instr_Collect -> Some 53
    | Instr_Say -> Some 54
    | Instr_Mount -> Some 55
    | Instr_Dismount -> Some 56
    | Instr_Boat -> Some 57
    | Instr_BearTrap -> Some 58
    | Instr_Call -> Some 59
    | Instr_Return -> Some 60
    | Instr_Declare -> Some 61
    | Instr_GlobalAccess -> Some 62
    | Instr_GlobalAssign -> Some 63
    | Instr_Index -> Some 64

let label_set pp =
  let rec aux pp set = match pp with
    | [] -> set
    | Label n :: t -> ( 
      if StringSet.mem n set then failwith ("Duplicate label: "^n)   
      else aux t (StringSet.add n set)
    )
    | _::t -> aux t set
  in
  aux pp StringSet.empty

let rec check_labels_exist pp set = match pp with
  | [] -> ()
  | LabelRef(n)::t -> if StringSet.mem n set then check_labels_exist t set else failwith ("Undefined label: "^n)
  | _ ::t -> check_labels_exist t set

let extract_label_indecies pps =
  let rec aux pps i map = match pps with
    | [] -> map
    | Label n :: t -> aux t i (StringMap.add n i map)
    | _::t -> aux t (i + 1) map
  in
  aux pps 0 StringMap.empty

let program_to_int_list instrs =
  check_labels_exist instrs (label_set instrs);
  let label_map = extract_label_indecies instrs in
  instrs
  |> List.filter_map (instruction_to_int label_map)

let available_labels stmt =
  let rec aux (Stmt(stmt,_)) set = match stmt with 
    | Label n -> StringSet.union set (StringSet.singleton n)
    | If(_,t,f) -> StringSet.union (aux t set) (aux f set)
    | IfIs(_,alts,Some el) -> List.fold_left (fun acc (_,s) -> aux s acc) (aux el set) alts 
    | IfIs(_,alts,None) -> List.fold_left (fun acc (_,s) -> aux s acc) set alts 
    | Block(stmts) -> List.fold_left (fun acc s -> aux s acc) set stmts
    | While(_,stmt,_) -> aux stmt set
    | _ -> set
  in
    aux stmt StringSet.empty