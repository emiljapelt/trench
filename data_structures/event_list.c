#include "event_list.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

void add_event(event_list* list, int clock, event_function func, void* data) {
    event_list_node* node = malloc(sizeof(event_list_node));

    event_list_node node_value = {
        .clock = clock,
        .data = data,
        .func = func,
        .next = list->list
    };

    memcpy(node, &node_value, sizeof(event_list_node));

    list->list = node;
}

void update_events(char* world, event_list* list) {
    event_list_node* node = list->list;
    while (node) {
        node->clock--;
        if (!node->clock) node->func(world, node->data);
        node = node->next;
    }

    event_list filtered = { .list = NULL };
    node = list->list;
    while(node) {
        event_list_node* temp = node;
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