#ifndef FIELDS_H
#define FIELDS_H

#include "event_list.h"
#include "color.h"

typedef enum {
    EMPTY,
    TRENCH,
    ICE_BLOCK,
    TREE,
} field_type;


typedef int (*const field_property_check)(const int x, const int y);

typedef union field_data field_data;

//typedef struct field_data {
//  field_type type;
    typedef union field_data {
        struct {
            unsigned int fortified : 1;
        } trench;
        struct {
            field_data* inner;
            field_type inner_type;
        } ice_block;
    } field_data;
//} field_data;

typedef struct {
    unsigned int obstruction : 1;
    unsigned int player : 1;
    unsigned int trapped : 1;
    unsigned int flammable : 1;
    unsigned int shelter : 1;
    unsigned int cover : 1;
    unsigned int _ : 26;
} field_scan;

typedef struct field_state {
    color* foreground_color_overlay;
    color* background_color_overlay;
    print_mod mod_overlay;
    char* symbol_overlay;
    field_type type;
    field_data* data;
    int player_data; // Players can read and write here
    event_list_t* enter_events;
    event_list_t* exit_events;
} field_state;

typedef struct fields_namespace {
    field_state* (*const get)(const int x, const int y);
    void (*const set)(const int x, const int y, field_state* f);
    field_property_check is_obstruction;
    field_property_check is_flammable;
    field_property_check is_shelter;
    field_property_check is_cover;
    field_property_check has_player;
    field_property_check has_trap;
    void (*const destroy_field)(const int x, const int y, char* death_msg);
    field_scan (*const scan)(const int x, const int y);
} fields_namespace;

extern const fields_namespace fields;

#endif