#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "game_state.h"
#include "player.h"
#include "visual.h"
#include "util.h"
#include "color.h"
#include "log.h"

// https://stackoverflow.com/questions/26423537/how-to-position-the-input-text-cursor-in-c
inline void clear_screen(void) {
    printf("\033[H\033[J");
}
inline void reset_cursor(void) {
    printf("\033[0;0H");
}

const char* symbol_lookup[] = {
    "*",
    "\u2219",
    "\u2022",
    "\u2575",
    "\u2579",
    "\u2576",
    "\u257a",
    "\u2577",
    "\u257b",
    "\u2574",
    "\u2578",
    "\u2502",
    "\u2503",
    "\u2500",
    "\u2501",
    "\u2514",
    "\u2517",
    "\u250c",
    "\u250f",
    "\u2518",
    "\u251b",
    "\u2510",
    "\u2513",
    "\u251c",
    "\u2523",
    "\u252c",
    "\u2533",
    "\u2524",
    "\u252b",
    "\u2534",
    "\u253b",
    "\u253c",
    "\u254b",
    "\u1330",
    "\u2311",
    "\u2316",
    "\u2313",
    "\u2620",
    "\u26b0",
    "\u2744",
    "\u219f",
    "\u2359", // "\u22ed"       "\u23c5" "\u26f5" // For some reason these print wrong
    "\u2b24",
    "\u2022",
    "\u27d0",
    "\u26ba",
    "\u26e4",
    "\u2a09",
    "\u00b7",
    "\u21af", // 26a1 this also print a char extra :(
    "\u22ee",
    "\u22ef",
    "\u2302",
    "\u2022",
    "\u205a",
    "\u2056",
    "\u2058",
    "\u2059",
    "\u2612",
    "\u26f0", 
};


// 0xNESW
const int connector_lookup[] = {
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
const int fortified_connector_lookup[] = {
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
    return (color) { .r = r, .g = g, .b = b };
}

int view_buf_max_size;
int view_buf_size;
int view_buf_fill;
char* view_buf;

void buffer(char* format, ...) {
    va_list args;
    char buf[MAX_SYMBOL_SIZE] = {0};

    va_start(args, format);

    vsnprintf(buf, MAX_SYMBOL_SIZE, format, args);

    view_buf_fill += strlen(buf);

    if (view_buf_fill >= view_buf_size) {
        if (view_buf_size >= view_buf_max_size) {
            _log(ERROR, "View buffer overflow");
            return;
        }
        int new_size = 2 * view_buf_size;
        if (new_size > view_buf_max_size) 
            new_size = view_buf_max_size;

        _log(INFO, "Increasing view buffer to: %i", new_size);
        view_buf = realloc(view_buf, new_size);
        view_buf_size = new_size;
    }

    strcat(view_buf, buf);
}

void set_color(color c, color_target ct) {
    char target;
    switch (ct) {
        case FORE: target = '3'; break;
        case BACK: target = '4'; break;
    }

    buffer("\033[%c8;2;%i;%i;%im\0", target, c.r, c.g, c.b);
}

void set_print_mod(print_mod m) {
    buffer("\033[%im\0", m);
}

void reset_print() {
    buffer("\033[m\0"); // same as "\033[0m\0"
}

field_visual empty_visual() {
    return (field_visual){
        .background_color = color_lookup[BLACK],
        .foreground_color = color_lookup[DARK_GREY],
        .mod = 0,
        .symbol = " "
    };
}

void print_board() {
    reset_cursor();
    int feed_index = 0;

    if (view_buf == NULL) {
        view_buf_max_size = 
            _gr->viewport.width + // First line
            _gr->viewport.height + // Newlines
            (2 * _gr->viewport.height + 2 * _gr->viewport.width) + // Border
            (_gr->viewport.height * FEED_WIDTH) + // Feed
            (_gr->viewport.height * _gr->viewport.width * MAX_SYMBOL_SIZE); // Board

        view_buf_size = view_buf_max_size / 10;

        view_buf = malloc(view_buf_size);
    }

    view_buf_fill = 0;
    memset(view_buf, 0, view_buf_size);

    buffer("Round: %i", _gs->round);
    for(int i = 0; i < _gr->viewport.width-5; i++) buffer(" ");
    buffer("\n");

    buffer("%s", symbol_lookup[SE]);
    for(int i = 0; i < _gr->viewport.width+2; i++) buffer("%s", symbol_lookup[EW]);
    buffer("%s\n", symbol_lookup[SW]);
    for(int y = 0; y < _gr->viewport.height; (buffer("\n"), y++)) {
        buffer("%s ", symbol_lookup[NS]);
        field_visual prev_visual = empty_visual();
        for(int x = 0; x < _gr->viewport.width; x++) {
            int actual_x = _gr->viewport.x + x;
            int actual_y = _gr->viewport.y + y;
            field_visual visual = in_bounds(actual_x, actual_y) ? get_field_visual(actual_x, actual_y, fields.get(actual_x, actual_y)) : empty_visual();

            int mod_changed = x == 0 || visual.mod != prev_visual.mod;
            if (mod_changed) {
                if (x != 0) reset_print();
                set_print_mod(visual.mod);
            }

            if (mod_changed || !color_eq(visual.foreground_color, prev_visual.foreground_color))
                set_color(visual.foreground_color, FORE);
            
            if (mod_changed || !color_eq(visual.background_color, prev_visual.background_color))
                set_color(visual.background_color, BACK);
            
            buffer("%s", visual.symbol);
            prev_visual = visual;
        }
        reset_print();
        buffer(" %s ", symbol_lookup[NS]);
        char fi = 0;
        while(fi <= FEED_WIDTH && feed_index < _gs->feed_point) {
            if (_gs->feed_buffer[feed_index] == '\n') {
                feed_index++; break;
            }
            buffer("%c", _gs->feed_buffer[feed_index]);
            feed_index++;
            fi++;
        } 
    }
    buffer("%s", symbol_lookup[NE]);
    for(int i = 0; i < _gr->viewport.width+2; i++) buffer("%s", symbol_lookup[EW]);
    buffer("%s\n", symbol_lookup[NW]);

    puts(view_buf);

    clear_feed();
}