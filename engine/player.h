#ifndef PLAYER_H
#define PLAYER_H

#include "resource_registry.h"

typedef struct event_list event_list;

typedef struct player_init {
    int* directive;
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
    int stack_len;
    int sp;
    char* path;
    int* directive;
    int directive_len;
    int dp;
    int x;
    int y;
    event_list* pre_death_events;
    event_list* post_death_events;
    resource_registry* resources;
} player_state;

#endif