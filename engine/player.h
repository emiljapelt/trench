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
    int step;
} player_state;

int player_x(player_state* ps);
void mod_player_x(player_state* ps, int v);
void set_player_x(player_state* ps, int v);
int player_y(player_state* ps);
void mod_player_y(player_state* ps, int v);
void set_player_y(player_state* ps, int v);
int player_b(player_state* ps);
void mod_player_b(player_state* ps, int v);
void kill_player(player_state* ps);

#endif