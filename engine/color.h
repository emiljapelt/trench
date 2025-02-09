#ifndef COLOR_H
#define COLOR_H

typedef struct color {
    unsigned int r : 8;
    unsigned int g : 8;
    unsigned int b : 8;
    unsigned int predef: 1;
} color;

typedef enum print_mod {
    BOLD = 1,
    SLIM = 2,
    ITALIC = 3,
    UNDERLINE = 4, 
    SLOW_BLINK = 5,
    FAST_BLINK = 6,
    CROSS_OUT = 9,
} print_mod;

typedef struct color_predef {
    color* red;
    color* green;
    color* blue;
    color* white;
    color* black;
    color* yellow;
    color* ice_blue;
} color_predef;

#endif