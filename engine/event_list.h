#ifndef EVENT_LIST_H
#define EVENT_LIST_H

#include "events.h"
#include "entity.h"
#include "array_list.h"

typedef array_list_t event_list_t;

typedef enum {
    NONE_EVENT,
    FIELD_EVENT,
    PHYSICAL_EVENT,
    MAGICAL_EVENT,
} event_kind;

typedef enum {
    STATELESS_EVENT, // Single shared stack, which is overwritten before exec
    STATEFUL_EVENT,
    SHARED_EVENT, // All events of this template share one stack
} event_type;

extern int* stateless_stack;

typedef struct event_template {
    event_type type : 8;
    event_kind kind : 8;
    int* program;
    unsigned int program_len;
    int* stack; // for shared state event types
} event_template;

typedef struct event {
    event_template* template;
    unsigned int cancelled : 1;
    int* stack;
    int* args;
    exec_context* exec;
} event;

void add_event(event_list_t* events, event_template* template, int* args);
void remove_event(event_list_t* events, int index);
int update_events(entity_t* e, event_list_t* events);
event* get_event(event_list_t* events, int index);
void remove_events_of_kind(event_list_t* events, event_kind kind);
void free_event(event* event);

#endif