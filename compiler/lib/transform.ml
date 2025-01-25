open Absyn

let rec pull_out_declarations stmt = match stmt with
  | Stmt(Block stmts,ln) -> (
    let (regs,stmts) = 
      List.fold_left (fun acc stmt -> pull_out_declarations stmt::acc) [] stmts |> List.split in
    (regs |> List.rev |> List.flatten, Stmt(Block(List.rev stmts),ln))
  )
  | Stmt(If(v,s0,s1),ln) ->
    let (regs0,s0) = pull_out_declarations s0 in
    let (regs1,s1) = pull_out_declarations s1 in
    (regs0@regs1,Stmt(If(v,s0,s1),ln))
  | Stmt(IfIs(v,alts,opt),ln) ->
    let (alts_vs,alts_stmts) = List.split alts in
    let (vars_alts,alts_stmts) = 
      List.fold_left (fun acc stmt -> pull_out_declarations stmt::acc) [] alts_stmts |> List.split in
    let (opt_vars, opt) = match opt with
    | Some s -> (fun (a,b) -> (a,Some b)) (pull_out_declarations s)
    | None -> ([],opt)
    in
    ((vars_alts |> List.rev |> List.flatten) @ opt_vars, Stmt(IfIs(v,List.combine alts_vs (List.rev alts_stmts),opt),ln))
  | Stmt(While(v,s,None),ln) ->
    let (regs,s) = pull_out_declarations s in
    (regs,Stmt(While(v,s,None),ln))
  | Stmt(While(v,s,Some si),ln) ->
    let (regs,s) = pull_out_declarations s in
    let (regsi,si) = pull_out_declarations si in
    (regs@regsi,Stmt(While(v,s,Some si),ln))
  | Stmt(Declare(typ,n),ln) -> ([Var(typ,n)], Stmt(Assign(Local n,Int 0),ln))
  | Stmt(DeclareAssign(typ,n,v),ln) -> ([Var(typ,n)], Stmt(Assign(Local n, v),ln))
  | _ -> ([], stmt)

let pull_out_declarations_of_file (File(_,stmts)) = 
  match pull_out_declarations (Stmt(Block stmts,0)) with
  | (decs, Stmt(Block stmts, _)) -> File(decs,stmts)
  | _ -> failwith "Declaration pull out failed"
  

module StringMap = Map.Make(String)

let rec rename_variables_of_value map v = match v with
  | Reference(Local n) -> Reference(Local(StringMap.find n map))
  | Binary_op(op,v0,v1) -> Binary_op(op,rename_variables_of_value map v0, rename_variables_of_value map v1)
  | Unary_op(op,v) -> Unary_op(op, rename_variables_of_value map v)
  | Scan(v0,v1) -> Scan(rename_variables_of_value map v0, rename_variables_of_value map v1)
  | Look(v,f) -> Look(rename_variables_of_value map v,f)
  | RandomSet vs -> RandomSet(List.map (rename_variables_of_value map) vs)
  | Flag(v,f) -> Flag(rename_variables_of_value map v, f)
  | Decrement(Local n,pre) -> Decrement(Local(StringMap.find n map), pre)
  | Increment(Local n,pre) -> Increment(Local(StringMap.find n map), pre)
  | _ -> v

let rec rename_variables_of_stmt i map (Stmt(stmt,ln)) = match stmt with
  | If(v,s0,s1) -> 
    let (i,_,s0) = rename_variables_of_stmt i map s0 in
    let (i,_,s1) = rename_variables_of_stmt i map s1 in
    (i, map, Stmt(If(rename_variables_of_value map v, s0, s1),ln))
  | IfIs(v,alts,opt) -> 
    let v = rename_variables_of_value map v in
    let (alt_vs, alt_stmts) = List.split alts in
    let alt_vs = List.map (rename_variables_of_value map) alt_vs in
    let (i,alt_stmts) = List.fold_left (fun (i,acc) s -> 
      let (i,_,s) = rename_variables_of_stmt i map s in (i,(s::acc))
    ) (i,[]) alt_stmts in
    let alts = List.combine alt_vs (List.rev alt_stmts) in
    (match opt with
      | Some s -> 
        let (i,_,s) = rename_variables_of_stmt i map s in (i,map,Stmt(IfIs(v,alts,Some s),ln))
      | None -> (i,map,Stmt(IfIs(v,alts,None),ln))
    )
  | Block stmts -> 
    let (i,_,stmts) = List.fold_left (fun (i,map,acc) stmt -> 
      let (i,map,stmt') = rename_variables_of_stmt i map stmt in
      (i,map,stmt'::acc)
    ) (i,map,[]) stmts in
    (i,map,Stmt(Block(List.rev stmts),ln))
  | While(v,s,None) -> 
    let (i,_,s) = rename_variables_of_stmt i map s in
    (i,map,Stmt(While(rename_variables_of_value map v,s,None),ln))
  | While(v,s,Some si) -> 
    let (i,_,s) = rename_variables_of_stmt i map s in
    let (i,_,si) = rename_variables_of_stmt i map si in
    (i,map,Stmt(While(rename_variables_of_value map v,s,Some si),ln))
  | Assign(Local n,v) -> 
    (i,map,Stmt(Assign(Local(StringMap.find n map), rename_variables_of_value map v),ln))
  | Label s -> (i,map,Stmt(Label s,ln))
  | Move d -> (i,map,Stmt(Move(rename_variables_of_value map d),ln))
  | Shoot d -> (i,map,Stmt(Shoot(rename_variables_of_value map d),ln))
  | Bomb(d,p) -> (i,map,Stmt(Bomb(rename_variables_of_value map d,rename_variables_of_value map p),ln))
  | Mine d -> (i,map,Stmt(Mine(rename_variables_of_value map d),ln))
  | Attack d -> (i,map,Stmt(Attack(rename_variables_of_value map d),ln))
  | Fortify v_o -> (i,map,Stmt(Fortify(Option.map (rename_variables_of_value map) v_o),ln))
  | Trench v_o -> (i,map,Stmt(Trench(Option.map (rename_variables_of_value map) v_o),ln))
  | Declare(t,n) -> 
    let new_name = n^"_"^string_of_int i in
    (i+1,StringMap.add n new_name map,Stmt(Declare(t,new_name),ln))
  | DeclareAssign(t,n,v) ->
    let new_name = n^"_"^string_of_int i in
    (i+1,StringMap.add n new_name map,Stmt(DeclareAssign(t,new_name,rename_variables_of_value map v),ln))
  | _ -> (i,map,Stmt(stmt,ln))

let rename_variables_of_file (File(regs,stmts)) =
  match rename_variables_of_stmt 0 StringMap.empty (Stmt(Block stmts,0)) with
  | (_,_,Stmt(Block stmts,_)) -> File(regs,stmts)
  | _ -> failwith "Renaming failed"
