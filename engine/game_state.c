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
    switch (_gs->board[(y * _gs->board_x) + x].type) {
        case TRENCH: {
            _gs->board[(y * _gs->board_x) + x].data->trench.fortified = 1;
            break;
        }
    }
}

void destroy_field(const int x, const int y) {
    _gs->board[(y * _gs->board_x) + x].type = EMPTY;
    free(_gs->board[(y * _gs->board_x) + x].data);
}

void build_field(const int x, const int y) {
    trench_field* field = malloc(sizeof(trench_field));
    field->fortified = 0;

    _gs->board[(y * _gs->board_x) + x].type = TRENCH;
    _gs->board[(y * _gs->board_x) + x].data = (field_data*)field;
}

void set_color_overlay(const int x, const int y, color_target ct, const color* c) {
    switch (ct) {
        case FORE: _gs->board[(y * _gs->board_x) + x].foreground_color_overlay = c; break;
        case BACK: _gs->board[(y * _gs->board_x) + x].background_color_overlay = c; break;
    }
}

void set_mod_overlay(const int x, const int y, const print_mod m) {
    _gs->board[(y * _gs->board_x) + x].mod_overlay = m;
}

void set_overlay(const int x, const int y, const char* symbol) {
    _gs->board[(y * _gs->board_x) + x].symbol_overlay = symbol;
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
    char msg[100];
    sprintf(msg, "Player %s died: %s\n", ps->name, (ps->death_msg) ? ps->death_msg : "Unknown reason");
    print_to_feed(msg);
    ps->alive = 0;
    ps->death_msg = NULL;
    for(int i = 0; i < _gs->team_count; i++) {
        if (_gs->team_states[i].team_id != ps->team) continue;
        _gs->team_states[i].members_alive--;
        break;
    }
}

void death_mark_player(player_state* ps, const char* reason) {
    ps->death_msg = reason;
}


void explode_field(const int x, const int y) {
    field_state* fld = get_field(x,y);

    switch (fld->type) {
        case TRENCH: {
            if(fld->data->trench.fortified) {
                fld->data->trench.fortified = 0; 
                return;
            }
            fld->type = EMPTY;
            free(fld->data);
        }
    }
    
    for(int p = 0; p < _gs->player_count; p++)
        if (_gs->players[p].x == x && _gs->players[p].y == y) 
            death_mark_player(_gs->players+p, "Got blown up");
}

void bomb_field(const int x, const int y) {
    set_overlay(x,y,EXPLOSION);
    set_color_overlay(x,y,FORE,color_predefs.red);
    explode_field(x,y);
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