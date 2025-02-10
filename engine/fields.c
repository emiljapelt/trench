#include "fields.h"

#include <stdlib.h>

#include "game_state.h"

field_state* get_field(const int x, const int y) {
    return _gs->board + ((y * _gs->board_x) + x);
}

void set_field(const int x, const int y, field_state* f) {
    _gs->board[(y * _gs->board_x) + x] = *f;
}

int has_obstruction(const int x, const int y) {
    switch (get_field(x,y)->data->type) {
        case ICE_BLOCK:
            return 1;
        default:
            return 0;
    }
}

int has_trench(const field_data* field) {
    if (field == NULL) return 0;
    switch (field->type) {
        case TRENCH:
            return 1;
        case ICE_BLOCK:
            return has_trench(field->data.ice_block.inner);
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
        .obstruction = has_obstruction(x,y),
        .player = has_player(x,y),
        .trapped = has_trap(x,y),
        .trench = has_trench(field->data),
    };
}

const fields_namespace fields = {
    .get = &get_field,
    .set = &set_field,
    .has_obstruction = &has_obstruction,
    .has_trench = &has_trench,
    .has_player = &has_player,
    .has_trap = &has_trap,
    .scan = &scan_field,
};