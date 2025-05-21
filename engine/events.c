#include "events.h"

#include <stdlib.h>

#include "game_state.h"
#include "visual.h"
#include "color.h"
#include "player.h"

int mine(player_state* ps, void* data) {
    for(int i = 0; i < _gs->players->count; i++) {
        player_state* player = get_player(_gs->players, i);
        if (player->x == ps->x && player->y == ps->y) 
            death_mark_player(player, "Got blown up by a mine");
    }
    set_overlay(ps->x,ps->y,EXPLOSION);
    set_color_overlay(ps->x, ps->y, FORE, color_predefs.red);
    set_mod_overlay(ps->x, ps->y, BOLD);
    return 1;
}

int bomb(player_state* ps, void* data) {
    bomb_event_args* args = (bomb_event_args*)data;
    if (args->player_id != ps->id) return 0;
    set_overlay(args->x,args->y,EXPLOSION);
    set_color_overlay(args->x,args->y,FORE,color_predefs.red);
    fields.destroy_field(args->x,args->y, "Got blown up");
    // bomb_field(args->x,args->y);
    return 1;
}

int projection_death(player_state* ps, void* data) {
    countdown_args* args = (countdown_args*)data;
    if (!ps->alive || !args->player_id == ps->id) return 0;
    if (args->remaining <= 0) {
        ps->death_msg = "Projection faded";
        return 1;
    }
    else {
        args->remaining--;
        return 0;
    }
}

int ice_block_melt(player_state* player, void* data) {
    ice_block_melt_event_args* args = (ice_block_melt_event_args*)data;
    field_state* field = get_field(args->x, args->y);

    if (field->type != ICE_BLOCK) return 1;
    if (args->player_id != player->id) 
        return 0;
    if (args->remaining) {
        args->remaining--;
        return 0;
    }

    fields.remove_field(args->x, args->y);
    return 1;
}

int mana_drain(player_state* player, void* data) {
    spend_resource(player->resources, "mana", peek_resource(player->resources, "mana"));
    return 1;
}

int tree_grow(player_state* player, void* data) {
    field_countdown_args* args = (field_countdown_args*)data;
    if (player->id != args->player_id) return 0;

    args->remaining--;
    if (args->remaining != 0) return 0;

    if (get_field(args->x, args->y)->type == EMPTY)
        fields.build.tree(args->x, args->y);
        
    return 1;
}

int ocean_drowning(player_state* player, void* data) {
    death_mark_player(player, "Drowned");
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