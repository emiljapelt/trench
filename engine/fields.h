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


typedef struct {
    unsigned int obstruction : 1;
    unsigned int player : 1;
    unsigned int trapped : 1;
    unsigned int flammable : 1;
    unsigned int cover : 1;
    unsigned int shelter : 1;
    unsigned int meltable : 1;
    unsigned int is_empty: 1;
    unsigned int is_trench: 1;
    unsigned int is_ice_block: 1;
    unsigned int is_tree: 1;
    unsigned int is_ocean: 1;
    unsigned int is_wall: 1;
    unsigned int is_bridge: 1;
    unsigned int is_clay: 1;

    unsigned int _ : 17;
} field_properties;

typedef struct field_state {
    color* foreground_color_overlay;
    color* background_color_overlay;
    print_mod mod_overlay;
    char* symbol_overlay;
    field_type type;
    field_data* data;
    int player_data; // Players can read and write here
    //player_list_t* players;
    entity_list_t* entities;
    event_list_t* enter_events;
    event_list_t* exit_events;
    //vehicle_state* vehicle;
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
    field_properties (*const properties_of_field)(field_state* field);
    field_properties (*const properties)(const int x, const int y);
    field_builders build;
} fields_namespace;

extern const fields_namespace fields;

#endif