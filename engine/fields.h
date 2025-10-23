#ifndef FIELDS_H
#define FIELDS_H

#include "event_list.h"
#include "entity_list.h"
#include "color.h"
#include "damage.h"
#include "vehicles.h"
#include "events.h"

typedef enum {
    EMPTY,
    TRENCH,
    ICE_BLOCK,
    TREE,
    OCEAN,
    WALL,
    BRIDGE,
    CLAY,
} field_type;

typedef union field_data field_data;

typedef union field_data {
    struct {
        unsigned int fortified : 1;
    } trench;

    struct {
        unsigned int fortified : 1;
    } wall;

    struct {
        field_data* inner;
        field_type inner_type;
        ice_block_melt_event_args* melt_event_args;
    } ice_block;

    struct {
        int amount;
    } clay_pit;
    
} field_data;


#define PROP_OBSTRUCTION    0b00000000000000000000000000000001
#define PROP_PLAYER         0b00000000000000000000000000000010
#define PROP_TRAPPED        0b00000000000000000000000000000100
#define PROP_FLAMMABLE      0b00000000000000000000000000001000
#define PROP_COVER          0b00000000000000000000000000010000
#define PROP_SHELTER        0b00000000000000000000000000100000
#define PROP_MELTABLE       0b00000000000000000000000001000000
#define PROP_IS_EMPTY       0b00000000000000000000000010000000
#define PROP_IS_TRENCH      0b00000000000000000000000100000000
#define PROP_IS_ICE_BLOCK   0b00000000000000000000001000000000
#define PROP_IS_TREE        0b00000000000000000000010000000000
#define PROP_IS_OCEAN       0b00000000000000000000100000000000
#define PROP_IS_WALL        0b00000000000000000001000000000000
#define PROP_IS_BRIDGE      0b00000000000000000010000000000000
#define PROP_IS_CLAY        0b00000000000000000100000000000000


typedef struct field_state {
    color* foreground_color_overlay;
    color* background_color_overlay;
    print_mod mod_overlay : 8;
    char* symbol_overlay;
    field_type type;
    field_data* data;
    int player_data; // Players can read and write here
    entity_list_t* entities;
    event_list_t* enter_events;
    event_list_t* exit_events;
} field_state;

typedef struct field_builders {
    void (*const trench)(field_state* field);
    void (*const wall)(field_state* field);
    void (*const tree)(field_state* field);
    void (*const clay_pit)(field_state* field);
    void (*const ocean)(field_state* field);
} field_builders;

typedef struct fields_namespace {
    field_state* (*const get)(const int x, const int y);
    void (*const set)(const int x, const int y, field_state* f);
    void (*const destroy_field)(field_state* field, char* death_msg);
    void (*const damage_field)(field_state* field, damage_t d_type, char* death_msg);
    void (*const remove_field)(field_state* field);
    int (*const fortify_field)(field_state* field);
    unsigned int (*const properties_of_field)(field_state* field);
    unsigned int (*const properties)(const int x, const int y);
    field_builders build;
} fields_namespace;

extern const fields_namespace fields;

#endif