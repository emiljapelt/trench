#ifndef PLAYER_H
#define PLAYER_H

typedef struct player_init {
    char* directive;
    int x;
    int y;
} player_init;

typedef struct player_state {
    unsigned char alive: 1;
    unsigned char id: 4;
    int* stack;
    int sp;
    char* directive;
    int directive_len;
    int dp;
    int x;
    int y;
    int bombs;
    int shots;
} player_state;

// int player_x(player_state* ps);
// void mod_player_x(player_state* ps, int v);
// void set_player_x(player_state* ps, int v);
// int player_y(player_state* ps);
// void mod_player_y(player_state* ps, int v);
// void set_player_y(player_state* ps, int v);
// int player_bombs(player_state* ps);
// int player_shots(player_state* ps);
// void mod_player_bombs(player_state* ps, int v);
// void mod_player_shots(player_state* ps, int v);
void kill_player(player_state* ps);

#endif