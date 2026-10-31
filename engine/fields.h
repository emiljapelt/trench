#ifndef FIELDS_H
#define FIELDS_H

#include "event_list.h"
#include "entity_list.h"
#include "color.h"
#include "damage.h"
#include "vehicles.h"
#include "events.h"
#include "visual.h"
#include "symbols.h"
#include "tile.h"

typedef enum {
    EMPTY,
    TRENCH,
    ICE_BLOCK,
    TREE,
    OCEAN,
    WALL,
    BRIDGE,
    CLAY,
    MINE_SHAFT,
    MOUNTAIN,
} field_type;

/*typedef union field_data field_data;

typedef union field_data {
    struct {
        unsigned int fortified : 1;
    } trench;

    struct {
        unsigned int fortified : 1;
    } wall;

    struct {
        unsigned int fortified : 1;
    } mine_shaft;

    struct {
        field_data* inner;
        field_type inner_type : 8;
        ice_block_melt_event_args* melt_event_args;
    } ice_block;

    struct {
        int amount;
    } clay_pit;
    
} field_data;*/


// move to visual.h ?
#define FOREGROUND_COLOR_OVERLAY    0b0001
#define BACKGROUND_COLOR_OVERLAY    0b0010
#define MOD_OVERLAY                 0b0100
#define SYMBOL_OVERLAY              0b1000


int connections(int x, int y, field_type field);

typedef struct field_state {
    color_predef foreground_color : 8;
    color_predef background_color : 8;
    print_mod mod : 8;
    int symbol;
    int overlays : 4;
    tile_stack* tiles;
    int player_data; // Players can read and write here
    entity_list_t* entities;
    event_list_t* enter_events;
    event_list_t* exit_events;
    resource_registry resources;
} field_state;

/*typedef struct field_builders {
    void (*const trench)(field_state* field);
    void (*const wall)(field_state* field);
    void (*const tree)(field_state* field);
    void (*const clay_pit)(field_state* field);
    void (*const ocean)(field_state* field);
    void (*const mine_shaft)(field_state* field);
} field_builders;*/

typedef struct fields_namespace {
    field_state* (*const get)(const int x, const int y);
    void (*const set)(const int x, const int y, field_state* f);
    void (*const destroy_field)(field_state* field, char* death_msg);
    void (*const damage_field)(field_state* field, damage_t d_type, player_state* player, char* death_msg);
    //void (*const remove_field)(field_state* field);
    //int (*const fortify_field)(field_state* field);
    unsigned int (*const properties_of_field)(field_state* field, player_state* player);
    unsigned int (*const properties)(const int x, const int y, player_state* player);
    //field_builders build;
} fields_namespace;

field_visual get_field_visual(const int x, const int y, field_state* field);

extern const fields_namespace fields;

#endif