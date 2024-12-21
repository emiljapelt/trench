#ifndef EVENTS_H
#define EVENTS_H

#include "player.h"

typedef void (*field_event_function)(player_state* ps, void* data);

typedef struct field_event_list_node {
    field_event_function func;
    int clock;
    void* data;
    struct field_event_list_node* next;
}
field_event_list_node;

typedef struct field_event_list {
    field_event_list_node* list;
} field_event_list;

void add_field_event(field_event_list* list, int clock, field_event_function func, void* data);
void update_field_events(player_state* ps, field_event_list* list);


#endif