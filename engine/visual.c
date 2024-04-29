#include <stdio.h>

#include "game_state.h"
#include "visual.h"

static inline void clear_screen(void) {
    // use cursor movements instead, avoids flickering
    // https://stackoverflow.com/questions/26423537/how-to-position-the-input-text-cursor-in-c
    //printf("\e[1;1H\e[2J");
    printf("\033[H\033[J");
    //printf("\033[%d;%dH", (0), (0));
}

char* get_field_char(int x, int y, game_state* gs) {
    if (get_field(x,y,gs)->destroyed) return DESTORYED;
    field_state *fld = get_field(x,y,gs);
    char ctrl = fld->controller;
    if(ctrl == 0) return " ";
    if(
        (in_bounds(x,y-1,gs) ? get_field(x,y-1,gs)->controller == ctrl : 0) && // N
        (in_bounds(x+1,y,gs) ? get_field(x+1,y,gs)->controller == ctrl : 0) && // E
        (in_bounds(x,y+1,gs) ? get_field(x,y+1,gs)->controller == ctrl : 0) && // S
        (in_bounds(x-1,y,gs) ? get_field(x-1,y,gs)->controller == ctrl : 0)    // W
    ) return fld->fortified ? F_ALL : ALL;
    if(
        (in_bounds(x,y-1,gs) ? get_field(x,y-1,gs)->controller == ctrl : 0) && // N
        (in_bounds(x+1,y,gs) ? get_field(x+1,y,gs)->controller == ctrl : 0) && // E
        (in_bounds(x,y+1,gs) ? get_field(x,y+1,gs)->controller != ctrl : 1) && // S
        (in_bounds(x-1,y,gs) ? get_field(x-1,y,gs)->controller == ctrl : 0)    // W
    ) return fld->fortified ? F_WNE : WNE;
    if(
        (in_bounds(x,y-1,gs) ? get_field(x,y-1,gs)->controller != ctrl : 1) && // N
        (in_bounds(x+1,y,gs) ? get_field(x+1,y,gs)->controller == ctrl : 0) && // E
        (in_bounds(x,y+1,gs) ? get_field(x,y+1,gs)->controller == ctrl : 0) && // S
        (in_bounds(x-1,y,gs) ? get_field(x-1,y,gs)->controller == ctrl : 0)    // W
    ) return fld->fortified ? F_ESW : ESW;
    if(
        (in_bounds(x,y-1,gs) ? get_field(x,y-1,gs)->controller == ctrl : 0) && // N
        (in_bounds(x+1,y,gs) ? get_field(x+1,y,gs)->controller != ctrl : 1) && // E
        (in_bounds(x,y+1,gs) ? get_field(x,y+1,gs)->controller == ctrl : 0) && // S
        (in_bounds(x-1,y,gs) ? get_field(x-1,y,gs)->controller == ctrl : 0)    // W
    ) return fld->fortified ? F_SWN : SWN;
    if(
        (in_bounds(x,y-1,gs) ? get_field(x,y-1,gs)->controller != ctrl : 1) && // N
        (in_bounds(x+1,y,gs) ? get_field(x+1,y,gs)->controller == ctrl : 0) &&  // E
        (in_bounds(x,y+1,gs) ? get_field(x,y+1,gs)->controller != ctrl : 1) &&  // S
        (in_bounds(x-1,y,gs) ? get_field(x-1,y,gs)->controller == ctrl : 0)    // W
    ) return fld->fortified ? F_EW : EW;
    if(
        (in_bounds(x,y-1,gs) ? get_field(x,y-1,gs)->controller != ctrl : 1) && // N
        (in_bounds(x+1,y,gs) ? get_field(x+1,y,gs)->controller == ctrl : 0) &&  // E
        (in_bounds(x,y+1,gs) ? get_field(x,y+1,gs)->controller != ctrl : 1) &&  // S
        (in_bounds(x-1,y,gs) ? get_field(x-1,y,gs)->controller != ctrl : 1)    // W
    ) return fld->fortified ? F_E : E;
    if(
        (in_bounds(x,y-1,gs) ? get_field(x,y-1,gs)->controller != ctrl : 1) && // N
        (in_bounds(x+1,y,gs) ? get_field(x+1,y,gs)->controller != ctrl : 1) &&  // E
        (in_bounds(x,y+1,gs) ? get_field(x,y+1,gs)->controller != ctrl : 1) &&  // S
        (in_bounds(x-1,y,gs) ? get_field(x-1,y,gs)->controller == ctrl : 0)    // W
    ) return fld->fortified ? F_W : W;
    if(
        (in_bounds(x,y-1,gs) ? get_field(x,y-1,gs)->controller == ctrl : 0) && // N
        (in_bounds(x+1,y,gs) ? get_field(x+1,y,gs)->controller != ctrl : 1) &&  // E
        (in_bounds(x,y+1,gs) ? get_field(x,y+1,gs)->controller == ctrl : 0) &&  // S
        (in_bounds(x-1,y,gs) ? get_field(x-1,y,gs)->controller != ctrl : 1)    // W
    ) return fld->fortified ? F_NS : NS;
    if(
        (in_bounds(x,y-1,gs) ? get_field(x,y-1,gs)->controller == ctrl : 0) && // N
        (in_bounds(x+1,y,gs) ? get_field(x+1,y,gs)->controller != ctrl : 1) &&  // E
        (in_bounds(x,y+1,gs) ? get_field(x,y+1,gs)->controller != ctrl : 1) &&  // S
        (in_bounds(x-1,y,gs) ? get_field(x-1,y,gs)->controller != ctrl : 1)    // W
    ) return fld->fortified ? F_N : N;
    if(
        (in_bounds(x,y-1,gs) ? get_field(x,y-1,gs)->controller != ctrl : 1) && // N
        (in_bounds(x+1,y,gs) ? get_field(x+1,y,gs)->controller != ctrl : 1) &&  // E
        (in_bounds(x,y+1,gs) ? get_field(x,y+1,gs)->controller == ctrl : 0) &&  // S
        (in_bounds(x-1,y,gs) ? get_field(x-1,y,gs)->controller != ctrl : 1)    // W
    ) return fld->fortified ? F_S : S;
    if(
        (in_bounds(x,y-1,gs) ? get_field(x,y-1,gs)->controller == ctrl : 0) && // N
        (in_bounds(x+1,y,gs) ? get_field(x+1,y,gs)->controller == ctrl : 0) && // E
        (in_bounds(x,y+1,gs) ? get_field(x,y+1,gs)->controller != ctrl : 1) && // S
        (in_bounds(x-1,y,gs) ? get_field(x-1,y,gs)->controller != ctrl : 1)    // W
    ) return fld->fortified ? F_NE : NE;
    if(
        (in_bounds(x,y-1,gs) ? get_field(x,y-1,gs)->controller != ctrl : 1) && // N
        (in_bounds(x+1,y,gs) ? get_field(x+1,y,gs)->controller != ctrl : 1) && // E
        (in_bounds(x,y+1,gs) ? get_field(x,y+1,gs)->controller == ctrl : 0) && // S
        (in_bounds(x-1,y,gs) ? get_field(x-1,y,gs)->controller == ctrl : 0)    // W
    ) return fld->fortified ? F_SW : SW;
    if(
        (in_bounds(x,y-1,gs) ? get_field(x,y-1,gs)->controller != ctrl : 1) && // N
        (in_bounds(x+1,y,gs) ? get_field(x+1,y,gs)->controller == ctrl : 0) && // E
        (in_bounds(x,y+1,gs) ? get_field(x,y+1,gs)->controller == ctrl : 0) && // S
        (in_bounds(x-1,y,gs) ? get_field(x-1,y,gs)->controller != ctrl : 1)    // W
    ) return fld->fortified ? F_SE : SE;
    if(
        (in_bounds(x,y-1,gs) ? get_field(x,y-1,gs)->controller == ctrl : 0) && // N
        (in_bounds(x+1,y,gs) ? get_field(x+1,y,gs)->controller != ctrl : 1) && // E
        (in_bounds(x,y+1,gs) ? get_field(x,y+1,gs)->controller != ctrl : 1) && // S
        (in_bounds(x-1,y,gs) ? get_field(x-1,y,gs)->controller == ctrl : 0)    // W
    ) return fld->fortified ? F_NW : NW;

    return fld->fortified ? F_NONE : NONE;
}

void print_board(game_state* gs) {
    clear_screen();
    for(int y = 0; y < gs->board_y; (printf("\n"), y++))
    for(int x = 0; x < gs->board_x; x++) {
        printf("%s", get_field_char(x, y, gs));
    }
}