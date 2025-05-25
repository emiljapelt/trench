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
        update_events((entity) { ENTITY_VEHICLE, .vehicle = v }, get_field(v->x, v->y)->exit_events);
        if (!v->destroy) {
            get_field(v->x,v->y)->vehicle = NULL;
            field->vehicle = v;
            v->x = x;
            v->y = y;
            update_events((entity) { ENTITY_VEHICLE, .vehicle = v }, get_field(v->x, v->y)->enter_events);
        }
        else
            destroy_vehicle(v, "Blown up by a mine");

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

void destroy_vehicle(vehicle_state* v, char* death_msg) {
    player_list_t* players = v->players;

    switch (v->type) {
        case VEHICLE_BOAT: {
            get_field(v->x, v->y)->vehicle = NULL;
            free(v);
        }
    }

    for(int i = 0; i < players->count; i++) {    
        player_state* player = get_player(players, i);
        death_mark_player(player, death_msg);
        player->vehicle = NULL;
    }

    array_list.free(players);
}