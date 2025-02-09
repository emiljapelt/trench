#ifndef EVENTS_H
#define EVENTS_H

#include "player.h"

typedef struct bomb_event_args {
    int x;
    int y;
    int player_id;
} bomb_event_args;

int bomb_event(player_state* ps, void* data);

int mine_event(player_state* ps, void* data);

typedef struct projection_death_args {
    int player_id;
    int remaining;
} projection_death_args;

int projection_death_event(player_state* ps, void* data);

typedef struct ice_block_melt_event_args {
    int x;
    int y;
    int player_id;
    int remaining;
} ice_block_melt_event_args;

int ice_block_melt_event(player_state* ps, void* data);

#endif