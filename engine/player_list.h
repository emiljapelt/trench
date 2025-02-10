#ifndef PLAYER_LIST_H
#define PLAYER_LIST_H

#include "player.h"
#include "array_list.h"

typedef struct player_list {
    array_list_t* list;
} player_list;


typedef void (*player_unit_function)(player_state* ps);
typedef int (*player_bool_function)(player_state* ps);

void add_player(player_list* list, player_state* ps);
player_state* get_player(player_list* players, int index);
void each_player(player_list* list, player_unit_function func);
player_state* first_player(player_list* list, player_bool_function func);

#endif