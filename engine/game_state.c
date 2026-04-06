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
    char buffer[msg_len]; 
    int feed_len = msg_len + (FEED_WIDTH - (msg_len % FEED_WIDTH));
    int feed_lines = feed_len / FEED_WIDTH;
    
    for(int i = 0; i < msg_len; i++) {
        switch(msg[i]) {
            case '\n':
            case '\r':
            case '\t':
                buffer[i] = ' ';
                break;   
            default:
                buffer[i] = msg[i];
        }
    }


    memmove(_gs->feed + feed_len, _gs->feed, _gr->viewport.height * FEED_WIDTH - feed_len);
    memset(_gs->feed, ' ', feed_len);
    memcpy(_gs->feed, buffer, msg_len);

    _gs->feed_change = 1;
}

void clear_feed() {
    memset(_gs->feed, ' ', _gr->viewport.height * FEED_WIDTH);
}

void set_player_steps_and_actions(player_state* ps) {
    ps->remaining_steps = _gr->steps;
    ps->remaining_actions = _gr->actions;
}

void kill_player(player_state* ps, const char* reason) {
    ps->entity->active = 0;
    update_events(ps->entity, ps->pre_death_events, (situation){ .type = NO_SITUATION});
    if (ps->entity->active) return;
    update_events(ps->entity, ps->post_death_events, (situation){ .type = NO_SITUATION});

    field_state* field = location_field(ps->entity->location);
    if (field != NULL) {
        field->symbol = COFFIN;
        field->foreground_color = WHITE;
        
        // drop resources
        add_resource(&field->resources, R_Wood, ps->resources.resource[R_Wood].amount);
        add_resource(&field->resources, R_Clay, ps->resources.resource[R_Clay].amount);
        add_resource(&field->resources, R_Ammo, ps->resources.resource[R_Ammo].amount);
        add_resource(&field->resources, R_Sapling, ps->resources.resource[R_Sapling].amount);
        add_resource(&field->resources, R_BearTrap, ps->resources.resource[R_BearTrap].amount);
        add_resource(&field->resources, R_Explosive, ps->resources.resource[R_Explosive].amount);
        add_resource(&field->resources, R_Metal, ps->resources.resource[R_Metal].amount);
    }

    zero_out_registry(&ps->resources);

    move_entity_to_location(ps->entity, VOID);

    char msg[100];
    sprintf(msg, "%s (#%i) died: %s", ps->name, ps->entity->id, reason ? reason : "Unknown reason");
    print_to_feed(msg);
    _log(INFO, msg);
    if (ps->team)
        ps->team->members_alive--;
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

void move_entity_to_location(entity_t* e, location loc) {
    location curr = e->location;
    switch (curr.type) {
        case VOID_LOCATION: break;
        case FIELD_LOCATION: { 
            field_state* field = curr.field;
            update_events(e, field->exit_events, (situation){ .type = MOVEMENT_SITUATION, .movement.loc = loc });
            remove_entity(field->entities, e->id);
            break;
        }
        case VEHICLE_LOCATION: {
            vehicle_state* vehicle = curr.vehicle;
            remove_entity(vehicle->entities, e->id);
            break;
        }
    }

    if (!e->active) return;

    location prev_loc = e->location;
    
    e->location = loc;
    
    switch (loc.type) {
        case VOID_LOCATION: break;
        case FIELD_LOCATION: {
            field_state* field = location_field(loc);
            add_entity(field->entities, e);
            update_events(e, field->enter_events, (situation){ .type = MOVEMENT_SITUATION, .movement.loc = prev_loc });
            break;
        }
        case VEHICLE_LOCATION: {
            vehicle_state* vehicle = loc.vehicle;
            add_entity(vehicle->entities, e);
            break;
            break;
        }
    }
}
