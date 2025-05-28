#include "fields.h"

#include <stdlib.h>
#include <stdio.h>

#include "game_state.h"
#include "damage.h"

field_state* get_field(const int x, const int y) {
    return _gs->board + ((y * _gs->board_x) + x);
}

void set_field(const int x, const int y, field_state* f) {
    _gs->board[(y * _gs->board_x) + x] = *f;
}

// Something that cannot be passed through
int is_obstruction(const int x, const int y) {
    switch (get_field(x,y)->type) {
        case WALL:
        case TREE:
        case ICE_BLOCK:
            return 1;
        default:
            return 0;
    }
}

// Something which is destroyed by fire
int is_flammable(const int x, const int y) {
    switch (get_field(x,y)->type) {
        case WALL:
        case TREE:
            return 1;
        default:
            return 0;
    }
}

// Something which is removed by heat
int is_meltable(const int x, const int y) {
    switch (get_field(x,y)->type) {
        case ICE_BLOCK:
            return 1;
        default:
            return 0;
    }
}

// Something which protects from falling things. e.g. bombs and rocks
int is_shelter(const int x, const int y) {
    field_state* field = get_field(x,y);
    switch (field->type) {
        case TRENCH:
            return field->data->trench.fortified;
        default:
            return 0;
    }
}

int is_cover(const int x, const int y) {
    switch (get_field(x,y)->type) {
        case TRENCH:
            return 1;
        default:
            return 0;
    }
}

int has_player(const int x, const int y) {
    //unsigned int player_found;
    //for(int i = 0; i < _gs->players->count; i++) {
    //    player_state* player = get_player(_gs->players, i);
    //    if (player->alive && player_x(player) == x && player_y(player) == y)
    //        return 1;
    //}
    //return 0;
    return get_field(x,y)->players->count > 0;
}

int has_trap(const int x, const int y) {
    field_state* field = get_field(x,y);
    return 
        (field->enter_events->count || field->exit_events->count)
            ? 1 : 0;
}

field_scan scan_field(const int x, const int y) {
    field_state* field = get_field(x,y);
    return (field_scan) {
        ._ = 0,
        .obstruction = is_obstruction(x,y),
        .player = has_player(x,y),
        .trapped = has_trap(x,y),
        .flammable = is_flammable(x,y),
        .meltable = is_meltable(x,y),
        .cover = is_cover(x,y),
        .shelter = is_shelter(x,y),
        .is_empty = field->type == EMPTY,
        .is_trench = field->type == TRENCH,
        .is_ice_block = field->type == ICE_BLOCK,
        .is_tree = field->type == TREE,
        .is_ocean = field->type == OCEAN,
        .is_wall = field->type == WALL,
        .is_bridge = field->type == BRIDGE,
    };
}

void destroy_field(const int x, const int y, char* death_msg) {
    field_state* field = get_field(x,y);
    switch (field->type) {
        case OCEAN:
            break;
        case TREE: {
            field->type = EMPTY;
            break;
        }
        case BRIDGE: {
            field->type = OCEAN;
            break;
        }
        case WALL: {
            if (field->data->wall.fortified) {
                field->data->wall.fortified = 0;
                return;
            }
            field->type = EMPTY;
            free(field->data);
            break;
        }
        case TRENCH:{
            if (field->data->trench.fortified) {
                field->data->trench.fortified = 0;
                return;
            }
            field->type = EMPTY;
            free(field->data);
            break;
        }
        case ICE_BLOCK: {
            field->type = EMPTY;
            free(field->data);
            break;
        }
    }
    for(int i = 0; i < field->players->count; i++) {
        player_state* player = get_player(field->players, i);
        death_mark_player(player, death_msg);
    }
}

void remove_field(const int x, const int y) {
    field_state* field = get_field(x,y);
    switch (field->type) {
        case OCEAN:
        case TREE: {
            field->type = EMPTY;
            break;
        }
        case BRIDGE: {
            field->type = OCEAN;
            break;
        }
        case WALL:
        case TRENCH: {
            field->type = EMPTY;
            free(field->data);
            break;
        }
        case ICE_BLOCK: {
            field_data* old_data = field->data;
            field->data = old_data->ice_block.inner;
            field->type = old_data->ice_block.inner_type;
            free(old_data);
            break;
        }
    }
}

void damage_field(const int x, const int y, damage_t d_type, char* death_msg) {
    field_scan scan = scan_field(x,y);
    if (scan.flammable && IS_DAMAGE_TYPE(FIRE_DMG, d_type))
        destroy_field(x,y,death_msg);
    else if (scan.meltable && IS_DAMAGE_TYPE(FIRE_DMG, d_type))
        remove_field(x,y);
}

int fortify_field(const int x, const int y) {
    field_state* field = get_field(x,y);
    switch (field->type) {
        case TRENCH: {
            field->data->trench.fortified = 1;
            return 1;
        }
        case WALL: {
            field->data->wall.fortified = 1;
            return 1;
        }
        default: return 0;
    }
}


void build_trench(const int x, const int y) {
    field_state* field = get_field(x,y);

    field_data* data = malloc(sizeof(field_data));
    data->trench.fortified = 0;

    field->type = TRENCH;
    field->data = data;
}

void build_wall(const int x, const int y) {
    field_state* field = get_field(x,y);

    field_data* data = malloc(sizeof(field_data));
    data->wall.fortified = 0;

    field->type = WALL;
    field->data = data;
}

void build_tree(const int x, const int y) {
    field_state* field = get_field(x,y);

    field->type = TREE;
}

const fields_namespace fields = {
    .get = &get_field,
    .set = &set_field,
    .is_obstruction = &is_obstruction,
    .is_flammable = &is_flammable,
    .is_cover = &is_cover,
    .is_shelter = &is_shelter,
    .has_player = &has_player,
    .has_trap = &has_trap,
    .scan = &scan_field,
    .destroy_field = &destroy_field,
    .damage_field = &damage_field,
    .remove_field = &remove_field,
    .fortify_field = &fortify_field,
    .build = {
        .trench = &build_trench,
        .wall = &build_wall,
        .tree = &build_tree,
    },
};