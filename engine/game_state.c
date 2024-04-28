#include <stdlib.h>
#include <string.h>

#include "game_rules.h"
#include "game_state.h"
#include "loader.h"
#include "util.h"

void create_players(int players, parsed_player_file** inits, game_state* gs, game_rules* gr) {

    player_state* pss = malloc(sizeof(player_state) * players);
    for(int i = 0; i < players; i++) {
        int* player_stack = malloc(sizeof(int)*1000);

        int reg_prog_seperator = 0;
        int regs = 1;
        while (inits[i]->reg_directive[reg_prog_seperator] != ':') {
            if (inits[i]->reg_directive[reg_prog_seperator] == ',') regs++;
            reg_prog_seperator++;
        }

        {
            int directive_index = 0;
            for(int r = 0; r < regs; r++) {
                int num_len = numeric_size(inits[i]->reg_directive, directive_index);
                if (1 || r > 2) player_stack[r] = sub_str_to_int(inits[i]->reg_directive, directive_index, num_len);
                directive_index += num_len+1;
            }
        }

        player_stack[0] = inits[i]->x;
        player_stack[1] = inits[i]->y;
        player_stack[2] = gr->bombs;

        int id = ++gs->player_count;
        pss[i] = (player_state){
            1, // alive
            inits[i]->id, // id
            player_stack, // stack base
            regs, // stack pointer
            inits[i]->reg_directive + reg_prog_seperator + 1, // directive instructions
            strlen(inits[i]->reg_directive + reg_prog_seperator + 1),
            0 // starting step
        }; 
        build_field(inits[i]->x, inits[i]->y, id, gs);
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