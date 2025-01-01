#ifndef PLAYER_H
#define PLAYER_H

//#include "event_list.h"

typedef struct event_list event_list;

typedef struct player_init {
    char* directive;
    int x;
    int y;
} player_init;

typedef struct player_state {
    unsigned char alive: 1;
    const char* death_msg;
    int id;
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
    event_list* death_events;
} player_state;

#endif