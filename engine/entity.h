#ifndef ENTITY_H
#define ENTITY_H

#include "location.h"

typedef enum {
    ENTITY_PLAYER,
    ENTITY_VEHICLE,
} entity_type;

typedef struct entity_t {
    entity_type type : 8;
    int id;
    location location;
    union {
        struct player_state* player;
        struct vehicle_state* vehicle;
    };
} entity_t;

entity_t* entity_of_player(struct player_state* player);

entity_t* entity_of_vehicle(struct vehicle_state* vehicle);

#endif