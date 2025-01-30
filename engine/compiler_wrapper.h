#ifndef H_COMPILER_WRAPPER
#define H_COMPILER_WRAPPER

#include "game_state.h"

int compile_game(const char* path, game_rules* gr, game_state* gs);
int compile_player(const char* path, int stack_size, directive_info* result);

#endif