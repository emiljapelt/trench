#ifndef FIELDS_H
#define FIELDS_H

#include "event_list.h"
#include "color.h"

typedef enum {
    EMPTY,
    TRENCH,
    ICE_BLOCK,
} field_type;

// forward declaration
typedef struct field_data field_data;

typedef struct {
    unsigned int fortified : 1;
} trench_field;

typedef struct {
    field_data* inner;
} ice_block_field;

typedef struct field_data {
    field_type type;
    union {
        trench_field trench;
        ice_block_field ice_block;
    } data;
} field_data;

typedef struct {
    unsigned int obstruction : 1;
    unsigned int trench : 1;
    unsigned int player : 1;
    unsigned int trapped : 1;
    unsigned int _ : 28;
} field_scan;

typedef struct field_state {
    const color* foreground_color_overlay;
    const color* background_color_overlay;
    print_mod mod_overlay;
    const char* symbol_overlay;
    field_data* data;
    int player_data; // Players can read and write here
    event_list* enter_events;
    event_list* exit_events;
} field_state;

typedef struct fields_namespace {
    field_state* (*get)(const int x, const int y);
    void (*set)(const int x, const int y, field_state* f);
    int (*has_obstruction)(int x, int y);
    int (*has_trench)(field_data*);
    int (*has_player)(int x, int y);
    int (*has_trap)(int x, int y);
    field_scan (*scan)(const int x, const int y);
} fields_namespace;

extern const fields_namespace fields;

#endif