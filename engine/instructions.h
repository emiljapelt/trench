#ifndef H_INSTRUCTIONS
#define H_INSTRUCTIONS

#include <stdlib.h>

#include "game_state.h"
#include "player.h"
#include "visual.h"
#include "util.h"
#include "events.h"
#include "resource_registry.h"

void instr_shoot(player_state* ps) {
    if(!spend_resource("shots", ps->id, 1)) return;
    
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
        for(int p = 0; p < _gs->player_count; p++) {
            if (_gs->players[p].x == x && _gs->players[p].y == y && !get_field(x,y)->type == TRENCH) {
                death_mark_player(_gs->players+p, "Was gunned down");
            }
        }
        move_coord(x, y, d, &x, &y);
    }
    
    //print_board();    
    //sleep(500);
    //x = ps->x;
    //y = ps->y;
    //while (in_bounds(x,y)) {
    //    unset_overlay(x,y);   
    //    move_coord(x, y, d, &x, &y);
    //}
    //print_board();
}

void instr_look(player_state* ps) {
    direction d = (direction)ps->stack[--ps->sp];

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
    if (get_field(x,y)->type == TRENCH) { ps->stack[ps->sp++] = i; return; }
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
    // if (get_field(x,y)->trenched) result |= TRENCH_FLAG;
    // if (get_field(x,y)->destroyed) result |= DESTORYED_FLAG;
    // if (get_field(x,y)->mine) result |= MINE_FLAG;
    for(int i = 0; i < _gs->player_count; i++) 
        if (_gs->players[i].x == x && _gs->players[i].y == y) result |= PLAYER_FLAG;

    end:

    ps->stack[ps->sp++] = result;
}

int mine_event(player_state* ps, void* data) {
    explode_field(ps->x,ps->y);
    set_overlay(ps->x,ps->y,EXPLOSION);
    set_color_overlay(ps->x, ps->y, &color_predefs.red);
    set_mod_overlay(ps->x, ps->y, BOLD);
    return 1;
}

int remove_death_mark(player_state* ps, void* data) {
    ps->death_msg = NULL;
    return 1;
}

void instr_mine(player_state* ps) {
    if(!spend_resource("bombs", ps->id, 1)) return;

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
    //if (get_field(x,y)->destroyed) return;
    // if (get_field(x,y)->mine) {
    //     death_mark_player(ps, "Stepped on a mine");
    //     get_field(x,y)->mine = 0;
    //     _gs->remaining_actions = 0;
    //     set_overlay(x,y,EXPLOSION);
    //     //print_board();
    //     //sleep(250);
    //     //unset_overlay(x,y);
    //     return;
    // }
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
    if(!spend_resource("bombs", ps->id, 1)) return;

    int x = ps->x;
    int y = ps->y;
    for (int i = p; i > 0; i--)
        move_coord(x,y,d,&x,&y);
    //set_overlay(x,y,TARGET);
    //add_bomb(x, y, ps);
    bomb_field(x,y);
    //print_board();
    //sleep(500);
    //unset_overlay(x,y);
    //sleep(250);
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
    int num = *(int*)((ps->directive)+(ps->dp));
    //ps->stack[ps->sp++] = num;
    ps->dp += 4;
    ps->stack[ps->sp++] = v & num;
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