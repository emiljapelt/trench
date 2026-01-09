open Absyn
open Builtins
open Helpers

let rec count_declarations stmt : int = match stmt with
  | Stmt(Block stmts,_) -> List.fold_left (fun acc stmt -> count_declarations stmt + acc) 0 stmts
  | Stmt(If(_,s0,s1),_) -> count_declarations s0 + count_declarations s1
  | Stmt(IfIs(_,alts,opt),_) ->
    let (_,alts_stmts) = List.split alts in
    let alts_count =
      List.fold_left (fun acc stmt -> count_declarations stmt + acc) 0 alts_stmts in
    let opt_count = match opt with
    | Some s -> (count_declarations s)
    | None -> 0
    in
    alts_count + opt_count
  | Stmt(While(_,s,None),_) -> count_declarations s
  | Stmt(While(_,s,Some si),_) -> count_declarations s + count_declarations si
  | Stmt(Declare(_,_),_) -> 1
  | Stmt(DeclareAssign(Some _,_,_),_) -> 1
  | Stmt(DeclareAssign(None,_,_),_) -> (failwith "Untyped declaration meet in declaration pull out phase")
  | _ -> 0


let rec extract_declarations stmt : variable list = match stmt with
  | Stmt(Block stmts,_) -> (
    let regs = 
      List.map  extract_declarations stmts in
    regs |> List.rev |> List.flatten
  )
  | Stmt(If(_,s0,s1),_) ->
    let (regs0) = extract_declarations s0 in
    let (regs1) = extract_declarations s1 in
    regs0@regs1
  | Stmt(IfIs(_,alts,opt),_) ->
    let (_,alts_stmts) = List.split alts in
    let vars_alts = 
      List.fold_left (fun acc stmt ->  extract_declarations stmt::acc) [] alts_stmts in
    let opt_vars = match opt with
    | Some s ->  extract_declarations s
    | None -> []
    in
    (vars_alts |> List.rev |> List.flatten) @ opt_vars
  | Stmt(While(_,s,None),_) ->
    let regs =  extract_declarations s in
    regs
  | Stmt(While(_,s,Some si),_) ->
    let regs =  extract_declarations s in
    let regsi =  extract_declarations si in
    regs@regsi
  | Stmt(Declare(typ,n),_) -> [Var(typ,n)]
  | Stmt(DeclareAssign(Some typ,n,_),_) -> [Var(typ,n)]
  | Stmt(DeclareAssign(None,_,_),_) -> (failwith "Untyped declaration meet in declaration pull out phase")
  | _ -> []
  

module StringMap = Map.Make(String)

let rec rename_variables_of_target map t =
  match t with
  | Local n -> Local(StringMap.find n map)
  | Array(target,index) -> Array(rename_variables_of_target map target, rename_variables_of_value map index)

and rename_variables_of_value map v = match v with
  | Reference target -> Reference(rename_variables_of_target map target)
  | Binary_op(op,v0,v1) -> Binary_op(op,rename_variables_of_value map v0, rename_variables_of_value map v1)
  | Unary_op(op,v) -> Unary_op(op, rename_variables_of_value map v)
  | RandomSet vs -> RandomSet(List.map (rename_variables_of_value map) vs)
  | FieldProp(v,fp) -> FieldProp(rename_variables_of_value map v, rename_variables_of_value map fp)
  | Decrement(target,pre) -> Decrement(rename_variables_of_target map target, pre)
  | Increment(target,pre) -> Increment(rename_variables_of_target map target, pre)
  | Func(ret,params,body) -> (    
    let new_map = StringMap.merge (fun _ old_value new_value -> match new_value with
    | Some v -> Some v
    | None -> old_value
    ) map (StringMap.of_list(List.map (fun (_,n) -> (n,n)) params @ [("this","this")]))
    in
    let (_,renamed_body) = rename_variables_of_stmt (new_map) body in
    Func(ret,params,renamed_body)
  )
  | Call(f,args) -> Call(rename_variables_of_value map f, List.map (rename_variables_of_value map) args)
  | Ternary(c,a,b) -> Ternary(rename_variables_of_value map c, rename_variables_of_value map a, rename_variables_of_value map b)
  | _ -> v

and rename_variables_of_stmt map (Stmt(stmt,ln)) = match stmt with
  | If(v,s0,s1) -> 
    let  (_,s0) = rename_variables_of_stmt map s0 in
    let  (_,s1) = rename_variables_of_stmt map s1 in
     ( map, Stmt(If(rename_variables_of_value map v, s0, s1),ln))
  | IfIs(v,alts,opt) -> 
    let v = rename_variables_of_value map v in
    let (alt_vs, alt_stmts) = List.split alts in
    let alt_vs = List.map (rename_variables_of_value map) alt_vs in
    let (alt_stmts) = List.map (fun s -> 
      let (_,s) = rename_variables_of_stmt map s in s
     ) alt_stmts in
    let alts = List.combine alt_vs (List.rev alt_stmts) in
    (match opt with
      | Some s -> 
        let  (_,s) = rename_variables_of_stmt map s in  (map,Stmt(IfIs(v,alts,Some s),ln))
      | None ->  (map,Stmt(IfIs(v,alts,None),ln))
    )
  | Block stmts -> 
    let  (_,stmts) = List.fold_left (fun  (map,acc) stmt -> 
      let  (map,stmt') = rename_variables_of_stmt map stmt in
       (map,stmt'::acc)
    )  (map,[]) stmts in
     (map,Stmt(Block(List.rev stmts),ln))
  | While(v,s,None) -> 
    let  (_,s) = rename_variables_of_stmt map s in
     (map,Stmt(While(rename_variables_of_value map v,s,None),ln))
  | While(v,s,Some si) -> 
    let  (_,s) = rename_variables_of_stmt map s in
    let  (_,si) = rename_variables_of_stmt map si in
     (map,Stmt(While(rename_variables_of_value map v,s,Some si),ln))
  | Assign(target,v) -> 
     (map,Stmt(Assign(rename_variables_of_target map target, rename_variables_of_value map v),ln))
  | Label s ->  (map,Stmt(Label s,ln))
  | Declare(t,n) -> 
    let new_name = rename n in
    (StringMap.add n new_name map,Stmt(Declare(t,new_name),ln))
  | DeclareAssign(t,n,v) ->
    let new_name = rename n in
    (StringMap.add n new_name map,Stmt(DeclareAssign(t,new_name,rename_variables_of_value map v),ln))
  | Return v ->  (map,Stmt(Return(rename_variables_of_value map v),ln))
  | Expr e ->  (map,Stmt(Expr(rename_variables_of_value map e),ln))
  | _ ->  (map,Stmt(stmt,ln))


let rename_variables_of_file (File stmts) =
  reset_rename_generator () ;
  match rename_variables_of_stmt builtin_rename_map (Stmt(Block stmts,0)) with
  | (_,Stmt(Block stmts,_)) -> File stmts
  | _ -> failwith "Renaming failed"
