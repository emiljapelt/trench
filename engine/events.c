#include "events.h"

#include <stdlib.h>
#include <stdio.h>

#include "vehicles.h"
#include "game_state.h"
#include "visual.h"
#include "color.h"
#include "player.h"
#include "util.h"

int mine(entity entity, void* data) {

    switch (entity.type) {
        case ENTITY_PLAYER: {
            for(int i = 0; i < _gs->players->count; i++) {
                player_state* player = get_player(_gs->players, i);
                if (player_x(player) == player_x(entity.player) && player_y(player) == player_y(entity.player)) 
                    death_mark_player(player, "Blown up by a mine");
            }
            set_overlay(player_x(entity.player), player_y(entity.player), EXPLOSION);
            set_color_overlay(player_x(entity.player), player_x(entity.player), FORE, color_predefs.red);
            set_mod_overlay(player_x(entity.player), player_y(entity.player), BOLD);
            return 1;
        }
        
        case ENTITY_VEHICLE: {
            entity.vehicle->destroy = 1;

            set_overlay(entity.vehicle->x, entity.vehicle->y, EXPLOSION);
            set_color_overlay(entity.vehicle->x, entity.vehicle->y, FORE, color_predefs.red);
            set_mod_overlay(entity.vehicle->x, entity.vehicle->y, BOLD);
            return 1;
        }
    }

    return 0;
}

int bomb(entity entity, void* data) {

    switch (entity.type) {
        case ENTITY_PLAYER: {
            bomb_event_args* args = (bomb_event_args*)data;
            if (args->player_id != entity.player->id) return 0;
            set_overlay(args->x,args->y,EXPLOSION);
            set_color_overlay(args->x,args->y,FORE,color_predefs.red);
            fields.destroy_field(args->x,args->y, "Got blown up");
            return 1;
        }
    }

    return 0;
}

int projection_death(entity entity, void* data) {

    switch (entity.type) {
        case ENTITY_PLAYER: {
            countdown_args* args = (countdown_args*)data;
            if (!entity.player->alive || !args->player_id == entity.player->id) return 0;
            if (args->remaining <= 0) {
                entity.player->death_msg = "Projection faded";
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

int ice_block_melt(entity entity, void* data) {

    switch (entity.type) {
        case ENTITY_PLAYER: {
            ice_block_melt_event_args* args = (ice_block_melt_event_args*)data;
            field_state* field = get_field(args->x, args->y);

            if (field->type != ICE_BLOCK) return 1;
            if (args->player_id != entity.player->id) 
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

int mana_drain(entity entity, void* data) {

    switch (entity.type) {
        case ENTITY_PLAYER: {
            spend_resource(entity.player->resources, "mana", peek_resource(entity.player->resources, "mana"));
            return 1;
        }
    }
    
    return 0;
}

int tree_grow(entity entity, void* data) {

    switch (entity.type) {
        case ENTITY_PLAYER: {
            field_countdown_args* args = (field_countdown_args*)data;
            if (entity.player->id != args->player_id) return 0;

            args->remaining--;
            if (args->remaining != 0) return 0;

            if (get_field(args->x, args->y)->type == EMPTY)
                fields.build.tree(args->x, args->y);
                
            return 1;
        }
    }
    
    return 0;
}

int ocean_drowning(entity entity, void* data) {

    switch (entity.type) {
        case ENTITY_PLAYER: {
            death_mark_player(entity.player, "Drowned");
            return 0;
        }
    }

    return 0;
}

const events_namespace events = {
    .bomb = &bomb,
    .mine = &mine,
    .ice_block_melt = &ice_block_melt,
    .projection_death = &projection_death,
    .mana_drain = &mana_drain,
    .tree_grow = &tree_grow,
    .ocean_drowning = &ocean_drowning,
};