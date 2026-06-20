#ifndef GAME_STATE_H
#define GAME_STATE_H

#include "direction.h"
#include "player.h"
#include "game_rules.h"
#include "event_list.h"
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
    int latest_print;
    int map_width;
    int map_height;
    int id_counter;
    entity_list_t* entities;
    field_state* map;
    char* feed;
    unsigned int feed_change : 1;
    int team_count;
    event_list_t* events;
    team_state* team_states;
} game_state;

extern game_state* _gs;
extern game_rules* _gr;

void kill_player(player_state* ps, const char* reason);
void set_player_steps_and_actions(player_state* ps);

void set_color_overlay(field_state* field, color_target ct, color_predef c);
void set_mod_overlay(field_state* field, print_mod m);
void set_overlay(field_state* field, symbol symbol);
void unset_overlay_field(const int x, const int y);
void unset_overlay();

void print_to_feed(const char* msg);
void clear_feed();

static inline char in_bounds(const int x, const int y) {
    return ((0 <= x) && (x < _gs->map_width) && (0 <= y) && (y < _gs->map_height));
}

void move_coord(int* x, int* y, direction dir, unsigned int dist);

void move_entity_to_location(entity_t*, location);


#endif