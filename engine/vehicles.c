#include "vehicles.h"

#include "game_state.h"
#include "game_rules.h"
#include "builtins.h"
#include "util.h"

#include <stdlib.h>
#include <stdio.h>

int boat_move(vehicle_state* v, player_state* player, direction d) {
    int x, y;
    location_coords(v->entity->location, &x, &y);

    if (fields.properties(x,y, player) & PROP_OBSTRUCTION)
        return INSTR_OBSTRUCTED;

    move_coord(&x, &y, d, 1);
    if (!in_bounds(x,y))
        return INSTR_OUT_OF_BOUNDS;

    field_state* target = fields.get(x,y);
    if (target->type == OCEAN || target->type == BRIDGE) {

        move_entity_to_location(v->entity, field_location_from_field(target));
        if (!v->entity->active)
            destroy_vehicle(v);

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

void destroy_vehicle(vehicle_state* v) {
    entity_list_t* entities = v->entities;
    location loc = v->entity->location;

    for(int i = 0; i < entities->count; i++)
        move_entity_to_location(get_entity(entities, i), loc);

    move_entity_to_location(v->entity, VOID);

    v->entity->active = 0;
}