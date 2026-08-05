#!/bin/bash

if [ -e _build ]
then rm -r _build 
fi

mkdir _build &&
cp ../compiler/lib/* ./_build &&
cd _build &&
ocamlc exceptions.ml helpers.ml features.ml themes.ml resources.ml programRep.ml trg.ml absyn.ml flags.ml field_props.ml builtins_modern.ml builtins.ml &&
menhir --infer tr_parser.mly &&
menhir --infer trg_parser.mly &&
ocamllex tr_lexer.mll &&
ocamllex trg_lexer.mll &&
ocamlc -custom -output-complete-obj -o compiler_lib.o exceptions.ml helpers.ml resources.ml trg.ml absyn.ml features.ml themes.ml flags.ml field_props.ml programRep.ml builtins_modern.ml builtins.ml optimize.ml tr_parser.mli tr_parser.ml tr_lexer.ml trg_parser.mli trg_parser.ml trg_lexer.ml toProgramRep.ml compile.ml &&
ocamlc -c ../compiler_wrapper.c &&
cp $(ocamlc -where)/libcamlrun.a compiler_module.a && chmod +w compiler_module.a &&
ar r compiler_module.a compiler_lib.o compiler_wrapper.o  &&
cd .. &&
#-g -fsanitize=address
gcc -o ./trench -I $(ocamlc -where) util.c log.c location.c resource_registry.c game_state.c symbols.c color.c entity.c fields.c array_list.c vehicles.c entity_list.c event_list.c events.c player.c visual.c builtins.c main.c ./_build/compiler_module.a -lcurses -pthread -lzstd -lc -lm -ldl &&
rm -r _build
