#ifndef PLAYER_LIST_H
#define PLAYER_LIST_H

#include "player.h"
#include "array_list.h"

typedef array_list_t player_list_t;

typedef void (*player_unit_function)(player_state* ps);
typedef int (*player_bool_function)(player_state* ps);

void add_player(player_list_t* list, player_state* ps);
player_state* get_player(player_list_t* players, int index);
void each_player(player_list_t* list, player_unit_function func);
player_state* first_player(player_list_t* list, player_bool_function func);
void remove_player_id(player_list_t* list, int id);

#endif