#include <stdlib.h>
#include <string.h>

#include "game_rules.h"
#include "game_state.h"
#include "loader.h"
#include "player.h"
#include "util.h"
#include "visual.h"

void create_players(parsed_player_file** inits, game_state* gs, game_rules* gr) {

    player_state* pss = malloc(sizeof(player_state) * gs->player_count);
    for(int i = 0; i < gs->player_count; i++) {
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
                player_stack[r] = sub_str_to_int(inits[i]->reg_directive, directive_index, num_len);
                directive_index += num_len+1;
            }
        }

        pss[i] = (player_state){
            .alive = 1,
            .id = inits[i]->id,
            .stack = player_stack,
            .sp = regs,
            .directive = inits[i]->reg_directive + reg_prog_seperator + 1,
            .directive_len = strlen(inits[i]->reg_directive + reg_prog_seperator + 1),
            .step = 0,
            .x = inits[i]->x,
            .y = inits[i]->y,
            .bombs = gr->bombs,
            .shots = gr->shots,
        }; 
        build_field(inits[i]->x, inits[i]->y, inits[i]->id, gs);
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

void build_field(int x, int y, unsigned char id, game_state* gs) {
    gs->board[(y * gs->board_x) + x].destroyed = 0;
    gs->board[(y * gs->board_x) + x].trenched = 1;
}

void shoot_field(int x, int y, char d, game_state* gs) {
    if(d == 'n' || d == 's')
        gs->board[(y * gs->board_x) + x].bullet_state = NS_bullets;
    else if (d == 'e' || d == 'w')
        gs->board[(y * gs->board_x) + x].bullet_state = EW_bullets;
}

void unshoot_field(int x, int y, game_state* gs) {
    gs->board[(y * gs->board_x) + x].bullet_state = No_bullets;
}


void explode_field(int x, int y, game_state* gs) {
    field_state* fld = get_field(x,y,gs);
    if (fld->fortified) fld->fortified = 0;
    else if (fld->trenched) {
        fld->trenched = 0;
        fld->destroyed = 1;
        for(int p = 0; p < gs->player_count; p++) {
            if (gs->players[p].x == x && gs->players[p].y == y) {
                kill_player(gs->players+p);
            }
        }
    }
}

void bomb_field(int x, int y, game_state* gs) {
    field_state* fld = get_field(x,y,gs);
    fld->explosion = 1;
    print_board(gs);
    sleep(500);

    explode_field(x,y,gs);

    fld->explosion = 0;
    print_board(gs);
    sleep(250);
}
void unexplode_field(int x, int y, game_state* gs) {
    gs->board[(y * gs->board_x) + x].explosion = 0;
}

void target_field(int x, int y, game_state* gs) {
    gs->board[(y * gs->board_x) + x].target = 1;
}
void untarget_field(int x, int y, game_state* gs) {
    gs->board[(y * gs->board_x) + x].target = 0;
}

void add_bomb(int x, int y, player_state* ps, game_state* gs) {
    bomb_chain* link = malloc(sizeof(bomb_chain));
    link->player_id = ps->id;
    link->next = gs->bomb_chain;
    link->x = x;
    link->y = y;
    gs->bomb_chain = link;
}

void update_bomb_chain(player_state* ps, game_state* gs) {
    bomb_chain* bc = gs->bomb_chain;
    bomb_chain** prev = &gs->bomb_chain;
    
    while(bc != NULL) {
        if(bc->player_id == ps->id) {
            bomb_field(bc->x, bc->y, gs);
            bomb_chain* link = bc;
            (*prev)->next = bc->next;
            bc = bc->next;
            free(link);
        } else {
            prev = &bc->next;
            bc = bc->next;
        }
    }
}