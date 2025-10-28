#ifndef VISUAL_H
#define VISUAL_H

#include "color.h"

typedef enum symbol {
    ASTERIX,
    NONE,
    F_NONE,
    N,
    F_N,
    E,
    F_E,
    S,
    F_S,
    W,
    F_W,
    NS,
    F_NS,
    EW,
    F_EW,
    NE,
    F_NE,
    SE,
    F_SE,
    NW,
    F_NW,
    SW,
    F_SW,
    NES,
    F_NES,
    ESW,
    F_ESW,
    SWN,
    F_SWN,
    WNE,
    F_WNE,
    ALL,
    F_ALL,
    PERSON,
    EXPLOSION,
    TARGET,
    MINE,
    SKULL,
    COFFIN,
    SNOWFLAKE,
    TREE_VISUAL,
    BOAT_VISUAL,
    FILLED_CIRCLE,
    BULLET,
    EMPTY_DIAMOND,
    BEAR_TRAP,
    PENTAGRAM,
    LARGE_X,
    MIDDOT,
    LIGHTNING,
    DOTS_NS,
    DOTS_EW,
    HOUSE_VISUAL,
    SINGLE_DOT,
    DOUBLE_DOT,
    TRIPLE_DOT,
    QUAD_DOT,
    PENTA_DOT,
} symbol;

extern const char* symbol_lookup[];
extern const int connector_lookup[];
extern const int fortified_connector_lookup[];

// Measured to 19b
// Calculated to 35b
#define MAX_SYMBOL_SIZE 40

#define FEED_WIDTH 16

typedef enum {
    FORE,
    BACK,
} color_target;

typedef struct field_visual {
    color foreground_color;
    color background_color;
    const char* symbol;
    print_mod mod;
} field_visual;

color rgb_color(int r, int g, int b);
void clear_screen(void);
void reset_cursor(void);
void print_board();

#endif