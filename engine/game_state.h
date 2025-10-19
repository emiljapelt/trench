#ifndef GAME_STATE_H
#define GAME_STATE_H

#include "direction.h"
#include "player.h"
#include "game_rules.h"
#include "event_list.h"
#include "player_list.h"
#include "resource_registry.h"
#include "visual.h"
#include "fields.h"

typedef struct directive_info {
    int dir_len;
    int* stack;
    int* directive;
} directive_info;

typedef struct team_state {
    char* team_name;
    color* color;
    int members_alive;
} team_state;

typedef struct game_state {
    int round;
    int board_x;
    int board_y;
    int id_counter;
    player_list_t* players;
    field_state* board;
    int feed_point;
    char* feed_buffer;
    int team_count;
    event_list_t* events;
    team_state* team_states;
} game_state;

extern game_state* _gs;
extern game_rules* _gr;

void death_mark_player(player_state* ps, const char* reason);
void kill_player(player_state* ps);
void set_player_steps_and_actions(player_state* ps);

void set_color_overlay(const int x, const int y, color_target ct, color* c);
void set_mod_overlay(const int x, const int y, print_mod m);
void set_overlay(const int x, const int y, char* visual);
void unset_overlay_field(const int x, const int y);
void unset_overlay();

void print_to_feed(const char* msg);
void clear_feed();

static inline char in_bounds(const int x, const int y) {
    return (0 <= x && x < _gs->board_x && 0 <= y && y < _gs->board_y);
}

void move_coord(int* x, int* y, direction dir, unsigned int dist);

void move_player_to_location(player_state*, location);
void move_vehicle_to_location(vehicle_state*, location);
void move_entity_to_location(entity_t*, location);


#endif