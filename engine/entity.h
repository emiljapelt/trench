#ifndef ENTITY_H
#define ENTITY_H

#include "location.h"

typedef enum {
    ENTITY_PLAYER,
    ENTITY_VEHICLE,
} entity_type;

typedef struct entity_t {
    entity_type type: 4;
    union {
        struct player_state* player;
        struct vehicle_state* vehicle;
    };
    
} entity_t;


typedef struct entity_namespace {
    entity_t* (*of_player)(struct player_state*);
    entity_t* (*of_vehicle)(struct vehicle_state*);
    int (*get_id)(struct entity_t*);
    location (*get_location)(struct entity_t*);
    void (*set_location)(struct entity_t*, location);
} entity_namespace;

extern const entity_namespace entity;


#endif