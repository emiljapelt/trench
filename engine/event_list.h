#ifndef EVENT_LIST_H
#define EVENT_LIST_H

#include "player.h"
#include "array_list.h"

typedef struct event_list {
    array_list* list;
} event_list;

// Return 1 if the event is finished, otherwise return 0.
typedef int (*event_function)(player_state* ps, void* data);

typedef enum {
    NONE_EVENT,
    PHYSICAL_EVENT,
    MAGICAL_EVENT,
} event_kind;

typedef struct event {
    event_function func;
    event_kind kind;
    void* data;
} event;

void add_event(event_list* events, event_kind kind, event_function func, void* data);
void remove_event(event_list* events, int index);
int update_events(player_state* ps, event_list* events);


#endif