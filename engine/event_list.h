#ifndef EVENT_LIST_H
#define EVENT_LIST_H

#include "player.h"
#include "linked_list.h"

typedef struct event_list {
    linked_list_node* list;
} event_list;

// Return 1 if the event is finished, otherwise return 0.
typedef int (*event_function)(player_state* ps, void* data);

typedef struct event {
    event_function func;
    void* data;
} event;

void add_event(event_list* list, event_function func, void* data);
int update_events(player_state* ps, event_list* list);


#endif