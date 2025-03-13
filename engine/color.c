#include "color.h"

color _red = {.r = 255, .g = 0, .b = 0, .predef = 1};
color _green = {.r = 0, .g = 255, .b = 0, .predef = 1};
color _blue = {.r = 0, .g = 0, .b = 255, .predef = 1};
color _white = {.r = 255, .g = 255, .b = 255, .predef = 1};
color _black = {.r = 0, .g = 0, .b = 0, .predef = 1};
color _yellow = {.r = 255, .g = 255, .b = 0, .predef = 1};
color _ice_blue = {.r = 162, .g = 210, .b = 223, .predef = 1};
color _magic_purple = {.r = 207, .g = 159, .b = 255, .predef = 1};
color _dark_grey = {.r = 40, .g = 40, .b = 40, .predef = 1};
color _wood_brown = {.r = 133, .g = 94, .b = 66, .predef = 1};

const color_predef color_predefs = {
    .red = &_red,
    .green = &_green,
    .blue = &_blue,
    .white = &_white,
    .black = &_black,
    .yellow = &_yellow,
    .ice_blue = &_ice_blue,
    .magic_purple = &_magic_purple,
    .dark_grey = &_dark_grey,
    .wood_brown = &_wood_brown,
};