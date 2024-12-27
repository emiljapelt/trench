#ifndef GAME_STATE_H
#define GAME_STATE_H

#include "player.h"
#include "game_rules.h"
#include "events.h"
#include "resource_registry.h"
#include "visual.h"

typedef enum direction {
    NORTH,
    EAST,
    SOUTH,
    WEST,
    HERE
} direction;

typedef struct directive_info {
    int regs;
    int dir_len;
    int* stack;
    char* directive;
} directive_info;

typedef enum field_state_flags {
    PLAYER_FLAG = 0b0001,
    TRENCH_FLAG = 0b0010,
    MINE_FLAG = 0b0100,
    DESTORYED_FLAG = 0b1000
} field_state_flags;

typedef enum {
    EMPTY,
    TRENCH,
} field_type;

typedef struct {
    unsigned int fortified : 1;
} trench_field;

typedef union {
    trench_field trench;
} field_data;

typedef struct {
    const color* foreground_color_overlay;
    const color* background_color_overlay;
    print_mod mod_overlay;
    const char* symbol_overlay;
    field_type type;
    field_data* data;
    event_list* enter_events;
    event_list* exit_events;
} field_state;

typedef struct team_state {
    int team_id;
    int members_alive;
} team_state;

typedef struct game_state {
    int round;
    int remaining_steps;
    int remaining_actions;
    int board_x;
    int board_y;
    player_state* players;
    int player_count;
    field_state* board;
    int feed_point;
    char* feed_buffer;
    int* global_arrays;
    int team_count;
    event_list* events;
    team_state* team_states;
    resource_registry* resource_registry; 
} game_state;

extern game_state* _gs;
extern game_rules* _gr;

field_state* get_field(const int x, const int y);
void set_field(const int x, const int y, field_state* f);
void fortify_field(const int x, const int y);
void destroy_field(const int x, const int y);
void build_field(const int x, const int y);
void explode_field(const int x, const int y);
void bomb_field(const int x, const int y);
void unexplode_field(const int x, const int y);

void death_mark_player(player_state* ps, const char* reason);
void kill_player(player_state* ps);

void set_color_overlay(const int x, const int y, color_target ct, const color* c);
void set_mod_overlay(const int x, const int y, const print_mod m);
void set_overlay(const int x, const int y, const char* visual);
void unset_overlay_field(const int x, const int y);
void unset_overlay();

void print_to_feed(const char* msg);
void clear_feed();

static inline char in_bounds(const int x, const int y) {
    return (0 <= x && x < _gs->board_x && 0 <= y && y < _gs->board_y);
}

void move_coord(int x, int y, direction d, int* _x, int* _y);

#endif