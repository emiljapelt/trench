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

field_state* get_field(const int x, const int y) {
    return _gs->board + ((y * _gs->board_x) + x);
}

void set_field(const int x, const int y, field_state* f) {
    _gs->board[(y * _gs->board_x) + x] = *f;
}

void fortify_field(const int x, const int y) {
    _gs->board[(y * _gs->board_x) + x].fortified = 1;
}

void destroy_field(const int x, const int y) {
    _gs->board[(y * _gs->board_x) + x].destroyed = 1;
}

void build_field(const int x, const int y) {
    _gs->board[(y * _gs->board_x) + x].destroyed = 0;
    _gs->board[(y * _gs->board_x) + x].trenched = 1;
}

void set_overlay(const int x, const int y, const char* visual) {
    _gs->overlay[(y * _gs->board_x) + x] = visual;
    //_gs->board[(y * _gs->board_x) + x].visual = visual;
    //_gs->board[(y * _gs->board_x) + x].vset = 1;
}
void unset_overlay() {
    memset(_gs->overlay, 0, sizeof(char*) * (_gs->board_x*_gs->board_y));
}
void unset_overlay_field(const int x, const int y) {
    _gs->overlay[(y * _gs->board_x) + x] = NULL;
    //_gs->board[(y * _gs->board_x) + x].vset = 0;
}

void print_to_feed(const char* msg) {
    int msg_len = strlen(msg);
    memcpy(_gs->feed_buffer+_gs->feed_point, msg, msg_len);
    _gs->feed_point += msg_len;
}

void clear_feed() {
    _gs->feed_point = 0;
}


void kill_player(player_state* ps) {
    set_overlay(ps->x,ps->y,COFFIN);
    //print_board();
    char msg[100];
    sprintf(msg, "Player %s died: %s\n", ps->name, (ps->death_msg) ? ps->death_msg : "Unknown reason");
    print_to_feed(msg);
    //sleep(1000);
    ps->alive = 0;
    ps->death_msg = NULL;
    for(int i = 0; i < _gs->team_count; i++) {
        if (_gs->team_states[i].team_id != ps->team) continue;
        _gs->team_states[i].members_alive--;
        break;
    }
    //unset_visual(ps->x,ps->y);
    //print_board();
    //sleep(250);
}

void death_mark_player(player_state* ps, const char* reason) {
    ps->death_msg = reason;
}


void explode_field(const int x, const int y) {
    field_state* fld = get_field(x,y);
    if (fld->mine) fld->mine = 0;
    if (fld->fortified) fld->fortified = 0;
    else {
        if (fld->trenched) fld->trenched = 0;
        fld->destroyed = 1;
        for(int p = 0; p < _gs->player_count; p++) {
            if (_gs->players[p].x == x && _gs->players[p].y == y) {
                death_mark_player(_gs->players+p, "Got blown up");
            }
        }
    }
}

void bomb_field(const int x, const int y) {
    set_overlay(x,y,EXPLOSION);
    //print_board();
    //sleep(500);
    explode_field(x,y);
    //unset_visual(x,y);
    //print_board();
    //sleep(250);
}

void add_bomb(const int x, const int y, const player_state* ps) {
    bomb_chain* link = malloc(sizeof(bomb_chain));
    link->player = ps->name;
    link->next = _gs->bomb_chain;
    link->x = x;
    link->y = y;
    _gs->bomb_chain = link;
}

void update_bomb_chain(const player_state* ps) {
    bomb_chain* bc = _gs->bomb_chain;
    bomb_chain** prev = &_gs->bomb_chain;
    
    while(bc != NULL) {
        if(strcmp(bc->player, ps->name) == 0) {
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

void move_coord(int x, int y, direction d, int* _x, int* _y) {
    switch(d) {
        case NORTH: *_x = x; *_y = y-1; break;
        case EAST: *_x = x+1; *_y = y; break;
        case SOUTH: *_x = x; *_y = y+1; break;
        case WEST: *_x = x-1; *_y = y; break;
        case HERE: *_x = x; *_y = y; break;
    }
}