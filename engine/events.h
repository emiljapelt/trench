#ifndef EVENTS_H
#define EVENTS_H

#include "player.h"

// Return 1 if the event is finished, otherwise return 0.
typedef int (*event_function)(player_state* ps, void* data);

typedef struct bomb_event_args {
    int x;
    int y;
    int player_id;
} bomb_event_args;

typedef struct projection_death_args {
    int player_id;
    int remaining;
} projection_death_args;

typedef struct ice_block_melt_event_args {
    int x;
    int y;
    int player_id;
    int remaining;
} ice_block_melt_event_args;

typedef struct events_namespace {
    event_function bomb;
    event_function mine;
    event_function projection_death;
    event_function ice_block_melt;
} events_namespace;
extern const events_namespace events;

#endif