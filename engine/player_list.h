#ifndef PLAYER_LIST_H
#define PLAYER_LIST_H

#include "player.h"

typedef void (*player_unit_function)(player_state* ps);
typedef int (*player_bool_function)(player_state* ps);

void add_player(linked_list* list, player_state* ps);
void each_player(linked_list* list, player_unit_function func);
player_state* first_player(linked_list* list, player_bool_function func);


void print_name(player_state* ps);

#endif