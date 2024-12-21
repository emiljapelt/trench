#include "events.h"
#include "game_state.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>


void add_field_event(field_event_list* list, int clock, field_event_function func, void* data) {
    field_event_list_node* node = malloc(sizeof(field_event_list_node));

    field_event_list_node node_value = {
        .clock = clock,
        .data = data,
        .func = func,
        .next = list->list
    };

    memcpy(node, &node_value, sizeof(field_event_list_node));

    list->list = node;
}

void update_field_events(player_state* ps, field_event_list* list) {
    field_event_list_node* node = list->list;
    while (node) {
        node->clock--;
        if (!node->clock) node->func(ps, node->data);
        node = node->next;
    }

    field_event_list filtered = { .list = NULL };
    node = list->list;
    while(node) {
        field_event_list_node* temp = node;
        node = node->next;
        if (temp->clock > 0) {
            temp->next = filtered.list;
            filtered.list = temp;
        }
        else {
            free(temp->data);
            free(temp);
        }
    }

    field_event_list reversed = { .list = NULL};
    node = filtered.list;
    while (node) {
        field_event_list_node* temp = node;
        node = node->next;
        temp->next = reversed.list;
        reversed.list = temp;
    }

    list->list = reversed.list;
    
}