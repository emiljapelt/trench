#ifndef EVENT_LIST_H
#define EVENT_LIST_H

#include "player.h"
#include "linked_list.h"

// Return 1 if the event is finished, otherwise return 0.
typedef int (*event_function)(player_state* ps, void* data);

typedef struct event {
    event_function func;
    void* data;
} event;

void add_event(linked_list* list, event_function func, void* data);
int update_events(player_state* ps, linked_list* list);


#endif