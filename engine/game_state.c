#include <stdlib.h>
#include <string.h>

#include "game_rules.h"
#include "game_state.h"
#include "player.h"
#include "util.h"
#include "visual.h"

game_state* _gs;
game_rules* _gr;

void create_players(char* player_info) {

    player_state* pss = malloc(sizeof(player_state) * _gs->player_count);
    int p = 0;
    for(int i = 0; i < _gs->player_count; i++) {
        int* player_stack = malloc(sizeof(int)*1000);

        int id = ((int*)(player_info+p))[0];
        int x = ((int*)(player_info+p))[1];
        int y = ((int*)(player_info+p))[2];
        int dl = ((int*)(player_info+p))[3];
        char* d = player_info+p+(4*sizeof(int));

        int regs_len = 0;
        while(d[regs_len] != ':') regs_len++;
        int regs;
        if (regs_len) {
            regs = 1;
            for (int r = 0; r < regs_len; r++) {
                if (d[i] == ',') regs++;
            }
        } else regs = 0;

        {
            int directive_index = 0;
            for(int r = 0; r < regs; r++) {
                int num_len = numeric_size(d, directive_index);
                player_stack[r] = sub_str_to_int(d, directive_index, num_len);
                directive_index += num_len+1;
            }
        }

        char* directive = malloc((dl-regs_len)+1); directive[dl-regs_len] = 0;
        memcpy(directive, d+regs_len+1, dl-regs_len);

        pss[i] = (player_state){
            .alive = 1,
            .id = id,
            .stack = player_stack,
            .sp = regs,
            .directive = directive,
            .directive_len = dl-(regs_len+1),
            .dp = 0,
            .x = x,
            .y = y,
            .bombs = _gr->bombs,
            .shots = _gr->shots,
        }; 
        //build_field(inits[i]->x, inits[i]->y, gs); // Inital player trench
        p += (4*sizeof(int)+dl);
    }
    _gs->players = pss;
}


field_state* get_field(int x, int y) {
    return _gs->board + ((y * _gs->board_x) + x);
}

void set_field(int x, int y, field_state* f) {
    _gs->board[(y * _gs->board_x) + x] = *f;
}

void fortify_field(int x, int y) {
    _gs->board[(y * _gs->board_x) + x].fortified = 1;
}

void destroy_field(int x, int y) {
    _gs->board[(y * _gs->board_x) + x].destroyed = 1;
}

void build_field(int x, int y) {
    _gs->board[(y * _gs->board_x) + x].destroyed = 0;
    _gs->board[(y * _gs->board_x) + x].trenched = 1;
}

void set_visual(int x, int y, const char* visual) {
    _gs->board[(y * _gs->board_x) + x].visual = visual;
    _gs->board[(y * _gs->board_x) + x].vset = 1;
    
}
void unset_visual(int x, int y) {
    _gs->board[(y * _gs->board_x) + x].vset = 0;
}


void explode_field(int x, int y) {
    field_state* fld = get_field(x,y);
    if (fld->fortified) fld->fortified = 0;
    else if (fld->trenched) {
        fld->trenched = 0;
        fld->destroyed = 1;
        for(int p = 0; p < _gs->player_count; p++) {
            if (_gs->players[p].x == x && _gs->players[p].y == y) {
                kill_player(_gs->players+p);
            }
        }
    }
}

void bomb_field(int x, int y) {
    field_state* fld = get_field(x,y);
    set_visual(x,y,EXPLOSION);
    print_board();
    sleep(500);

    explode_field(x,y);

    unset_visual(x,y);
    print_board();
    sleep(250);
}

void add_bomb(int x, int y, player_state* ps) {
    bomb_chain* link = malloc(sizeof(bomb_chain));
    link->player_id = ps->id;
    link->next = _gs->bomb_chain;
    link->x = x;
    link->y = y;
    _gs->bomb_chain = link;
}

void update_bomb_chain(player_state* ps) {
    bomb_chain* bc = _gs->bomb_chain;
    bomb_chain** prev = &_gs->bomb_chain;
    
    while(bc != NULL) {
        if(bc->player_id == ps->id) {
            bomb_field(bc->x, bc->y);
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