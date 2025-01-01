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


#endif