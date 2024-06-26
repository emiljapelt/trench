#ifndef GAME_STATE_H
#define GAME_STATE_H

#include "player.h"
#include "game_rules.h"

typedef enum direction {
    NORTH,
    EAST,
    SOUTH,
    WEST,
    HERE
} direction;

typedef struct directive_info {
    int regs;
    int regs_len;
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
    unsigned char mine : 1;
    unsigned char vset : 1;
    const char* visual;
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
    int* global_arrays;
} game_state;

extern game_state* _gs;
extern game_rules* _gr;

directive_info load_directive_to_struct(const char* directive, const int len);
void create_players(char* player_info);

field_state* get_field(int x, int y);
void set_field(int x, int y, field_state* f);
void fortify_field(int x, int y);
void destroy_field(int x, int y);
void build_field(int x, int y);
void explode_field(int x, int y);
void bomb_field(int x, int y);
void unexplode_field(int x, int y);

void set_visual(int x, int y, const char* visual);
void unset_visual(int x, int y);

void add_bomb(int x, int y, player_state* ps);
void update_bomb_chain(player_state* ps);

static inline char in_bounds(int x, int y) {
    return (0 <= x && x < _gs->board_x && 0 <= y && y < _gs->board_y);
}

#endif