#include "events.h"

#include <stdlib.h>
#include <stdio.h>

#include "vehicles.h"
#include "game_state.h"
#include "visual.h"
#include "color.h"
#include "player.h"
#include "util.h"

int mine(entity_t* entity, void* data) {
    field_args* args = (field_args*)data;

    if (get_field(args->x, args->y)->entities->count > 0) return 0;

    switch (entity->type) {
        case ENTITY_PLAYER: {
            death_mark_player(entity->player, "Blown up by a mine");

            location_field(entity->player->location)->symbol_overlay = EXPLOSION;
            location_field(entity->player->location)->foreground_color_overlay = color_predefs.red;
            location_field(entity->player->location)->mod_overlay = BOLD;
            return 1;
        }
        
        case ENTITY_VEHICLE: {
            entity->vehicle->destroy = 1;

            location_field(entity->vehicle->location)->symbol_overlay = EXPLOSION;
            location_field(entity->vehicle->location)->foreground_color_overlay = color_predefs.red;
            location_field(entity->vehicle->location)->mod_overlay = BOLD;
            return 1;
        }
    }

    return 0;
}

int bomb(entity_t* entity, void* data) {

    switch (entity->type) {
        case ENTITY_PLAYER: {
            bomb_event_args* args = (bomb_event_args*)data;
            if (args->player_id != entity->player->id) return 0;
            set_overlay(args->x,args->y,EXPLOSION);
            set_color_overlay(args->x,args->y,FORE,color_predefs.red);
            fields.destroy_field(args->x,args->y, "Got blown up");
            return 1;
        }
    }

    return 0;
}

int projection_death(entity_t* entity, void* data) {

    switch (entity->type) {
        case ENTITY_PLAYER: {
            countdown_args* args = (countdown_args*)data;
            if (!entity->player->alive || args->player_id != entity->player->id) return 0;
            if (args->remaining <= 0) {
                entity->player->death_msg = "Projection faded";
                return 1;
            }
            else {
                args->remaining--;
                return 0;
            }
        }
    }
    
    return 0;
}

int ice_block_melt(entity_t* entity, void* data) {

    switch (entity->type) {
        case ENTITY_PLAYER: {
            ice_block_melt_event_args* args = (ice_block_melt_event_args*)data;
            field_state* field = get_field(args->x, args->y);

            if (field->type != ICE_BLOCK) return 1;
            if (args->player_id != entity->player->id) 
                return 0;
            if (args->remaining) {
                args->remaining--;
                return 0;
            }

            fields.remove_field(args->x, args->y);
            return 1;
        }
    }
    
    return 0;
}

int mana_drain(entity_t* entity, void* data) {

    switch (entity->type) {
        case ENTITY_PLAYER: {
            spend_resource(entity->player->resources, "mana", peek_resource(entity->player->resources, "mana"));
            return 1;
        }
    }
    
    return 0;
}

int tree_grow(entity_t* entity, void* data) {

    switch (entity->type) {
        case ENTITY_PLAYER: {
            field_countdown_args* args = (field_countdown_args*)data;
            if (entity->player->id != args->player_id) return 0;

            args->remaining--;
            if (args->remaining != 0) return 0;

            if (get_field(args->x, args->y)->type == EMPTY)
                fields.build.tree(args->x, args->y);
                
            return 1;
        }
    }
    
    return 0;
}

int ocean_drowning(entity_t* entity, void* data) {

    switch (entity->type) {
        case ENTITY_PLAYER: {
            if (location_field(entity->player->location)->type != OCEAN) return 0;
            death_mark_player(entity->player, "Drowned");
            return 0;
        }
    }

    return 0;
}

int bear_trap_escape(entity_t* entity, void* data) {
    if (entity->type == ENTITY_PLAYER) {
        player_event_args* args = (player_event_args*)data;
        if (args->player_id != entity->player->id) return 0;
        entity->player->remaining_actions = 0;
        entity->player->remaining_steps = 0;
        return 1;
    }
    return 1;
}

int bear_trap_trigger(entity_t* entity, void* data) {
    if (entity->type == ENTITY_PLAYER) {
        entity->player->remaining_actions = 0;
        entity->player->remaining_steps = 0;

        player_event_args* args = malloc(sizeof(player_event_args));
        args->player_id = entity->player->id;
        add_event(
            _gs->events,
            PHYSICAL_EVENT,
            &bear_trap_escape,
            args
        );
    }
    return 1;
}

const events_namespace events = {
    .bomb = &bomb,
    .mine = &mine,
    .ice_block_melt = &ice_block_melt,
    .projection_death = &projection_death,
    .mana_drain = &mana_drain,
    .tree_grow = &tree_grow,
    .ocean_drowning = &ocean_drowning,
    .bear_trap_trigger = &bear_trap_trigger,
    .bear_trap_escape = &bear_trap_escape,
};