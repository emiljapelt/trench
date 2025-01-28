#include "player_list.h"

#include <stdlib.h>
#include <stdio.h>

#include "linked_list.h"
#include "player.h"

void add_player(linked_list* list, player_state* ps) {
    list_add(list, ps);
}

void each_player(linked_list* list, player_unit_function func) {
    linked_list_node* node = list->list;
    while(node) {
        player_state* ps = (player_state*)node->data;
        func(ps);
        node = node->next;
    }
}

player_state* first_player(linked_list* list, player_bool_function func) {
    linked_list_node* node = list->list;
    while(node) {
        player_state* ps = (player_state*)node->data;
        if (func(ps)) return ps;
        node = node->next;
    }
    return NULL;
}

void print_name(player_state* ps) {
    printf("%s\n", ps->name);
}