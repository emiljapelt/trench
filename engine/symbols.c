#include "symbols.h"

const char* symbol_lookup[] = {
    "*",
    "\u2219",
    "\u2022",
    "\u2575",
    "\u2579",
    "\u2576",
    "\u257a",
    "\u2577",
    "\u257b",
    "\u2574",
    "\u2578",
    "\u2502",
    "\u2503",
    "\u2500",
    "\u2501",
    "\u2514",
    "\u2517",
    "\u250c",
    "\u250f",
    "\u2518",
    "\u251b",
    "\u2510",
    "\u2513",
    "\u251c",
    "\u2523",
    "\u252c",
    "\u2533",
    "\u2524",
    "\u252b",
    "\u2534",
    "\u253b",
    "\u253c",
    "\u254b",
    "\u1330",
    "\u2311",
    "\u2316",
    "\u2313",
    "\u2620",
    "\u26b0",
    "\u2744",
    "\u219f",
    "\u2359", // "\u22ed"       "\u23c5" "\u26f5" // For some reason these print wrong
    "\u2b24",
    "\u2022",
    "\u27d0",
    "\u26ba",
    "\u26e4",
    "\u2a09",
    "\u00b7",
    "\u21af", // 26a1 this also print a char extra :(
    "\u22ee",
    "\u22ef",
    "\u2302",
    "\u2022",
    "\u205a",
    "\u2056",
    "\u2058",
    "\u2059",
    "\u2612",
    "A", // "\u26f0" 
};


// 0xNESW
const int connector_lookup[] = {
    NONE, // 0x0000
    W, // 0x0001
    S, // 0x0010
    SW, // 0x0011
    E, // 0x0100
    EW, // 0x0101
    SE, // 0x0110
    ESW, // 0x0111
    N, // 0x1000
    NW, // 0x1001
    NS, // 0x1010
    SWN, // 0x1011
    NE, // 0x1100
    WNE, // 0x1101
    NES, // 0x1110
    ALL, // 0x1111
};
const int fortified_connector_lookup[] = {
    F_NONE, // 0x0000
    F_W, // 0x0001
    F_S, // 0x0010
    F_SW, // 0x0011
    F_E, // 0x0100
    F_EW, // 0x0101
    F_SE, // 0x0110
    F_ESW, // 0x0111
    F_N, // 0x1000
    F_NW, // 0x1001
    F_NS, // 0x1010
    F_SWN, // 0x1011
    F_NE, // 0x1100
    F_WNE, // 0x1101
    F_NES, // 0x1110
    F_ALL, // 0x1111
};