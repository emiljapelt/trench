#ifndef PLAYER_H
#define PLAYER_H

typedef struct player_init {
    char* directive;
    int x;
    int y;
} player_init;

typedef struct player_state {
    unsigned char alive: 1;
    const char* death_msg;
    int team;
    char* name;
    int* stack;
    int sp;
    char* path;
    char* directive;
    int directive_len;
    int dp;
    int x;
    int y;
    int bombs;
    int shots;
} player_state;

#endif