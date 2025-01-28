#ifndef PLAYER_H
#define PLAYER_H

//#include "event_list.h"
#include "resource_registry.h"

typedef struct linked_list linked_list;

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
    int sp;
    char* path;
    int* directive;
    int directive_len;
    int dp;
    int x;
    int y;
    linked_list* death_events;
    resource_registry* resources;
} player_state;

#endif