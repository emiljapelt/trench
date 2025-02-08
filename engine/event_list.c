#include "event_list.h"
#include "game_state.h"
#include "array_list.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>


void add_event(event_list* events, event_kind kind, event_function func, void* data) {
    event* e = malloc(sizeof(event));
    e->data = data;
    e->func = func;
    e->kind = kind;
    list_add(events->list, e);
}

event* get_event(event_list* events, int index) {
    return (event*)list_get(events->list, index);
}

void remove_event(event_list* events, int index) {
    list_remove(events->list, index, 0);
}

int update_events(player_state* player, event_list* events) {

    int finished_count = 0;
    for(int i = 0; i < events->list->count; i++) {
        event* e = get_event(events, i);
        if (e->func(player, e->data)) { 
            e->func = NULL;
            finished_count++;
        }
    }

    for(int i = 0; i < events->list->count; i++) {
        event* e = get_event(events, i);
        if (e->func == NULL) {
            remove_event(events, i);
            free(e->data);
            free(e);
        }
    }

    return finished_count;
}