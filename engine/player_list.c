#include "player_list.h"

#include <stdlib.h>
#include <stdio.h>

#include "util.h"
#include "player.h"

void add_player(player_list* players, player_state* ps) {
    list_add(players->list, ps);
}

player_state* get_player(player_list* players, int index) {
    return (player_state*)list_get(players->list, index);
}

void each_player(player_list* players, player_unit_function func) {
    for(int i = 0; i < players->list->count; i++) {
        player_state* ps = get_player(players, i);
        func(ps);
    }
}

player_state* first_player(player_list* players, player_bool_function func) {
    for(int i = 0; i < players->list->count; i++) {
        player_state* ps = get_player(players, i);
        if (func(ps)) return ps;
    }
    return NULL;
}