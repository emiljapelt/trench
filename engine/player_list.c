#include "player_list.h"

#include <stdlib.h>
#include <stdio.h>

#include "util.h"
#include "player.h"

void add_player(player_list_t* players, player_state* ps) {
    array_list.add(players, ps);
}

player_state* get_player(player_list_t* players, int index) {
    return (player_state*)array_list.get(players, index);
}

void each_player(player_list_t* players, player_unit_function func) {
    for(int i = 0; i < players->count; i++) {
        player_state* ps = get_player(players, i);
        func(ps);
    }
}

player_state* first_player(player_list_t* players, player_bool_function func) {
    for(int i = 0; i < players->count; i++) {
        player_state* ps = get_player(players, i);
        if (func(ps)) return ps;
    }
    return NULL;
}

void remove_player_id(player_list_t* list, int id) {
    for(int i = 0; i < list->count; i++) {
        if (get_player(list, i)->id == id) {
            array_list.remove(list, i, 0);
            return;
        }
    }
}