#ifndef COLOR_H
#define COLOR_H

typedef struct color {
    unsigned int r : 8;
    unsigned int g : 8;
    unsigned int b : 8;
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

extern const color color_lookup[];

typedef enum color_predef {
    RED,
    GREEN,
    BLUE,
    WHITE,
    BLACK,
    YELLOW,
    ICE_BLUE,
    MAGIC_PURPLE,
    DARK_GREY,
    WOOD_BROWN,
    CLAY_BROWN,
} color_predef;

int color_eq(color c1, color c2);

#endif