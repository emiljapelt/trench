#ifndef VISUAL_H
#define VISUAL_H
#include "game_state.h"

static inline void clear_screen(void);
char get_field_char(int x, int y, game_state* gs);
void print_board(game_state* gs);

#endif