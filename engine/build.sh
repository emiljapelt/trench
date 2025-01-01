#!/bin/bash

if [ -e _build ]
then rm -r _build 
fi

mkdir _build &&
cp ../compiler/lib/* ./_build &&
cd _build &&
ocamlc flags.ml themes.ml absyn.ml exceptions.ml helpers.ml programRep.ml typing.ml &&
menhir --infer player_parser.mly &&
menhir --infer game_parser.mly &&
ocamllex player_lexer.mll &&
ocamllex game_lexer.mll &&
ocamlc -custom -output-complete-obj -o compiler_lib.o absyn.ml themes.ml flags.ml exceptions.ml helpers.ml programRep.ml typing.ml optimize.ml player_parser.mli player_parser.ml player_lexer.ml game_parser.mli game_parser.ml game_lexer.ml toProgramRep.ml transform.ml compile.ml &&
ocamlc -c ../compiler_wrapper.c &&
cp $(ocamlc -where)/libcamlrun.a compiler_module.a && chmod +w compiler_module.a &&
ar r compiler_module.a compiler_lib.o compiler_wrapper.o  &&
cd .. &&
gcc -o ./trench -I $(ocamlc -where) util.c game_state.c field_scan.c event_list.c events.c resource_registry.c visual.c main.c ./_build/compiler_module.a -lcurses -pthread -lzstd -lc -lm -ldl &&
rm -r _build