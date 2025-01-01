#ifndef H_INSTRUCTIONS
#define H_INSTRUCTIONS

#include <stdlib.h>

#include "game_state.h"
#include "player.h"
#include "visual.h"
#include "util.h"
#include "event_list.h"
#include "events.h"
#include "resource_registry.h"
#include "field_scan.h"

void instr_shoot(player_state* ps) {
    if(!spend_resource("shot", ps->id, 1)) return;
    
    direction d = (direction)ps->stack[--ps->sp];
    int x = ps->x;
    int y = ps->y;
    const char* visual; switch (d) {
        case NORTH:
        case SOUTH: 
            visual = BULLETS_NS;
            break;
        case EAST:
        case WEST:
            visual = BULLETS_EW;
            break;
    }
    move_coord(x, y, d, &x, &y);
    while (in_bounds(x,y)) { 
        set_overlay(x,y,visual);
        set_color_overlay(x,y,FORE,color_predefs.yellow);
        for(int p = 0; p < _gs->player_count; p++) {
            if (_gs->players[p].x == x && _gs->players[p].y == y && get_field(x,y)->type != TRENCH) {
                death_mark_player(_gs->players+p, "Was gunned down");
            }
        }
        move_coord(x, y, d, &x, &y);
    }
}

void instr_look(player_state* ps) {
    direction d = (direction)ps->stack[--ps->sp];
    int offset = *(int*)((ps->directive)+(ps->dp));
    ps->dp += 4;

    int x = ps->x;
    int y = ps->y;
    int i = 0;
    incr:
    i++;
    switch (d) {
        case NORTH: y--; break;
        case EAST: x++; break;
        case SOUTH: y++; break;
        case WEST: x--; break;
    }
    if (!in_bounds(x,y)) { ps->stack[ps->sp++] = 0; return; }
    field_scan scan = scan_field(x,y);
    field_scan* bypass = &scan;
    if ((*(int*)bypass) & (1 << offset)) { ps->stack[ps->sp++] = i; return; }
    goto incr;
}

void instr_scan(player_state* ps) {
    int x = ps->x;
    int y = ps->y;
    direction power = (direction)ps->stack[--ps->sp];
    direction d = (direction)ps->stack[--ps->sp];

    int result = 0;
    switch (d) {
        case NORTH: y -= power; break;
        case EAST: x += power; break;
        case SOUTH: y += power; break;
        case WEST: x -= power; break;
    }
    if (!in_bounds(x, y)) goto end;

    field_scan scan = scan_field(x,y);
    field_scan* bypass = &scan;
    result = *(int*)bypass;

    end:
    ps->stack[ps->sp++] = result;
}

void instr_mine(player_state* ps) {
    if(!spend_resource("bomb", ps->id, 1)) return;

    int x, y;
    direction d = (direction)ps->stack[--ps->sp];
    move_coord(ps->x, ps->y, d, &x, &y);
    char kill = 0;
    for(int i = 0; i < _gs->player_count; i++) 
        if (_gs->players[i].x == x && _gs->players[i].y == y) { death_mark_player(_gs->players+i, "Hit by a thrown mine"); kill = 1; }
    if (!kill) {
        if (!in_bounds(x,y)) return;
        set_overlay(x,y,MINE);
        add_event(
            get_field(x,y)->exit_events,
            &mine_event, NULL
        );
    }
}

void instr_move(player_state* ps) { 
    direction d = (direction)ps->stack[--ps->sp];

    int x, y;
    move_coord(ps->x, ps->y, d, &x, &y);
    if (!in_bounds(x, y)) return;

    update_events(ps, get_field(ps->x,ps->y)->exit_events);
    if (!ps->death_msg) {
        ps->x = x;
        ps->y = y;
        update_events(ps, get_field(x,y)->enter_events);
    }
}

void instr_melee(player_state* ps) {
    int x, y;
    direction d = (direction)ps->stack[--ps->sp];
    move_coord(ps->x, ps->y, d, &x, &y);
    for(int i = 0; i < _gs->player_count; i++)
        if (_gs->players[i].x == x && _gs->players[i].y == y) 
            death_mark_player(_gs->players+i, "Lost a fist fight");
    //move(d,ps);
}

void instr_trench(player_state* ps) {
    int x, y;
    direction d = (direction)ps->stack[--ps->sp];
    move_coord(ps->x, ps->y, d, &x, &y);
    if (!in_bounds(x, y)) return;

    field_state* field = get_field(x,y);
    switch (field->type) {
        case EMPTY: {
            build_field(x,y);
        }
    }
}

void instr_fortify(player_state* ps) {
    int x, y;
    direction d = (direction)ps->stack[--ps->sp];
    move_coord(ps->x, ps->y, d, &x, &y);
    fortify_field(x,y);
}

void instr_random_int(player_state* ps) { 
    ps->stack[ps->sp++] = rand();
}

void instr_random_range(player_state* ps) {
    int num = *(int*)((ps->directive)+(ps->dp));
    int pick = ps->stack[ps->sp - ((rand() % num)+1)];
    ps->sp -= num;
    ps->stack[ps->sp++] = pick;
    ps->dp += 4;
}

void instr_place(player_state* ps) {
    int num = *(int*)((ps->directive)+(ps->dp));
    ps->stack[ps->sp++] = num;
    ps->dp += 4;
}

