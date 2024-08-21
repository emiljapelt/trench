open Absyn

let rec pull_out_declarations stmt = match stmt with
  | Stmt(Block stmts,ln) -> (
    let (regs,stmts) = 
      List.fold_left (fun acc stmt -> pull_out_declarations stmt::acc) [] stmts |> List.split in
    (regs |> List.rev |> List.flatten, Stmt(Block(List.rev stmts),ln))
  )
  | Stmt(Declare(typ,n),ln) -> ([Register(typ,n)], Stmt(Assign(Local n,Int 0),ln))
  | Stmt(DeclareAssign(typ,n,v),ln) -> ([Register(typ,n)], Stmt(Assign(Local n, v),ln))
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
  | Look v -> Look(rename_variables_of_value map v)
  | RandomSet vs -> RandomSet(List.map (rename_variables_of_value map) vs)
  | Flag(v,f) -> Flag(rename_variables_of_value map v, f)
  | _ -> v

let rec rename_variables_of_stmt i map (Stmt(stmt,ln)) = match stmt with
  | If(v,s0,s1) -> 
    let (i,_,s0) = rename_variables_of_stmt i map s0 in
    let (i,_,s1) = rename_variables_of_stmt i map s1 in
    (i, map, Stmt(If(rename_variables_of_value map v, s0, s1),ln))
  | Block stmts -> 
    let (i,_,stmts) = List.fold_left (fun (i,map,acc) stmt -> 
      let (i,map,stmt') = rename_variables_of_stmt i map stmt in
      (i,map,stmt'::acc)
    ) (i,map,[]) stmts in
    (i,map,Stmt(Block(List.rev stmts),ln))
  | Repeat _ -> failwith "nope"
  | Assign(Local n,v) -> 
    (i,map,Stmt(Assign(Local(StringMap.find n map), rename_variables_of_value map v),ln))
  | Assign(Global(t,v),nv) -> (i,map,Stmt(Assign(Global(t,rename_variables_of_value map v),rename_variables_of_value map nv),ln))
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
