#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "visual.h"

#include "game_state.h"
#include "player.h"
#include "util.h"
#include "color.h"
#include "log.h"
#include "symbols.h"


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

void set_color(color c, color_target t) {
    buffer(_TERM_PRINT_COLOR, t, c.r, c.g, c.b);
}

void set_print_mod(print_mod m) {
    buffer(_TERM_PRINT_MOD, m);
}

void reset_print(void) {
    buffer(_TERM_PRINT_RESET);
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
    clear_screen();
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
        for(int i = 0; i < FEED_WIDTH; i++) {
            buffer("%c", _gs->feed[y * FEED_WIDTH + i]);
        }
    }
    buffer("%s", symbol_lookup[NE]);
    for(int i = 0; i < _gr->viewport.width+2; i++) buffer("%s", symbol_lookup[EW]);
    buffer("%s\n", symbol_lookup[NW]);

    puts(view_buf);

    _gs->feed_change = 0;
    _gs->latest_print = _gs->round;
}