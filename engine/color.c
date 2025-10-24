#include "color.h"

const color color_lookup[] = {
    {.r = 255, .g = 0, .b = 0},
    {.r = 0, .g = 255, .b = 0},
    {.r = 0, .g = 0, .b = 255},
    {.r = 255, .g = 255, .b = 255},
    {.r = 0, .g = 0, .b = 0},
    {.r = 255, .g = 255, .b = 0},
    {.r = 162, .g = 210, .b = 223},
    {.r = 207, .g = 159, .b = 255},
    {.r = 40, .g = 40, .b = 40},
    {.r = 133, .g = 94, .b = 66},
    {.r = 195, .g = 140, .b = 95},
};

int color_eq(color c1, color c2) {
    return c1.r == c2.r && c1.g == c2.g && c1.b == c2.b;
}