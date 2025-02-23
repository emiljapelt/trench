#include <stdio.h>
#include <stdlib.h>

#include "game_state.h"
#include "player.h"
#include "visual.h"
#include "util.h"
#include "color.h"

// https://stackoverflow.com/questions/26423537/how-to-position-the-input-text-cursor-in-c
inline void clear_screen(void) {
    printf("\033[H\033[J");
}
inline void reset_cursor(void) {
    printf("\033[0;0H");
}

static inline char trench_connects(int x, int y) {
    return (!in_bounds(x,y)) 
        ? 0 
        : get_field(x,y)->type == TRENCH;
}

// 0xNESW
char* char_lookup[] = {
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
char* f_char_lookup[] = {
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


color rgb_color(int r, int g, int b) {
    return (color) { .r = r, .g = g, .b = b, .predef = 0 };
}

void set_color(color c, color_target ct) {
    char target;
    switch (ct) {
        case FORE: target = '3'; break;
        case BACK: target = '4'; break;
    }

    printf("\033[%c8;2;%i;%i;%im", target, c.r, c.g, c.b);
}

void set_print_mod(print_mod m) {
    printf("\033[%im", m);
}

void reset_print() {
    printf("\033[0m");
}

field_visual get_field_data_visual(const int x, const int y, const field_type type, const field_data* data) {
    field_visual result = {
        .background_color = NULL,
        .foreground_color = color_predefs.dark_grey,
        .mod = 0,
        .symbol = MIDDOT
    };

    switch (type) {
        case TRENCH: {
            int char_idx = 
                (trench_connects(x,y-1) << 3) | // N
                (trench_connects(x+1,y) << 2) | // E
                (trench_connects(x,y+1) << 1) | // S
                trench_connects(x-1,y);         // W 

            result.symbol = (data->trench.fortified) ? f_char_lookup[char_idx] : char_lookup[char_idx];
            result.foreground_color = color_predefs.white;
            break;
        }
        case ICE_BLOCK: {
            result.background_color = color_predefs.ice_blue;
            result.foreground_color = color_predefs.white;
            result.symbol = get_field_data_visual(x, y, data->ice_block.inner_type, data->ice_block.inner).symbol;
            break;
        }
        case EMPTY: {
            for(int i = 0; i < _gs->players->count; i++) {
                player_state* player = get_player(_gs->players, i);
                if (player->x == x && player->y == y && player->alive) {
                    result.symbol = PERSON;
                    if (player->team) 
                        result.foreground_color = player->team->color;
                    else 
                        result.foreground_color = color_predefs.white;
                }
            }
            break;
        }
    }
    return result;
}

field_visual get_field_visual(const int x, const int y, const field_state* field) {

    field_visual result = get_field_data_visual(x,y,field->type,field->data);

    if (_gs->board[(y * _gs->board_x) + x].symbol_overlay) {
        char* symbol = _gs->board[(y * _gs->board_x) + x].symbol_overlay;
        _gs->board[(y * _gs->board_x) + x].symbol_overlay = NULL;
        result.symbol = symbol;
    }
    if (_gs->board[(y * _gs->board_x) + x].foreground_color_overlay) {
        color* color = _gs->board[(y * _gs->board_x) + x].foreground_color_overlay;
        _gs->board[(y * _gs->board_x) + x].foreground_color_overlay = NULL;
        result.foreground_color = color;
    }
    if (_gs->board[(y * _gs->board_x) + x].background_color_overlay) {
        color* color = _gs->board[(y * _gs->board_x) + x].background_color_overlay;
        _gs->board[(y * _gs->board_x) + x].background_color_overlay = NULL;
        result.background_color = color;
    }

    return result; 
}

void print_board() {
    reset_cursor();
    printf("Round: %i", _gs->round);
    for(int i = 0; i < _gs->board_x-6; i++) putchar(' ');
    putchar('\n');
    printf("%s", SE);
    for(int i = 0; i < _gs->board_x; i++) printf("%s", EW);
    printf("%s\n", SW);
    for(int y = 0; y < _gs->board_y; (putchar('\n'), y++)) {
        printf("%s", NS);
        for(int x = 0; x < _gs->board_x; x++) {
            field_visual visual = get_field_visual(x,y,get_field(x,y));

            if (visual.foreground_color) {
                set_color(*visual.foreground_color, FORE);
                if (!visual.foreground_color->predef)
                    free(visual.foreground_color);
            }

            if (visual.background_color) {
                set_color(*visual.background_color, BACK);
                if (!visual.background_color->predef) 
                    free(visual.background_color);
            }

            if (visual.mod)
                set_print_mod(visual.mod);

            printf("%s", visual.symbol);
            reset_print();
        }
        printf("%s", NS);
    }
    printf("%s", NE);
    for(int i = 0; i < _gs->board_x; i++) printf("%s", EW);
    printf("%s\n", NW);
    for(int i = 0; i < _gs->feed_point; i++) putchar(_gs->feed_buffer[i]);
    clear_feed();
}