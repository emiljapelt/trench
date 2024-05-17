#include <stdio.h>

#include "game_state.h"
#include "player.h"
#include "visual.h"

static inline void clear_screen(void) {
    // use cursor movements instead, avoids flickering
    // https://stackoverflow.com/questions/26423537/how-to-position-the-input-text-cursor-in-c
    //printf("\e[1;1H\e[2J");
    printf("\033[H\033[J");
    //printf("\033[%d;%dH", (0), (0));
}

static inline char trench_connects(int x, int y, game_state* gs) {
    return (in_bounds(x,y,gs) ? get_field(x,y,gs)->trenched : 0);
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

const char* get_field_char(int x, int y, game_state* gs) {
    field_state *fld = get_field(x,y,gs);

    if(fld->target) return TARGET;
    if(fld->explosion) return EXPLOSION;
    if(fld->destroyed) return DESTORYED;

    if(!fld->trenched) { 
        for(int i = 0; i < gs->player_count; i++) {
            if (gs->players[i].x == x && gs->players[i].y == y && gs->players[i].alive) return PERSON;
        }

        if(fld->bullet_state) {
            if (fld->bullet_state == NS_bullets) return BULLETS_NS;
            if (fld->bullet_state == EW_bullets) return BULLETS_EW;
        }

        return " "; 
    }

    int char_idx = 
        (trench_connects(x,y-1,gs) << 3) | // N
        (trench_connects(x+1,y,gs) << 2) | // E
        (trench_connects(x,y+1,gs) << 1) | // S
        trench_connects(x-1,y,gs);         // W 

    return (fld->fortified) ? f_char_lookup[char_idx] : char_lookup[char_idx];
}

void print_board(game_state* gs) {
    clear_screen();
    printf("Round: %i\n", gs->round);
    for(int y = 0; y < gs->board_y; (printf("\n"), y++))
    for(int x = 0; x < gs->board_x; x++) {
        printf("%s", get_field_char(x, y, gs));
    }
}