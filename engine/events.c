#include "events.h"

#include "game_state.h"
#include "visual.h"
#include "player.h"

int mine_event(player_state* ps, void* data) {
    explode_field(ps->x,ps->y);
    set_overlay(ps->x,ps->y,EXPLOSION);
    set_color_overlay(ps->x, ps->y, FORE, color_predefs.red);
    set_mod_overlay(ps->x, ps->y, BOLD);
    return 1;
}

int bomb_event(player_state* ps, void* data) {
    bomb_event_args* args = (bomb_event_args*)data;
    if (args->player_id != ps->id) return 0;
    bomb_field(args->x,args->y);
    return 1;
}