#ifndef TILE_H
#define TILE_H

#include "color.h"
#include "array_list.h"
                                                     
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
#define PROP_IS_MINE_SHAFT  0b00000000000000001000000000000000
#define PROP_IS_MOUNTAIN    0b00000000000000010000000000000000
#define PROP_IS_ENEMY       0b00000000000000100000000000000000
#define PROP_IS_ALLY        0b00000000000001000000000000000000
#define PROP_IS_VEHICLE     0b00000000000010000000000000000000

typedef enum {
    NO_SYMBOL,
    SYMBOL,
    CONNECTOR,
} symbol_type;

typedef struct symbol {
    symbol_type type;
    union {
        int symbol;
        unsigned int connector;
    };
} symbol;

// Constants
typedef struct tile_template {
    char trm; // int for UTF-8 support?
    unsigned int transparent_properties : 1;
    unsigned int transparent_fg : 1;
    unsigned int transparent_bg : 1;
    unsigned int transparent_mod : 1;
    unsigned int hardend : 1; // Protects underlying tiles, better name?
    //unsigned int indestructable : 1; ???or a hardness level???
    unsigned int properties;
    unsigned int weaknesses;
    color fg;
    color bg;
    unsigned int mod;
    symbol symbol; // NULL is transparent
    // events
} tile_template;

typedef struct tile {
    tile_template* template;
    // events
} tile;

typedef array_list_t tile_stack;

void push_tile(tile_stack* s,tile* t);
tile* pop_tile(tile_stack* s);
tile* peek_tile(tile_stack* s);

tile* create_tile(const tile_template*);

unsigned int get_tile_properties(tile_stack* stack);
symbol get_tile_symbol(tile_stack* stack);
color get_tile_fg(tile_stack* stack);
color get_tile_bg(tile_stack* stack);
unsigned int get_tile_mod(tile_stack* stack);

extern const int tile_template_count;
extern const tile_template tile_template_lookup[];

#endif