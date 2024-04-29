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

static inline char connects(int x, int y, char ctrl, game_state* gs) {
    return (in_bounds(x,y,gs) ? get_field(x,y,gs)->controller == ctrl : 0);
}

// 0xNESW
const char* char_lookup[] = {
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
const char* f_char_lookup[] = {
    NONE, // 0x0000
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

const char* get_field_char(int x, int y, game_state* gs) {
    if (get_field(x,y,gs)->destroyed) return DESTORYED;
    field_state *fld = get_field(x,y,gs);
    char ctrl = fld->controller;

    if(ctrl == 0) return " ";
    int char_idx = 
        (connects(x,y-1,ctrl,gs) << 3) | // N
        (connects(x+1,y,ctrl,gs) << 2) | // E
        (connects(x,y+1,ctrl,gs) << 1) | // S
        connects(x-1,y,ctrl,gs);         // W 

    if (fld->fortified) return f_char_lookup[char_idx];
    else return char_lookup[char_idx];
}

void print_board(game_state* gs) {
    clear_screen();
    for(int y = 0; y < gs->board_y; (printf("\n"), y++))
    for(int x = 0; x < gs->board_x; x++) {
        printf("%s", get_field_char(x, y, gs));
    }
}