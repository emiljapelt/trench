#ifndef GAME_STATE_H
#define GAME_STATE_H

#include "player.h"

typedef struct field_state {
    int destroyed : 1;
    int fortified : 1;
    int controller : 4;
} field_state;

typedef struct game_state {
    const int board_x;
    const int board_y;
    player_state* players;
    int player_count;
    field_state* board;
} game_state;

void create_players(int players, player_init* inits, game_state* gs);

field_state* get_field(int x, int y, game_state* gs);
void set_field(int x, int y, game_state* gs, field_state* f);
void fortify_field(int x, int y, game_state* gs);
void destroy_field(int x, int y, game_state gs);
void build_field(int x, int y, int id, game_state* gs);

static inline char in_bounds(int x, int y, game_state* gs) {
    return (0 <= x && x < gs->board_x && 0 <= y && y < gs->board_y);
}

#endif