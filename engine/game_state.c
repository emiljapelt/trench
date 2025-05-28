#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stddef.h>

#include "game_rules.h"
#include "game_state.h"
#include "player.h"
#include "util.h"
#include "visual.h"

game_state* _gs;
game_rules* _gr;


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
    field_state* field = location_field(ps->location);
    field->symbol_overlay = COFFIN;
    field->foreground_color_overlay = color_predefs.white;
    

    switch (ps->location.type) {
        case VEHICLE_LOCATION: 
            remove_player_id(ps->location.vehicle->players, ps->id);
            break;
        case FIELD_LOCATION:
            remove_player_id(field->players, ps->id);
            break;
    }   

    char msg[100];
    sprintf(msg, "%s (#%i) died: %s\n", ps->name, ps->id, (ps->death_msg) ? ps->death_msg : "Unknown reason");
    print_to_feed(msg);
    ps->alive = 0;
    ps->death_msg = NULL;
    if (ps->team)
        ps->team->members_alive--;
}

void death_mark_player(player_state* ps, const char* reason) {
    ps->death_msg = reason;
}

void move_coord(int* x, int* y, direction dir, unsigned int dist) {
    if (dir == HERE) return;
    switch (dir) {
        case NORTH: *y -= dist; break;
        case EAST: *x += dist; break;
        case SOUTH: *y += dist; break;
        case WEST: *x -= dist; break;
    }
}

void move_player_to_location(player_state* player, location loc) {
    switch (player->location.type) {
        case VOID_LOCATION: break;
        case FIELD_LOCATION: { 
            field_state* field = player->location.field;
            remove_player_id(field->players, player->id);
            update_events((entity){ .type = ENTITY_PLAYER, .player = player}, field->exit_events);
            break;
        }
        case VEHICLE_LOCATION: {
            vehicle_state* vehicle = player->location.vehicle;
            remove_player_id(vehicle->players, player->id);
            break;
        }
    }

    if (player->death_msg) return;

    player->location = loc;
    switch (loc.type) {
        case VOID_LOCATION: break;
        case FIELD_LOCATION: {
            field_state* field = location_field(loc);
            add_player(field->players, player);
            update_events((entity){ .type = ENTITY_PLAYER, .player = player}, field->enter_events);
            break;
        }
        case VEHICLE_LOCATION: {
            vehicle_state* vehicle = loc.vehicle;
            add_player(vehicle->players, player);
            break;
        }
    }
}
