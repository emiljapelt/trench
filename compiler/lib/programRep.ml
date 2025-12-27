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
  | Instr_Wait
  | Instr_Pass
  | Instr_Call
  | Instr_Return
  | Instr_Declare
  | Instr_GlobalAccess
  | Instr_GlobalAssign
  | Instr_Index
  | Meta_Round

let instruction_to_int label_map instr = match instr with
    | I i -> Some i
    | Label _ -> None
    | LabelRef l -> StringMap.find_opt l label_map
    | Meta_PlayerX -> Some 0
    | Meta_PlayerY -> Some 1
    | Meta_BoardX -> Some 2
    | Meta_BoardY -> Some 3
    | Meta_PlayerID -> Some 4
    | Instr_Add -> Some 5
    | Instr_Sub -> Some 6
    | Instr_Mul -> Some 7
    | Instr_And -> Some 8 
    | Instr_Or -> Some 9
    | Instr_Eq -> Some 10
    | Instr_Not -> Some 11
    | Instr_Lt -> Some 12
    | Instr_Div -> Some 13
    | Instr_Mod -> Some 14
    | Instr_Random -> Some 15
    | Instr_RandomSet -> Some 16
    | Instr_Place -> Some 17
    | Instr_Access -> Some 18
    | Instr_Swap -> Some 19
    | Instr_Copy -> Some 20
    | Instr_DecStack -> Some 21
    | Instr_FieldProp -> Some 22
    | Instr_Assign -> Some 23
    | Instr_GoToIf -> Some 24
    | Instr_GoTo -> Some 25
    | Instr_Wait -> Some 26
    | Instr_Pass -> Some 27
    | Instr_Call -> Some 28
    | Instr_Return -> Some 29
    | Instr_Declare -> Some 30
    | Instr_GlobalAccess -> Some 31
    | Instr_GlobalAssign -> Some 32
    | Instr_Index -> Some 33
    | Meta_Round -> Some 34

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


let is_var name scopes =
  match List.find_opt (fun (Var(_,n)) -> n = name) scopes.local with
  | Some _ -> true
  | None -> (
    match Option.map (fun scope -> List.find_opt (fun (Var(_,n)) -> n = name) scope) scopes.global |> Option.join with
    | Some _ -> true
    | _ -> false
  )