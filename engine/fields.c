#include "fields.h"

#include <stdlib.h>

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
        case ICE_BLOCK:
            return 1;
        default:
            return 0;
    }
}

// Something which is destroyed by fire
int is_flammable(const int x, const int y) {
    switch (get_field(x,y)->type) {
        case TREE:
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
    unsigned int player_found;
    for(int i = 0; i < _gs->players->count; i++) {
        player_state* player = get_player(_gs->players, i);
        if (player->alive && player->x == x && player->y == y)
            return 1;
    }
    return 0;
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
        .cover = is_cover(x,y),
        .shelter = is_shelter(x,y),
    };
}

void destroy_field(const int x, const int y, char* death_msg) {
    field_state* field = get_field(x,y);
    switch (field->type) {
        case TREE:{
            field->type = EMPTY;
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
    for(int i = 0; i < _gs->players->count; i++) {
        player_state* player = get_player(_gs->players, i);
        if (player->x == x && player->y == y) {
            death_mark_player(player, death_msg);
        }
    }
}

void damage_field(const int x, const int y, damage_t d_type, char* death_msg) {
    field_scan scan = scan_field(x,y);
    if (scan.flammable && IS_DAMAGE_TYPE(FIRE_DAMAGE, d_type))
        destroy_field(x,y,death_msg);
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
};