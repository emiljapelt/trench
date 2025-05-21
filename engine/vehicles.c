#include "vehicles.h"

#include "game_state.h"
#include "game_rules.h"
#include "util.h"
#include <stdlib.h>

#include <stdio.h>

int boat_move(vehicle_state* v, direction d) {
    int x = v->x;
    int y = v->y;
    move_coord(&x, &y, d, 1);
    field_state* field = get_field(x,y);
    if (field->type == OCEAN && field->vehicle == NULL) {
        get_field(v->x,v->y)->vehicle = NULL;
        field->vehicle = v;
        v->x = x;
        v->y = y;
        return 1;
    }
    return 0;
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