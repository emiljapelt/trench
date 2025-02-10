#ifndef VISUAL_H
#define VISUAL_H

#include "game_state.h"
#include "color.h"
#include "fields.h"

// ** CHARACTER CODES ** 
#define DESTORYED "*"
#define NONE "\u2219"
#define F_NONE "\u2022"
#define N "\u2575"
#define F_N "\u2579"
#define E "\u2576"
#define F_E "\u257a"
#define S "\u2577"
#define F_S "\u257b"
#define W "\u2574"
#define F_W "\u2578"
#define NS "\u2502"
#define F_NS "\u2503"
#define EW "\u2500"
#define F_EW "\u2501"
#define NE "\u2514"
#define F_NE "\u2517"
#define SE "\u250c"
#define F_SE "\u250f"
#define NW "\u2518"
#define F_NW "\u251b"
#define SW "\u2510"
#define F_SW "\u2513"
#define NES "\u251c"
#define F_NES "\u2523"
#define ESW "\u252c"
#define F_ESW "\u2533"
#define SWN "\u2524"
#define F_SWN "\u252b"
#define WNE "\u2534"
#define F_WNE "\u253b"
#define ALL "\u253c"
#define F_ALL "\u254b"

#define PERSON "\u1330"

#define EXPLOSION "\u2311"
#define TARGET "\u2316"
#define MINE "\u2313"
#define SKULL "\u2620"
#define COFFIN "\u26b0"
#define SNOWFLAKE "\u2744"
#define LIGHTNING "\u2607"

#define BULLETS_NS "\u22ee"
#define BULLETS_EW "\u2026"

// ****

typedef enum {
    FORE,
    BACK,
} color_target;

typedef struct field_visual {
    color* foreground_color;
    color* background_color;
    char* symbol;
    print_mod mod;
} field_visual;

color rgb_color(int r, int g, int b);
void clear_screen(void);
void reset_cursor(void);
field_visual get_field_visual(const int x, const int y, const field_state* field);
void print_board();
extern const color_predef color_predefs;

#endif