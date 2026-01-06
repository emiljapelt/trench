#include "vehicles.h"

#include "game_state.h"
#include "game_rules.h"
#include "builtins.h"
#include "util.h"

#include <stdlib.h>
#include <stdio.h>

int boat_move(vehicle_state* v, direction d) {
    int x, y;
    location_coords(v->location, &x, &y);

    if (fields.properties(x,y) & PROP_OBSTRUCTION)
        return INSTR_OBSTRUCTED;

    move_coord(&x, &y, d, 1);
    if (!in_bounds(x,y))
        return INSTR_OUT_OF_BOUNDS;

    field_state* target = fields.get(x,y);
    if (target->type == OCEAN || target->type == BRIDGE) {

        move_vehicle_to_location(v, field_location_from_field(target));
        if (v->destroy)
            destroy_vehicle(v, "The boat sank");

        return INSTR_SUCCESS;
    }
    else 
        return INSTR_OBSTRUCTED;
}

vehicle_move_function get_vehicle_move_func(vehicle_type type) {
    switch (type) {
        case VEHICLE_BOAT:
            return &boat_move;
    }
}

int get_vehicle_capacity(vehicle_type type) {
    switch (type) {
        case VEHICLE_BOAT:
            return _gr->settings.boat.capacity;
    }
}

void destroy_vehicle(vehicle_state* v, char* death_msg) {
    entity_list_t* entities = v->entities;
    location loc = v->location;

    for(int i = 0; i < entities->count; i++) {
        move_entity_to_location(get_entity(entities, i), loc);
    }

    switch (v->type) {
        case VEHICLE_BOAT: {
            move_vehicle_to_location(v, (location){ .type = VOID_LOCATION });
            free(v);
        }
    }

    array_list.free(entities);
}