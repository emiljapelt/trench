#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "game_rules.h"
#include "game_state.h"
#include "player.h"
#include "util.h"
#include "visual.h"

game_state* _gs;
game_rules* _gr;

directive_info load_directive_to_struct(const char* directive, const int len) {
    int regs_len = 0;
    int regs;
    int* stack = malloc(sizeof(int)*1000);
    
    while(directive[regs_len] != ':') regs_len++;
    if (regs_len) {
        regs = 1;
        for (int r = 0; r < regs_len; r++) {
            if (directive[r] == ',') regs++;
        }
    } else regs = 0;

    {
        int directive_index = 0;
        for(int r = 0; r < regs; r++) {
            int num_len = numeric_size(directive, directive_index);
            stack[r] = sub_str_to_int(directive, directive_index, num_len);
            directive_index += num_len+1;
        }
    }

    char* dir = malloc((len-regs_len)+1); dir[len-regs_len] = 0;
    memcpy(dir, directive+regs_len+1, len-regs_len);

    return (directive_info) {
        .regs_len = regs_len,
        .regs = regs,
        .stack = stack,
        .directive = dir,
        .dir_len = len-(regs_len+1),
    };
}

void create_players(char* player_info) {

    player_state* pss = malloc(sizeof(player_state) * _gs->player_count);
    int p = 0;
    for(int i = 0; i < _gs->player_count; i++) {
        int id = ((int*)(player_info+p))[0];
        int x = ((int*)(player_info+p))[1];
        int y = ((int*)(player_info+p))[2];
        int pl = ((int*)(player_info+p))[3];
        char* pth = player_info+p+(4*sizeof(int));
        int dl = ((int*)(player_info+p+pl))[4];
        char* dir = player_info+p+pl+(5*sizeof(int))+1;
        
        directive_info di = load_directive_to_struct(dir,dl);

        char* path = malloc(pl+1); path[pl] = 0;
        memcpy(path, pth, pl);

        pss[i] = (player_state) {
            .alive = 1,
            .id = id,
            .stack = di.stack,
            .sp = di.regs,
            .path = path,
            .directive = di.directive,
            .directive_len = dl-(di.regs_len+1),
            .dp = 0,
            .x = x,
            .y = y,
            .bombs = _gr->bombs,
            .shots = _gr->shots,
        };
        //build_field(inits[i]->x, inits[i]->y, gs); // Inital player trench
        p += (5*sizeof(int)+dl+pl+1);
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
    if(fld->mine) fld->mine = 0;
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