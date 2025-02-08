#include "field_scan.h"

#include "game_state.h"

field_scan scan_field(const int x, const int y) {

    field_state* field = get_field(x,y);

    unsigned int obstruction;
    switch (field->type) {
        case EMPTY:
        case TRENCH:
            obstruction = 0;
    }

    unsigned int player_found;
    for(int i = 0; i < _gs->players->list->count; i++) {
        player_state* player = get_player(_gs->players, i);
        if (player->x == x && player->y == y) {
            player_found = 1;
            break;
        }
    }

    return (field_scan) {
        ._ = 0,
        .obstruction = obstruction,
        .player = player_found,
        .trapped = field->enter_events->list || field->exit_events->list,
        .trench = (field->type == TRENCH) ? 1 : 0,
    };
}