#include "events.h"
#include "game_state.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>


void add_event(event_list* list, event_function func, void* data) {
    event_list_node* node = malloc(sizeof(event_list_node));

    event_list_node node_value = {
        .data = data,
        .func = func,
        .next = list->list
    };

    memcpy(node, &node_value, sizeof(event_list_node));

    list->list = node;
}

void update_events(player_state* ps, event_list* list) {
    event_list_node* node = list->list;
    while (node) {
        int finished = node->func(ps, node->data);
        if (finished) node->func = NULL;
        node = node->next;
    }

    event_list filtered = { .list = NULL };
    node = list->list;
    while(node) {
        event_list_node* temp = node;
        node = node->next;
        if (temp->func) {
            temp->next = filtered.list;
            filtered.list = temp;
        }
        else {
            free(temp->data);
            free(temp);
        }
    }

    event_list reversed = { .list = NULL};
    node = filtered.list;
    while (node) {
        event_list_node* temp = node;
        node = node->next;
        temp->next = reversed.list;
        reversed.list = temp;
    }

    list->list = reversed.list;
    
}