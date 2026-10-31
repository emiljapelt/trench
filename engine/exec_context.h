#ifndef EXEC_CONTEXT_H
#define EXEC_CONTEXT_H


typedef enum {
    ENTITY_CONTEXT,
    EVENT_CONTEXT,
} context_type;

//typedef struct player_state;

// Should the context maybe contain the remaining steps/actions, and -1 mean unlimited?
typedef struct exec_context {
    context_type type;
    int program_len;
    int* program;
    int* stack;
    int dp;
    int sp;
    int bp;
    int steps;
    int actions;
    union {
        struct entity_t* entity;
        struct event* event;
    };
    //int entity_id; // Related entity? pointer? union pointers to event/entity?
    //player_state* player;
} exec_context;

#endif