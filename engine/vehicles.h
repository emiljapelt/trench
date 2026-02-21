#ifndef VEHICLES_H
#define VEHICLES_H

#include "direction.h"
#include "location.h"
#include "entity_list.h"
#include "player.h"

typedef enum {
    VEHICLE_BOAT = 0,
} vehicle_type;

typedef struct vehicle_state {
    vehicle_type type : 8;
    unsigned int destroy : 1;
    entity_t* entity;
    entity_list_t* entities;
    resource_registry resources;
} vehicle_state;

typedef int (*vehicle_move_function)(vehicle_state* v, player_state* player, direction d);

vehicle_move_function get_vehicle_move_func(vehicle_type type);

int get_vehicle_capacity(vehicle_type type);

void destroy_vehicle(vehicle_state* v, char* death_msg);

#endif