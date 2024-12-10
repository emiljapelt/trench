#ifndef H_EVENT_LIST
#define H_EVENT_LIST

typedef void (*event_function)(char* world, void* data);

typedef struct event_list_node
{
    event_function func;
    int clock;
    void* data;
    struct event_list_node* next;
}
event_list_node;

typedef struct event_list
{
    event_list_node* list;
} event_list;

void add_event(event_list* list, int clock, event_function func, void* data);
void update_events(char* world, event_list* list);


#endif