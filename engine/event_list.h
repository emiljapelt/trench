#ifndef EVENT_LIST_H
#define EVENT_LIST_H

#include "events.h"
#include "entity.h"
#include "array_list.h"

typedef array_list_t event_list_t;

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

void add_event(event_list_t* events, event_kind kind, event_function func, void* data);
void remove_event(event_list_t* events, int index);
int update_events(entity e, event_list_t* events);
event* get_event(event_list_t* events, int index);


#endif