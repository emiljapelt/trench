#ifndef VISUAL_H
#define VISUAL_H
#include "game_state.h"


// ** CHARACTER CODES ** 

#define DESTORYED "\u2217"
#define NONE "\u2022"
#define F_NONE "\u2299"
#define N "\u2575"
#define F_N "\u2579"
#define E "\u2576"
#define F_E "\u257a"
#define S "\u2577"
#define F_S "\u257b"
#define W "\u2574"
#define F_W "\u2578"
#define NS "\u2502"
#define F_NS "\u2551"
#define EW "\u2500"
#define F_EW "\u2550"
#define NE "\u2514"
#define F_NE "\u255a"
#define SE "\u250c"
#define F_SE "\u2554"
#define NW "\u2518"
#define F_NW "\u255d"
#define SW "\u2510"
#define F_SW "\u2557"
#define NES "\u251c"
#define F_NES "\u2560"
#define ESW "\u252c"
#define F_ESW "\u2566"
#define SWN "\u2524"
#define F_SWN "\u2563"
#define WNE "\u2534"
#define F_WNE "\u2569"
#define ALL "\u253c"
#define F_ALL "\u256c"

// ****


static inline void clear_screen(void);
char* get_field_char(int x, int y, game_state* gs);
void print_board(game_state* gs);

#endif