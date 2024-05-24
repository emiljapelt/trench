#ifndef GAME_STATE_H
#define GAME_STATE_H

#include "loader.h"
#include "player.h"
#include "game_rules.h"

typedef enum direction {
    NORTH,
    EAST,
    SOUTH,
    WEST,
    HERE
} direction;

typedef enum bullet_states {
    No_bullets = 0,
    NS_bullets,
    EW_bullets,
} bullet_states;

typedef struct bomb_chain {
    unsigned char player_id: 4;
    int x;
    int y;
    struct bomb_chain* next;
} bomb_chain;

typedef struct field_state {
    unsigned char destroyed : 1;
    unsigned char fortified : 1;
    unsigned char trenched : 1;
    bullet_states bullet_state : 2;
    unsigned char target : 1;
    unsigned char explosion : 1;
} field_state;

typedef struct game_state {
    int round;
    int remaining_steps;
    int remaining_actions;
    const int board_x;
    const int board_y;
    player_state* players;
    const int player_count;
    field_state* board;
    bomb_chain* bomb_chain;
} game_state;

void create_players(parsed_player_file** inits, game_state* gs, game_rules* gr);

field_state* get_field(int x, int y, game_state* gs);
void set_field(int x, int y, game_state* gs, field_state* f);
void fortify_field(int x, int y, game_state* gs);
void destroy_field(int x, int y, game_state gs);
void build_field(int x, int y, game_state* gs);
void shoot_field(int x, int y, const direction d, game_state* gs);
void unshoot_field(int x, int y, game_state* gs);
void explode_field(int x, int y, game_state* gs);
void bomb_field(int x, int y, game_state* gs);
void unexplode_field(int x, int y, game_state* gs);
void target_field(int x, int y, game_state* gs);
void untarget_field(int x, int y, game_state* gs);

void add_bomb(int x, int y, player_state* ps, game_state* gs);
void update_bomb_chain(player_state* ps, game_state* gs);

static inline char in_bounds(int x, int y, game_state* gs) {
    return (0 <= x && x < gs->board_x && 0 <= y && y < gs->board_y);
}

#endif