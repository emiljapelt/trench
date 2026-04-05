open ProgramRep

let cont_optimize_instruction_list instrs =
  let rec aux instrs c = match instrs with
    | [] -> c []
    | Instr_Add::t -> aux t (fun acc -> match acc with
      | I(a)::Instr_Place::I(b)::Instr_Place::acc -> c(I(a+b)::Instr_Place::acc)
      | _ -> c(Instr_Add::acc)
    )
    | Instr_Mul::I(1)::Instr_Place::t -> aux t (fun acc -> c acc)
    | Instr_Mul::t -> aux t (fun acc -> match acc with
      | I(a)::Instr_Place::I(b)::Instr_Place::acc -> c(I(a*b)::Instr_Place::acc)
      | _ -> c(Instr_Mul::acc)
    )
    | Instr_Sub::t -> aux t (fun acc -> match acc with
      | I(a)::Instr_Place::I(b)::Instr_Place::acc -> c(I(b-a)::Instr_Place::acc)
      | _ -> c(Instr_Sub::acc)
    )
    (*| I(_)::Instr_Return::I(_)::Instr_Declare::I(s)::Instr_Return::t ->
      aux t (fun acc -> c(Instr_Return::I(s)::acc))  *) (* Attemt to remove implicit return, breaks test *)
    | h::t -> aux t (fun acc -> c(h::acc))
  in
  aux (List.rev instrs) (List.rev)


let naive_optimize_instruction_list instrs =
  let rec aux instrs acc = match instrs with
    | [] -> acc |> List.rev |> List.flatten
    | Instr_Place::I(a)::Instr_Place::I(b)::op::t -> (match op with
      | Instr_Mul -> aux t ([Instr_Place ; I(a * b)] :: acc)
      | Instr_Add -> aux t ([Instr_Place ; I(a + b)] :: acc)
      | Instr_Sub -> aux t ([Instr_Place ; I(a - b)] :: acc)
      | _ -> aux t ([Instr_Place;I(a);Instr_Place;I(b);op]::acc)
    )
    | h::t -> aux t ([h]::acc)
  in
  aux instrs [] 

let none_optimize_instruction_list instrs = instrs

let optimize_instruction_list = cont_optimize_instruction_list