#include <stdlib.h>
#include <string.h>

#include "game_state.h"

void create_players(int players, player_init* inits, game_state* gs) {
    player_state* pss = malloc(sizeof(player_state) * players);
    for(int i = 0; i < players; i++) {
        int* player_stack = malloc(sizeof(int)*100);
        player_stack[0] = inits[i].x;
        player_stack[1] = inits[i].y;
        player_stack[2] = 10; // bombs
        int id = ++gs->player_count;
        pss[i] = (player_state){
            1,
            id,
            player_stack,
            3,
            inits[i].directive,
            strlen(inits[i].directive),
            0
        }; 
        build_field(inits[i].x, inits[i].y, id, gs);
    }
    gs->players = pss;
}


field_state* get_field(int x, int y, game_state* gs) {
    return gs->board + ((y * gs->board_x) + x);
}

void set_field(int x, int y, game_state* gs, field_state* f) {
    gs->board[(y * gs->board_x) + x] = *f;
}

void fortify_field(int x, int y, game_state* gs) {
    gs->board[(y * gs->board_x) + x].fortified = 1;
}

void destroy_field(int x, int y, game_state gs) {
    gs.board[(y * gs.board_x) + x].destroyed = 1;
}

void build_field(int x, int y, int id, game_state* gs) {
    gs->board[(y * gs->board_x) + x].destroyed = 0;
    gs->board[(y * gs->board_x) + x].controller = id;
}