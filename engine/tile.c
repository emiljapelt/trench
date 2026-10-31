#include "tile.h"

#include <stdlib.h>

#include "color.h"
#include "damage.h"

void push_tile(tile_stack* s, tile* t) {
    array_list.add(s,t);
}

tile* pop_tile(tile_stack* s) {
    if (s->count == 0) return NULL;
    return array_list.remove(s, s->count - 1, 0);
}

tile* peek_tile(tile_stack* s) {
    if (s->count == 0) return NULL;
    return array_list.get(s, s->count - 1);
}

tile* create_tile(const tile_template* template) {
    tile* t = malloc(sizeof(tile));
    t->template = template;
    return t;
}

unsigned int get_tile_properties(tile_stack* stack) {
    unsigned int props = 0;
    int index = stack->count - 1;
    while(index >= 0) {
        tile* t = ((tile*)array_list.get(stack, index));
        props |= t->template->properties;
        if (t->template->transparent_properties) index--;
        else break;
    }
    return props;
}

symbol get_tile_symbol(tile_stack* stack) {
    int index = stack->count - 1;
    while(index >= 0) {
        tile* t = ((tile*)array_list.get(stack, index));
        if (t->template->symbol.type == NO_SYMBOL) index--;
        else return t->template->symbol;
    }
    return (symbol) { .type = NO_SYMBOL };
}

color get_tile_fg(tile_stack* stack) {
    int index = stack->count - 1;
    while(index >= 0) {
        tile* t = ((tile*)array_list.get(stack, index));
        if (t->template->transparent_fg) index--;
        else return t->template->fg;
    }
    return color_lookup[DARK_GREY];
}

color get_tile_bg(tile_stack* stack) {
    int index = stack->count - 1;
    while(index >= 0) {
        tile* t = ((tile*)array_list.get(stack, index));
        if (t->template->transparent_bg) index--;
        else return t->template->bg;
    }
    return color_lookup[BLACK];
}

unsigned int get_tile_mod(tile_stack* stack) {
    int index = stack->count - 1;
    while(index >= 0) {
        tile* t = ((tile*)array_list.get(stack, index));
        if (t->template->transparent_mod) index--;
        else return t->template->mod;
    }
    return 0;
}

const int tile_template_count = 3;
const tile_template tile_template_lookup[] = 
{
    {// Trench
        .properties = PROP_IS_TRENCH | PROP_COVER,
        .hardend = 0,
        .symbol = {
            .type = CONNECTOR,
            .connector = PROP_IS_TRENCH,
        },
        .fg = {
            .r = 255, .g = 255, .b = 255,
        },
        .weaknesses = 0,
        .trm = '+',
        .transparent_properties = 0,
    },
    {// Tree
        .properties = PROP_IS_TREE | PROP_OBSTRUCTION,
        .hardend = 0,
        .symbol = {
            .type = SYMBOL,
            .symbol = 0x219f,
        },
        .fg = {
            .r = 0, .g = 255, .b = 0,
        },
        .weaknesses = FIRE_DMG,
        .trm = 'T',
        .transparent_properties = 0,
    },
    {// Fortification
        .properties = PROP_SHELTER,
        .transparent_properties = 1,
        .transparent_bg = 1,
        .transparent_fg = 1,
        .hardend = 1,
        .symbol = {
            .type = NO_SYMBOL,
        },
        .transparent_mod = 0,
        .mod = 1, // BOLD
        .weaknesses = 0,
        .trm = 0,
    },
};