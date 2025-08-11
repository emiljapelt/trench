#!/bin/bash

if [ -e _build ]
then rm -r _build 
fi

mkdir _build &&
cp ../compiler/lib/* ./_build &&
cd _build &&
ocamlc flags.ml features.ml themes.ml absyn.ml exceptions.ml settings.ml field_props.ml helpers.ml programRep.ml typing.ml &&
menhir --infer player_parser.mly &&
menhir --infer game_parser.mly &&
ocamllex player_lexer.mll &&
ocamllex game_lexer.mll &&
ocamlc -custom -output-complete-obj -o compiler_lib.o absyn.ml features.ml themes.ml flags.ml exceptions.ml settings.ml field_props.ml helpers.ml programRep.ml typing.ml optimize.ml player_parser.mli player_parser.ml player_lexer.ml game_parser.mli game_parser.ml game_lexer.ml  transform.ml toProgramRep.ml compile.ml &&
ocamlc -c ../compiler_wrapper.c &&
cp $(ocamlc -where)/libcamlrun.a compiler_module.a && chmod +w compiler_module.a &&
ar r compiler_module.a compiler_lib.o compiler_wrapper.o  &&
cd .. &&
gcc -o ./trench -I $(ocamlc -where) util.c log.c location.c game_state.c color.c entity.c fields.c array_list.c player_list.c vehicles.c entity_list.c event_list.c events.c resource_registry.c player.c visual.c main.c ./_build/compiler_module.a -lcurses -pthread  -lc -lm -ldl &&
rm -r _build
