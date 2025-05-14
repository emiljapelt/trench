#ifndef PLAYER_H
#define PLAYER_H

#include "resource_registry.h"
#include "array_list.h"

typedef array_list_t event_list_t;
typedef struct team_state team_state;

typedef struct player_init {
    int* directive;
    int x;
    int y;
} player_init;

typedef struct player_state {
    unsigned char alive: 1;
    const char* death_msg;
    int id;
    team_state* team;
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
    int remaining_steps;
    int remaining_actions;
    int pager_channel;
    array_list_t* pager_msgs;
    event_list_t* pre_death_events;
    event_list_t* post_death_events;
    resource_registry* resources;
} player_state;

player_state* copy_player_state(const player_state*);

#endif