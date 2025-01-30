#include "event_list.h"
#include "game_state.h"
#include "linked_list.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>


void add_event(event_list* list, event_function func, void* data) {
    event* node = malloc(sizeof(event));
    node->func = func;
    node->data = data;
    list_add(&list->list, node);
}

int update_events(player_state* ps, event_list* list) {
    if (!list->list) return 0;

    int finished_count = 0;
    linked_list_node* node = list->list;
    while (node) {
        event* e = (event*)(node->data);
        int finished = e->func(ps, e->data);
        if (finished) { 
            e->func = NULL;
            finished_count++;
        }
        node = node->next;
    }

    event_list filtered = { .list = NULL };
    node = list->list;
    while(node) {
        linked_list_node* temp = node;
        event* e = (event*)temp->data;
        node = node->next;
        if (e->func) {
            temp->next = filtered.list;
            filtered.list = temp;
        }
        else {
            free(e->data);
            free(temp->data);
            free(temp);
        }
    }

    list_rev(&filtered.list);
    list->list = filtered.list;
    return finished_count;
}