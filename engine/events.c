#include "events.h"

#include <stdlib.h>
#include <stdio.h>

#include "vehicles.h"
#include "game_state.h"
#include "visual.h"
#include "color.h"
#include "player.h"
#include "util.h"

int mine(entity_t* entity, void* data, situation situ) {
    field_args* args = (field_args*)data;

    // Check if there are more entities than just 'entity'
    if (fields.get(args->x, args->y)->entities->count > 1) return 0;

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

int bomb(entity_t* entity, void* data, situation situ) {

    switch (entity->type) {
        case ENTITY_PLAYER: {
            bomb_event_args* args = (bomb_event_args*)data;
            if (args->player_id != entity->player->id) return 0;
            set_overlay(args->x,args->y,EXPLOSION);
            set_color_overlay(args->x,args->y,FORE,color_predefs.red);
            fields.destroy_field(fields.get(args->x,args->y), "Got blown up");
            return 1;
        }
    }

    return 0;
}

int projection_upkeep(entity_t* entity, void* data, situation situ) {

    switch (entity->type) {
        case ENTITY_PLAYER: {
            player_event_args* args = (player_event_args*)data;
            if (args->player_id != entity->player->id) return 0;
            if (!entity->player->alive) return 1;

            if (!spend_resource(entity->player->resources, "mana", _gr->settings.projection.upkeep)) {
                entity->player->death_msg = "Projection faded";
                return 1;
            }
        }
    }
    
    return 0;
}

int ice_block_melt(entity_t* entity, void* data, situation situ) {

    switch (entity->type) {
        case ENTITY_PLAYER: {
            ice_block_melt_event_args* args = (ice_block_melt_event_args*)data;
            field_state* field = fields.get(args->x, args->y);

            if (field->type != ICE_BLOCK) return 1;
            if (args->player_id != entity->player->id) 
                return 0;
            if (args->remaining) {
                args->remaining--;
                return 0;
            }

            fields.remove_field(field);
            return 1;
        }
    }
    
    return 0;
}

int mana_drain(entity_t* entity, void* data, situation situ) {

    switch (entity->type) {
        case ENTITY_PLAYER: {
            spend_resource(entity->player->resources, "mana", peek_resource(entity->player->resources, "mana"));
            return 1;
        }
    }
    
    return 0;
}

int tree_grow(entity_t* entity, void* data, situation situ) {

    switch (entity->type) {
        case ENTITY_PLAYER: {
            field_countdown_args* args = (field_countdown_args*)data;
            if (entity->player->id != args->player_id) return 0;

            args->remaining--;
            if (args->remaining != 0) return 0;

            field_state* field = fields.get(args->x, args->y);
            if (field->type == EMPTY)
                fields.build.tree(field);
                
            return 1;
        }
    }
    
    return 0;
}

int ocean_drowning(entity_t* entity, void* data, situation situ) {

    switch (entity->type) {
        case ENTITY_PLAYER: {
            death_mark_player(entity->player, "Drowned");
            return 0;
        }
    }

    return 0;
}

int bear_trap_escape(entity_t* entity, void* data, situation situ) {
    if (entity->type == ENTITY_PLAYER) {
        player_event_args* args = (player_event_args*)data;
        if (args->player_id != entity->player->id) return 0;
        entity->player->remaining_actions = 0;
        entity->player->remaining_steps = 0;
        return 1;
    }
    return 1;
}

int bear_trap_trigger(entity_t* entity, void* data, situation situ) {
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

int clay_spread(entity_t* entity, void* data, situation situ) {
    if (entity->type == ENTITY_PLAYER) {
        field_state* field = location_field(situ.movement.loc);
        field_state* clay_field = location_field(entity->player->location);
        if (clay_field->data->clay_pit.amount >= _gr->settings.clay_pit.spread_limit)
        switch (field->type) {
            case EMPTY:
                int x, y;
                location_coords(situ.movement.loc, &x, &y);
                fields.build.clay_pit(field);
                break;
            case CLAY:
                if (field->data->clay_pit.amount < _gr->settings.clay_pit.contain_limit) {
                    field->data->clay_pit.amount++;
                    clay_field->data->clay_pit.amount--;
                }
                break;
        }
    }

    return 0;
}

const events_namespace events = {
    .bomb = &bomb,
    .mine = &mine,
    .ice_block_melt = &ice_block_melt,
    .projection_upkeep = &projection_upkeep,
    .mana_drain = &mana_drain,
    .tree_grow = &tree_grow,
    .ocean_drowning = &ocean_drowning,
    .bear_trap_trigger = &bear_trap_trigger,
    .bear_trap_escape = &bear_trap_escape,
    .clay_spread = &clay_spread
};