#ifndef PLAYER_LIST_H
#define PLAYER_LIST_H

#include "player.h"
#include "linked_list.h"

typedef struct player_list {
    linked_list_node* list;
} player_list;


typedef void (*player_unit_function)(player_state* ps);
typedef int (*player_bool_function)(player_state* ps);

void add_player(player_list* list, player_state* ps);
void each_player(player_list* list, player_unit_function func);
player_state* first_player(player_list* list, player_bool_function func);


void print_name(player_state* ps);

#endif