#include "event_list.h"
#include "game_state.h"
#include "array_list.h"
#include "entity.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>


void add_event(event_list_t* events, event_kind kind, event_function func, void* data) {
    event* e = malloc(sizeof(event));
    e->data = data;
    e->func = func;
    e->kind = kind;
    array_list.add(events, e);
}

event* get_event(event_list_t* events, int index) {
    return (event*)array_list.get(events, index);
}

void remove_event(event_list_t* events, int index) {
    array_list.remove(events, index, 0);
}

int update_events(entity_t* entity, event_list_t* events) {

    int finished_count = 0;
    for(int i = 0; i < events->count; i++) {
        event* e = get_event(events, i);
        if (e->func && e->func(entity, e->data)) { 
            e->func = NULL;
            finished_count++;
        }
    }

    for(int i = 0; i < events->count; i++) {
        event* e = get_event(events, i);
        if (e->func == NULL) {
            remove_event(events, i);
            free(e->data);
            free(e);
        }
    }

    return finished_count;
}