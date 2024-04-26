#include <stdio.h>

#include "game_state.h"

static inline void clear_screen(void) {
    // use cursor movements instead, avoids flickering
    // https://stackoverflow.com/questions/26423537/how-to-position-the-input-text-cursor-in-c
    //printf("\e[1;1H\e[2J");
    printf("\033[H\033[J");
    //printf("\033[%d;%dH", (0), (0));
}

char get_field_char(int x, int y, game_state* gs) {
    if (get_field(x,y,gs)->destroyed) return '*';
    field_state *fld = get_field(x,y,gs);
    char ctrl = fld->controller;
    if(ctrl == 0) return ' ';
    if(
        (in_bounds(x,y-1,gs) ? get_field(x,y-1,gs)->controller == ctrl : 0) && // N
        (in_bounds(x+1,y,gs) ? get_field(x+1,y,gs)->controller == ctrl : 0) && // E
        (in_bounds(x,y+1,gs) ? get_field(x,y+1,gs)->controller == ctrl : 0) && // S
        (in_bounds(x-1,y,gs) ? get_field(x-1,y,gs)->controller == ctrl : 0)    // W
    ) return fld->fortified ? 206 : 197;
    if(
        (in_bounds(x,y-1,gs) ? get_field(x,y-1,gs)->controller == ctrl : 0) && // N
        (in_bounds(x+1,y,gs) ? get_field(x+1,y,gs)->controller == ctrl : 0) && // E
        (in_bounds(x,y+1,gs) ? get_field(x,y+1,gs)->controller != ctrl : 0) && // S
        (in_bounds(x-1,y,gs) ? get_field(x-1,y,gs)->controller == ctrl : 0)    // W
    ) return fld->fortified ? 202 : 193;
    if(
        (in_bounds(x,y-1,gs) ? get_field(x,y-1,gs)->controller != ctrl : 0) && // N
        (in_bounds(x+1,y,gs) ? get_field(x+1,y,gs)->controller == ctrl : 0) && // E
        (in_bounds(x,y+1,gs) ? get_field(x,y+1,gs)->controller == ctrl : 0) && // S
        (in_bounds(x-1,y,gs) ? get_field(x-1,y,gs)->controller == ctrl : 0)    // W
    ) return fld->fortified ? 203 : 194;
    if(
        (in_bounds(x,y-1,gs) ? get_field(x,y-1,gs)->controller == ctrl : 0) && // N
        (in_bounds(x+1,y,gs) ? get_field(x+1,y,gs)->controller != ctrl : 0) && // E
        (in_bounds(x,y+1,gs) ? get_field(x,y+1,gs)->controller == ctrl : 0) && // S
        (in_bounds(x-1,y,gs) ? get_field(x-1,y,gs)->controller == ctrl : 0)    // W
    ) return fld->fortified ? 185 : 180;
    if(
        ((in_bounds(x,y-1,gs) ? get_field(x,y-1,gs)->controller != ctrl : 0) && // N
        (in_bounds(x+1,y,gs) ? get_field(x+1,y,gs)->controller == ctrl : 0) &&  // E
        (in_bounds(x,y+1,gs) ? get_field(x,y+1,gs)->controller != ctrl : 0) &&  // S
        (in_bounds(x-1,y,gs) ? get_field(x-1,y,gs)->controller == ctrl : 0))    // W
        ||
        ((in_bounds(x,y-1,gs) ? get_field(x,y-1,gs)->controller != ctrl : 0) && // N
        (in_bounds(x+1,y,gs) ? get_field(x+1,y,gs)->controller == ctrl : 0) &&  // E
        (in_bounds(x,y+1,gs) ? get_field(x,y+1,gs)->controller != ctrl : 0) &&  // S
        (in_bounds(x-1,y,gs) ? get_field(x-1,y,gs)->controller != ctrl : 0))    // W
        ||
        ((in_bounds(x,y-1,gs) ? get_field(x,y-1,gs)->controller != ctrl : 0) && // N
        (in_bounds(x+1,y,gs) ? get_field(x+1,y,gs)->controller != ctrl : 0) &&  // E
        (in_bounds(x,y+1,gs) ? get_field(x,y+1,gs)->controller != ctrl : 0) &&  // S
        (in_bounds(x-1,y,gs) ? get_field(x-1,y,gs)->controller == ctrl : 0))    // W
    ) return fld->fortified ? 205 : 196;
    if(
        ((in_bounds(x,y-1,gs) ? get_field(x,y-1,gs)->controller == ctrl : 0) && // N
        (in_bounds(x+1,y,gs) ? get_field(x+1,y,gs)->controller != ctrl : 0) &&  // E
        (in_bounds(x,y+1,gs) ? get_field(x,y+1,gs)->controller == ctrl : 0) &&  // S
        (in_bounds(x-1,y,gs) ? get_field(x-1,y,gs)->controller != ctrl : 0))    // W
        ||
        ((in_bounds(x,y-1,gs) ? get_field(x,y-1,gs)->controller == ctrl : 0) && // N
        (in_bounds(x+1,y,gs) ? get_field(x+1,y,gs)->controller != ctrl : 0) &&  // E
        (in_bounds(x,y+1,gs) ? get_field(x,y+1,gs)->controller != ctrl : 0) &&  // S
        (in_bounds(x-1,y,gs) ? get_field(x-1,y,gs)->controller != ctrl : 0))    // W
        ||
        ((in_bounds(x,y-1,gs) ? get_field(x,y-1,gs)->controller != ctrl : 0) && // N
        (in_bounds(x+1,y,gs) ? get_field(x+1,y,gs)->controller != ctrl : 0) &&  // E
        (in_bounds(x,y+1,gs) ? get_field(x,y+1,gs)->controller == ctrl : 0) &&  // S
        (in_bounds(x-1,y,gs) ? get_field(x-1,y,gs)->controller != ctrl : 0))    // W
    ) return fld->fortified ? 186 : 179;
    if(
        (in_bounds(x,y-1,gs) ? get_field(x,y-1,gs)->controller == ctrl : 0) && // N
        (in_bounds(x+1,y,gs) ? get_field(x+1,y,gs)->controller == ctrl : 0) && // E
        (in_bounds(x,y+1,gs) ? get_field(x,y+1,gs)->controller != ctrl : 0) && // S
        (in_bounds(x-1,y,gs) ? get_field(x-1,y,gs)->controller != ctrl : 0)    // W
    ) return fld->fortified ? 200 : 192;
    if(
        (in_bounds(x,y-1,gs) ? get_field(x,y-1,gs)->controller != ctrl : 0) && // N
        (in_bounds(x+1,y,gs) ? get_field(x+1,y,gs)->controller != ctrl : 0) && // E
        (in_bounds(x,y+1,gs) ? get_field(x,y+1,gs)->controller == ctrl : 0) && // S
        (in_bounds(x-1,y,gs) ? get_field(x-1,y,gs)->controller == ctrl : 0)    // W
    ) return fld->fortified ? 187 : 191;
    if(
        (in_bounds(x,y-1,gs) ? get_field(x,y-1,gs)->controller != ctrl : 0) && // N
        (in_bounds(x+1,y,gs) ? get_field(x+1,y,gs)->controller == ctrl : 0) && // E
        (in_bounds(x,y+1,gs) ? get_field(x,y+1,gs)->controller == ctrl : 0) && // S
        (in_bounds(x-1,y,gs) ? get_field(x-1,y,gs)->controller != ctrl : 0)    // W
    ) return fld->fortified ? 201 : 218;
    if(
        (in_bounds(x,y-1,gs) ? get_field(x,y-1,gs)->controller == ctrl : 0) && // N
        (in_bounds(x+1,y,gs) ? get_field(x+1,y,gs)->controller != ctrl : 0) && // E
        (in_bounds(x,y+1,gs) ? get_field(x,y+1,gs)->controller != ctrl : 0) && // S
        (in_bounds(x-1,y,gs) ? get_field(x-1,y,gs)->controller == ctrl : 0)    // W
    ) return fld->fortified ? 188 : 217;
}

void print_board(game_state* gs) {
    clear_screen();
    for(int y = 0; y < gs->board_y; (printf("\n"), y++))
    for(int x = 0; x < gs->board_x; x++) {
        printf("%c", get_field_char(x, y, gs));
    }
}