#include "vehicles.h"

#include "game_state.h"
#include "game_rules.h"
#include "util.h"
#include <stdlib.h>

#include <stdio.h>

int boat_move(vehicle_state* v, direction d) {
    int x, y;
    location_coords(v->location, &x, &y);
    move_coord(&x, &y, d, 1);

    field_state* target = get_field(x,y);
    if (target->type == OCEAN && target->vehicle == NULL) {
        update_events((entity) { ENTITY_VEHICLE, .vehicle = v }, location_field(v->location)->exit_events);
        if (!v->destroy) {
            location_field(v->location)->vehicle = NULL;
            target->vehicle = v;
            v->location = field_location_from_field(target);
            update_events((entity) { ENTITY_VEHICLE, .vehicle = v }, target->enter_events);
        }
        else
            destroy_vehicle(v, "AAAA boat gone!");

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
    location loc = v->location;

    switch (v->type) {
        case VEHICLE_BOAT: {
            location_field(loc)->vehicle = NULL;
            free(v);
        }
    }

    for(int i = 0; i < players->count; i++) {    
        player_state* player = get_player(players, i);
        death_mark_player(player, death_msg);
        player->location = loc;
        add_player(location_field(loc)->players, player);
    }

    array_list.free(players);
}