#ifndef EVENTS_H
#define EVENTS_H

#include "player.h"

// Return 1 if the event is finished, otherwise return 0.
typedef int (*event_function)(player_state* ps, void* data);

typedef struct event_list_node {
    event_function func;
    void* data;
    struct event_list_node* next;
}
event_list_node;

typedef struct event_list {
    event_list_node* list;
} event_list;

void add_event(event_list* list, event_function func, void* data);
int update_events(player_state* ps, event_list* list);


#endif