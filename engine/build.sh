#!/bin/bash

if [ -e _build ]
then rm -r _build 
fi

mkdir _build &&
cp ../compiler/lib/* ./_build &&
cd _build &&
ocamlc exceptions.ml helpers.ml flags.ml features.ml themes.ml resources.ml absyn.ml settings.ml field_props.ml programRep.ml builtins.ml &&
menhir --infer player_parser.mly &&
menhir --infer game_parser.mly &&
ocamllex player_lexer.mll &&
ocamllex game_lexer.mll &&
ocamlc -custom -output-complete-obj -o compiler_lib.o exceptions.ml  helpers.ml resources.ml absyn.ml features.ml themes.ml flags.ml settings.ml field_props.ml programRep.ml builtins.ml optimize.ml player_parser.mli player_parser.ml player_lexer.ml game_parser.mli game_parser.ml game_lexer.ml toProgramRep.ml compile.ml &&
ocamlc -c ../compiler_wrapper.c &&
cp $(ocamlc -where)/libcamlrun.a compiler_module.a && chmod +w compiler_module.a &&
ar r compiler_module.a compiler_lib.o compiler_wrapper.o  &&
cd .. &&
#gcc -g -fsanitize=address -o ./trench -I $(ocamlc -where) util.c log.c location.c resource_registry.c game_state.c color.c entity.c fields.c array_list.c player_list.c vehicles.c entity_list.c event_list.c events.c player.c visual.c builtins.c main.c ./_build/compiler_module.a -lcurses -pthread -lzstd -lc -lm -ldl &&
gcc -o ./trench -I $(ocamlc -where) util.c log.c location.c resource_registry.c game_state.c color.c entity.c fields.c array_list.c vehicles.c entity_list.c event_list.c events.c player.c visual.c builtins.c main.c ./_build/compiler_module.a -lcurses -pthread -lzstd -lc -lm -ldl &&
rm -r _build