void instr_bomb(player_state* ps) {
    int p = ps->stack[--ps->sp];
    direction d = (direction)ps->stack[--ps->sp];
    if(!spend_resource("bomb", ps->id, 1)) return;

    int x = ps->x;
    int y = ps->y;
    for (int i = p; i > 0; i--)
        move_coord(x,y,d,&x,&y);

    bomb_event_args* args = malloc(sizeof(bomb_event_args));
    args->x = x;
    args->y = y;
    args->player_id = ps->id;
    add_event(_gs->events, &bomb_event, args);
    set_color_overlay(x,y,FORE,color_predefs.red);
    set_overlay(x,y,TARGET);
}

void instr_global_access(player_state* ps) {
    int i = ps->stack[--ps->sp];
    int t = ps->stack[--ps->sp];
    if (i > 0 && i < _gr->array) {
        ps->stack[ps->sp++] = _gs->global_arrays[(t*_gr->array)+i];
    } else ps->stack[ps->sp++] = 0;
}

void instr_access(player_state* ps) {
    switch (ps->directive[ps->dp]) {
        case 'x': { // Player x position
            ps->stack[ps->sp++] = ps->x;
            ps->dp++;
            break;
        }
        case 'y': { // Player y position
            ps->stack[ps->sp++] = ps->y;
            ps->dp++;
            break;
        }
        case 'r': { // Player resources
            ps->dp++;
            int index = *(int*)((ps->directive)+(ps->dp));
            ps->dp += 4;
            ps->stack[ps->sp++] = peek_resource_index(index, ps->id);
            break;
        }
        case '_': { // Board x size
            ps->stack[ps->sp++] = _gs->board_x;
            ps->dp++;
            break;
        }
        case '|': { // Board y size
            ps->stack[ps->sp++] = _gs->board_y;
            ps->dp++;
            break;
        }
        case 'g': { // Global array size
            ps->stack[ps->sp++] = _gr->array;
            ps->dp++;
            break;
        }
        case 'v': { // Variable access
            int num = ps->stack[ps->sp-1];
            ps->stack[ps->sp-1] = ps->stack[num];
            ps->dp++;
            break;
        }
    }
}

void instr_goto(player_state* ps) {
    int num = *(int*)((ps->directive)+(ps->dp));
    ps->dp = num;
}

void instr_goto_if(player_state* ps) {
    int v = ps->stack[--ps->sp];
    if (v) { 
        int num = *(int*)((ps->directive)+(ps->dp));
        ps->dp = num; 
    }
    else ps->dp += 4;
}

void instr_eq(player_state* ps) {
    int v0 = ps->stack[--ps->sp];
    int v1 = ps->stack[--ps->sp];
    ps->stack[ps->sp++] = v0 == v1;
}

void instr_lt(player_state* ps) {
    int v0 = ps->stack[--ps->sp];
    int v1 = ps->stack[--ps->sp];
    ps->stack[ps->sp++] = v0 < v1;
}

void instr_sub(player_state* ps) {
    int v0 = ps->stack[--ps->sp];
    int v1 = ps->stack[--ps->sp];
    ps->stack[ps->sp++] = v1 - v0;
}

void instr_add(player_state* ps) {
    int v0 = ps->stack[--ps->sp];
    int v1 = ps->stack[--ps->sp];
    ps->stack[ps->sp++] = v0 + v1;
}

void instr_mul(player_state* ps) {
    int v0 = ps->stack[--ps->sp];
    int v1 = ps->stack[--ps->sp];
    ps->stack[ps->sp++] = v0 * v1;
}

void instr_div(player_state* ps) { 
    int v0 = ps->stack[--ps->sp];
    int v1 = ps->stack[--ps->sp];
    if (v1 == 0) { death_mark_player(ps, "Mental break down (div by 0)"); return; }
    ps->stack[ps->sp++] = v0 / v1;
}

void instr_mod(player_state* ps) {
    int v0 = ps->stack[--ps->sp];
    int v1 = ps->stack[--ps->sp];
    if (v1 == 0) { death_mark_player(ps, "Mental break down (div by 0)"); return; }
    ps->stack[ps->sp++] = ((v0%v1) + v1)%v1;
}

void instr_not(player_state* ps) {
    int v = ps->stack[--ps->sp];
    ps->stack[ps->sp++] = !v;
}

void instr_or(player_state* ps) {
    int v0 = ps->stack[--ps->sp];
    int v1 = ps->stack[--ps->sp];
    ps->stack[ps->sp++] = v0 || v1;
}

void instr_and(player_state* ps) {
    int v0 = ps->stack[--ps->sp];
    int v1 = ps->stack[--ps->sp];
    ps->stack[ps->sp++] = v0 && v1;
}

void instr_assign(player_state* ps) { 
    switch (ps->directive[ps->dp++]) {
        case '_': { // Local
            int v = ps->stack[--ps->sp];
            int target = ps->stack[--ps->sp];
            ps->stack[target] = v;
            break;
        }
        case '@': { // Global
            int v = ps->stack[--ps->sp];
            int i = ps->stack[--ps->sp];
            int t = ps->stack[--ps->sp];
            if (i > 0 && i < _gr->array) _gs->global_arrays[(t*_gr->array)+i] = v;
            break;
        }
    }
}

void instr_flag_access(player_state* ps) { 
    int v = ps->stack[--ps->sp];
    int offset = *(int*)((ps->directive)+(ps->dp));
    ps->dp += 4;
    ps->stack[ps->sp++] = v & (1 << offset);
}

void instr_dec_stack(player_state* ps) {
    ps->sp--;
}

void instr_clone(player_state* ps) {
    ps->stack[ps->sp] = ps->stack[ps->sp - 1];
    ps->sp++;
}

void instr_swap(player_state* ps) {
    int v0 = ps->stack[--ps->sp];
    int v1 = ps->stack[--ps->sp];
    ps->stack[ps->sp++] = v0;
    ps->stack[ps->sp++] = v1;
}

#endif