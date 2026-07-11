open Helpers

type instruction =
  | I of int
  | Label of string
  | LabelRef of string
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
  | Instr_BinOr
  | Instr_BinNot
  | Instr_BinAnd
  | Instr_Random
  | Instr_GoToIf
  | Instr_GoTo
  | Instr_Wait
  | Instr_Pass
  | Instr_Call
  | Instr_Return
  | Instr_Declare
  | Instr_Place
  | Instr_Swap
  | Instr_Copy
  | Instr_MoveSP
  | Instr_BP
  | Instr_Index
  | Instr_Extract
  | Instr_LoadGlobal
  | Instr_StoreGlobal
  | Instr_LoadLocal
  | Instr_StoreLocal
  | Instr_Meta
  | Instr_Bits
  | Instr_TCall

let instruction_to_int label_map instr = match instr with
    | I i -> Some i
    | Label _ -> None
    | LabelRef l -> StringMap.find_opt l label_map
    | Instr_Add ->          Some 0
    | Instr_Sub ->          Some 1
    | Instr_Mul ->          Some 2
    | Instr_And ->          Some 3
    | Instr_Or ->           Some 4
    | Instr_Eq ->           Some 5
    | Instr_Not ->          Some 6
    | Instr_Lt ->           Some 7
    | Instr_Div ->          Some 8 
    | Instr_Mod ->          Some 9
    | Instr_BinOr ->        Some 10
    | Instr_BinNot ->       Some 11
    | Instr_BinAnd ->       Some 12
    | Instr_Random ->       Some 13
    | Instr_GoToIf ->       Some 15
    | Instr_GoTo ->         Some 16
    | Instr_Wait ->         Some 17
    | Instr_Pass ->         Some 18
    | Instr_Call ->         Some 19
    | Instr_Return ->       Some 20
    | Instr_Declare ->      Some 21
    | Instr_Place ->        Some 22
    | Instr_Swap ->         Some 23
    | Instr_Copy ->         Some 24
    | Instr_MoveSP ->       Some 25
    | Instr_BP ->           Some 26
    | Instr_Index ->        Some 27
    | Instr_Extract ->      Some 28
    | Instr_LoadGlobal ->   Some 29
    | Instr_StoreGlobal ->  Some 30
    | Instr_LoadLocal ->    Some 31
    | Instr_StoreLocal ->   Some 32
    | Instr_Meta ->         Some 33
    | Instr_Bits ->         Some 34
    | Instr_TCall ->        Some 35

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

let rec check_labels_exist instrs set = match instrs with
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
