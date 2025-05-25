#ifndef ENTITY_H
#define ENTITY_H

typedef enum {
    ENTITY_PLAYER,
    ENTITY_VEHICLE,
} entity_type;

typedef struct entity {
    entity_type type: 4;
    union {
        struct player_state* player;
        struct vehicle_state* vehicle;
    };
    
} entity;


#endif