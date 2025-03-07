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

int fortify_field(const int x, const int y) {
    field_state* field = get_field(x,y);
    switch (field->type) {
        case TRENCH: {
            field->data->trench.fortified = 1;
            return 1;
        }
        default: return 0;
    }
}

// TODO: Maybe limit when a trench can be created ???!!!
void build_trench_field(const int x, const int y) {
    field_state* field = get_field(x,y);

    field_data* data = malloc(sizeof(field_data));
    data->trench.fortified = 0;

    field->type = TRENCH;
    field->data = data;
}

void set_color_overlay(const int x, const int y, color_target ct, color* c) {
    switch (ct) {
        case FORE: _gs->board[(y * _gs->board_x) + x].foreground_color_overlay = c; break;
        case BACK: _gs->board[(y * _gs->board_x) + x].background_color_overlay = c; break;
    }
}

void set_mod_overlay(const int x, const int y, print_mod m) {
    _gs->board[(y * _gs->board_x) + x].mod_overlay = m;
}

void set_overlay(const int x, const int y, char* symbol) {
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

void set_player_steps_and_actions(player_state* ps) {
    ps->remaining_steps = _gr->steps;
    ps->remaining_actions = _gr->actions;
}

void kill_player(player_state* ps) {
    set_overlay(ps->x,ps->y,COFFIN);
    set_color_overlay(ps->x,ps->y,FORE,color_predefs.white);
    char msg[100];
    sprintf(msg, "Player %s (%i) died: %s\n", ps->name, ps->id, (ps->death_msg) ? ps->death_msg : "Unknown reason");
    print_to_feed(msg);
    ps->alive = 0;
    ps->death_msg = NULL;
    ps->team->members_alive--;
}

void death_mark_player(player_state* ps, const char* reason) {
    ps->death_msg = reason;
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