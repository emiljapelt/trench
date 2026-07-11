#ifndef SYMBOLS_H
#define SYMBOLS_H

#define TRM_TRENCH '+'
#define TRM_WALL 'w'
#define TRM_OCEAN '~'
#define TRM_TREE 'T'
#define TRM_CLAY 'C'
#define TRM_MOUNTAIN 'M'
// Unless defined, every character is an empty
#define TRM_EMPTY '.'

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
    MINE_SHAFT_VISUAL,
    MOUNTAIN_VISUAL,
} symbol;

extern const char* symbol_lookup[];
extern const int connector_lookup[];
extern const int fortified_connector_lookup[];

#endif