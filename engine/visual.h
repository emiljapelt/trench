#ifndef VISUAL_H
#define VISUAL_H

#include "color.h"

// Measured to 43b
#define MAX_SYMBOL_SIZE 43

#define FEED_WIDTH 16

typedef struct field_visual {
    color foreground_color;
    color background_color;
    const char* symbol;
    print_mod mod;
} field_visual;

color rgb_color(int r, int g, int b);
void print_board();

#endif