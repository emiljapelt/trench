#include "event_list.h"

#include "game_state.h"
#include "array_list.h"
#include "entity.h"
#include "execute.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

int* stateless_stack;

void add_event(event_list_t* events, event_template* template, int* args) {
    event* e = malloc(sizeof(event));
    e->cancelled = 0;
    e->template = template;
    e->args = args;
    e->exec = malloc(sizeof(exec_context));

    switch (template->type) {
        case STATELESS_EVENT:
            e->stack = stateless_stack;
            break;
    
        case STATEFUL_EVENT:
            e->stack = malloc(sizeof(int) * 100); // Size select option?
            memset(e->stack, 0, sizeof(int) * 100);
            break;

        case SHARED_EVENT:
            e->stack = template->stack;
            break;
    }

    // TODO: Helper method :)
    e->exec->program = e->template->program;
    e->exec->program_len = e->template->program_len;
    e->exec->stack = e->stack; // TODO: A bit weird with so many pointers to the stack???                    
    e->exec->bp = 0;
    e->exec->sp = 0;
    e->exec->dp = 0;
    e->exec->actions = -1;
    e->exec->steps = -1;
    e->exec->event = e;
    e->exec->type = EVENT_CONTEXT;

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
        if (!e->cancelled) { 
            exec(e->exec);
            finished_count++;
        }
    }

    for(int i = 0; i < events->count; i++) {
        event* e = get_event(events, i);
        if (e->cancelled) {
            remove_event(events, i--);
            free_event(e);
        }
    }

    return finished_count;
}

// rename to cancel_events_of_kind
void remove_events_of_kind(event_list_t* events, event_kind kind) {
    for (int i = 0; i < events->count; i++) {
        event* e = get_event(events, i);
        if (e->template->kind == kind)
            e->cancelled = 1;
    }
}

void free_event(event* event) {
    if (event->args != NULL)
        free(event->args);
    if (event->template->type == STATEFUL_EVENT && event->stack != NULL)
        free(event->stack);
    free(event);
}