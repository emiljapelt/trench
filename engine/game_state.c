#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stddef.h>

#include "game_rules.h"
#include "game_state.h"
#include "player.h"
#include "util.h"
#include "visual.h"
#include "log.h"

game_state* _gs = NULL; 
game_rules* _gr = NULL;


void set_color_overlay(field_state* field, color_target ct, color_predef c) {
    switch (ct) {
        case FORE: 
            field->foreground_color = c; 
            field->overlays |= FOREGROUND_COLOR_OVERLAY;
            break;
        case BACK: 
            field->background_color = c; 
            field->overlays |= BACKGROUND_COLOR_OVERLAY;
            break;
    }
}

void set_mod_overlay(field_state* field, print_mod m) {
    field->mod = m;
    field->overlays |= MOD_OVERLAY;
}

void set_overlay(field_state* field, symbol symbol) {
    field->symbol = symbol;
    field->overlays |= SYMBOL_OVERLAY;
}


void print_to_feed(const char* msg) {
    int msg_len = strlen(msg);
    memcpy(_gs->feed_buffer+_gs->feed_point, msg, msg_len);
    _gs->feed_point += msg_len;
}

void clear_feed() {
    _gs->feed_point = 0;
}

void set_player_steps_and_actions(player_state* ps) {
    ps->remaining_steps = _gr->steps;
    ps->remaining_actions = _gr->actions;
}

void kill_player(player_state* ps) {
    field_state* field = location_field(ps->location);
    field->symbol = COFFIN;
    field->foreground_color = WHITE;
    

    switch (ps->location.type) {
        case VEHICLE_LOCATION: 
            remove_entity(ps->location.vehicle->entities, ps->id);
            break;
        case FIELD_LOCATION:
            remove_entity(field->entities, ps->id);
            break;
    }

    char msg[100];
    sprintf(msg, "%s (#%i) died: %s\n", ps->name, ps->id, (ps->death_msg) ? ps->death_msg : "Unknown reason");
    print_to_feed(msg);
    _log(INFO, msg);
    ps->alive = 0;
    ps->death_msg = NULL;
    if (ps->team)
        ps->team->members_alive--;

    // drop resources
    add_resource(&field->resources, R_Wood, ps->resources.resource[R_Wood].amount);
    add_resource(&field->resources, R_Clay, ps->resources.resource[R_Clay].amount);
    add_resource(&field->resources, R_Ammo, ps->resources.resource[R_Ammo].amount);
    add_resource(&field->resources, R_Sapling, ps->resources.resource[R_Sapling].amount);
    add_resource(&field->resources, R_BearTrap, ps->resources.resource[R_BearTrap].amount);
    add_resource(&field->resources, R_Explosive, ps->resources.resource[R_Explosive].amount);
    add_resource(&field->resources, R_Metal, ps->resources.resource[R_Metal].amount);

    zero_out_registry(&ps->resources);

    // The player struct is freed and removed elsewere.
}

void death_mark_player(player_state* ps, const char* reason) {
    ps->death_msg = reason;
}

void move_coord(int* x, int* y, direction dir, unsigned int dist) {
    if (dir == HERE) return;
    switch (dir) {
        case NORTH: *y -= dist; break;
        case EAST: *x += dist; break;
        case SOUTH: *y += dist; break;
        case WEST: *x -= dist; break;
    }
}

void move_player_to_location(player_state* player, location loc) {
    switch (player->location.type) {
        case VOID_LOCATION: break;
        case FIELD_LOCATION: { 
            field_state* field = player->location.field;
            update_events(entity.of_player(player), field->exit_events, (situation){ .type = MOVEMENT_SITUATION, .movement.loc = loc });
            free(remove_entity(field->entities, player->id));
            break;
        }
        case VEHICLE_LOCATION: {
            vehicle_state* vehicle = player->location.vehicle;
            free(remove_entity(vehicle->entities, player->id));
            break;
        }
    }

    if (player->death_msg) return;

    
    location prev_loc = player->location;

    player->location = loc;
    switch (loc.type) {
        case VOID_LOCATION: break;
        case FIELD_LOCATION: {
            field_state* field = location_field(loc);
            add_entity(field->entities, entity.of_player(player));
            update_events(entity.of_player(player), field->enter_events, (situation){ .type = MOVEMENT_SITUATION, .movement.loc = prev_loc });
            break;
        }
        case VEHICLE_LOCATION: {
            vehicle_state* vehicle = loc.vehicle;
            add_entity(vehicle->entities, entity.of_player(player));
            break;
        }
    }
}

void move_vehicle_to_location(vehicle_state* vehicle, location loc) {
    switch (vehicle->location.type) {
        case VOID_LOCATION: break;
        case FIELD_LOCATION: { 
            field_state* field = vehicle->location.field;
            update_events(entity.of_vehicle(vehicle), field->exit_events, (situation){ .type = MOVEMENT_SITUATION, .movement.loc = loc });
            free(remove_entity(field->entities, vehicle->id));
            break;
        }
        case VEHICLE_LOCATION: {
            // Do something
            break;
        }
    }

    if (vehicle->destroy) return;

    location prev_loc = vehicle->location;

    vehicle->location = loc;
    switch (loc.type) {
        case VOID_LOCATION: break;
        case FIELD_LOCATION: {
            field_state* field = location_field(loc);
            add_entity(field->entities, entity.of_vehicle(vehicle));
            update_events(entity.of_vehicle(vehicle), field->enter_events, (situation){ .type = MOVEMENT_SITUATION, .movement.loc = prev_loc });
            break;
        }
        case VEHICLE_LOCATION: {
            // Do something
            break;
        }
    }
}


void move_entity_to_location(entity_t* e, location loc) {
    location curr = entity.get_location(e);
    switch (curr.type) {
        case VOID_LOCATION: break;
        case FIELD_LOCATION: { 
            field_state* field = curr.field;
            update_events(e, field->exit_events, (situation){ .type = MOVEMENT_SITUATION, .movement.loc = loc });
            free(remove_entity(field->entities, entity.get_id(e)));
            break;
        }
        case VEHICLE_LOCATION: {
            // Do something
            break;
        }
    }

    if ((e->type == ENTITY_PLAYER && e->player->death_msg) || (e->type == ENTITY_VEHICLE && e->vehicle->destroy))
        return;

    location prev_loc = entity.get_location(e);
    
    entity.set_location(e, loc);
    switch (loc.type) {
        case VOID_LOCATION: break;
        case FIELD_LOCATION: {
            field_state* field = location_field(loc);
            add_entity(field->entities, e);
            update_events(e, field->enter_events, (situation){ .type = MOVEMENT_SITUATION, .movement.loc = prev_loc });
            break;
        }
        case VEHICLE_LOCATION: {
            // Do something
            break;
        }
    }
}
