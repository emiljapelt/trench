#ifndef H_INSTRUCTIONS
#define H_INSTRUCTIONS

#include <stdlib.h>
#include <string.h>

#include "game_state.h"
#include "player.h"
#include "visual.h"
#include "util.h"
#include "event_list.h"
#include "events.h"
#include "resource_registry.h"
#include "field_scan.h"

typedef enum {
  Meta_PlayerX = 0,
  Meta_PlayerY = 1,
  Meta_BoardX = 2,
  Meta_BoardY = 3,
  Meta_Resource = 4,
  Meta_PlayerID = 42,
  Instr_Add = 6,
  Instr_Sub = 7,
  Instr_Mul = 8,
  Instr_And = 9,
  Instr_Or = 10,
  Instr_Eq = 11,
  Instr_Not = 12,
  Instr_Lt = 13,
  Instr_Div = 14,
  Instr_Mod = 15,
  Instr_Scan = 16,
  Instr_Random = 17,
  Instr_RandomSet = 18,
  Instr_Place = 19,
  Instr_Access = 20,
  Instr_Swap = 21,
  Instr_Copy = 22,
  Instr_DecStack = 23,
  Instr_FieldFlag = 24,
  Instr_Assign = 25,
  Instr_GoToIf = 26,
  Instr_GoTo = 27,
  Instr_Move = 28,
  Instr_Attack = 29,
  Instr_Trench = 30,
  Instr_Fortify = 31,
  Instr_Bomb = 32,
  Instr_Shoot = 33,
  Instr_Wait = 34,
  Instr_Pass = 35,
  Instr_Look = 36,
  Instr_Mine = 37,
  Instr_Melee = 38,
  Instr_Read = 39,
  Instr_Write = 40,
  Instr_Projection = 41,
} instruction;

void instr_shoot(player_state* ps) {
    if(!spend_resource(ps->resources, "shot", 1)) return;
    
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

        linked_list_node* player_node = _gs->players->list;
        while(player_node) {
            player_state* player = (player_state*)player_node->data;
            if (player->x == x && player->y == y && get_field(x,y)->type != TRENCH) {
                death_mark_player(player, "Was gunned down");
            }
            player_node = player_node->next;
        }

        move_coord(x, y, d, &x, &y);
    }
}

void instr_look(player_state* ps) {
    direction d = (direction)ps->stack[--ps->sp];
    int offset = ps->directive[ps->dp];
    ps->dp++;

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
    if(!spend_resource(ps->resources, "bomb", 1)) return;

    int x, y;
    direction d = (direction)ps->stack[--ps->sp];
    move_coord(ps->x, ps->y, d, &x, &y);
    char kill = 0;

    linked_list_node* player_node = _gs->players->list;
    while(player_node) {
        player_state* player = (player_state*)player_node->data;
        if (player->x == x && player->y == y) { 
            death_mark_player(player, "Hit by a thrown mine"); 
            kill = 1; 
        }
        player_node = player_node->next;
    }

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

    linked_list_node* player_node = _gs->players->list;
    while(player_node) {
        player_state* player = (player_state*)player_node->data;
        if (player->x == x && player->y == y) 
            death_mark_player(player, "Lost a fist fight");
        player_node = player_node->next;
    }
        
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
    int num = ps->directive[ps->dp];
    int pick = ps->stack[ps->sp - ((rand() % num)+1)];
    ps->sp -= num;
    ps->stack[ps->sp++] = pick;
    ps->dp++;
}

void instr_place(player_state* ps) {
    ps->stack[ps->sp++] = ps->directive[ps->dp];
    ps->dp++;
}

void instr_bomb(player_state* ps) {
    int p = ps->stack[--ps->sp];
    direction d = (direction)ps->stack[--ps->sp];
    if(!spend_resource(ps->resources, "bomb", 1)) return;

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

void meta_player_x(player_state* ps) {
    ps->stack[ps->sp++] = ps->x;
}
void meta_player_y(player_state* ps) {
    ps->stack[ps->sp++] = ps->y;
}
void meta_board_x(player_state* ps) {
    ps->stack[ps->sp++] = _gs->board_x;
}
void meta_board_y(player_state* ps) {
    ps->stack[ps->sp++] = _gs->board_y;
}
void meta_resource(player_state* ps) {
    int index = ps->directive[ps->dp];
    ps->dp++;
    ps->stack[ps->sp++] = peek_resource_index(ps->resources, index);
}
void meta_player_id(player_state* ps) {
    ps->stack[ps->sp++] = ps->id;
}

void instr_access(player_state* ps) {
    int num = ps->stack[ps->sp-1];
    ps->stack[ps->sp-1] = ps->stack[num];
}

void instr_goto(player_state* ps) {
    ps->dp = ps->directive[ps->dp];;
}

void instr_goto_if(player_state* ps) {
    int v = ps->stack[--ps->sp];
    if (v)
        ps->dp = ps->directive[ps->dp];
    else ps->dp++;
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
    int v = ps->stack[--ps->sp];
    int target = ps->stack[--ps->sp];
    ps->stack[target] = v;
}

void instr_flag_access(player_state* ps) { 
    int v = ps->stack[--ps->sp];
    int offset = ps->directive[ps->dp];
    ps->dp++;
    ps->stack[ps->sp++] = v & (1 << offset);
}

void instr_dec_stack(player_state* ps) {
    ps->sp--;
}

void instr_copy(player_state* ps) {
    ps->stack[ps->sp] = ps->stack[ps->sp - 1];
    ps->sp++;
}

void instr_swap(player_state* ps) {
    int v0 = ps->stack[--ps->sp];
    int v1 = ps->stack[--ps->sp];
    ps->stack[ps->sp++] = v0;
    ps->stack[ps->sp++] = v1;
}

void instr_write(player_state* ps) {
    get_field(ps->x, ps->y)->player_data = ps->stack[--ps->sp];
}

void instr_read(player_state* ps) {
    ps->stack[ps->sp++] = get_field(ps->x, ps->y)->player_data;
}

void instr_projection(player_state* ps) {
    if(!spend_resource(ps->resources, "mana", 100)) return;

    resource_registry* projection_registry = copy_resource_registry(ps->resources);
    player_state* projection = malloc(sizeof(player_state));
    event_list* projection_pre_death_events = malloc(sizeof(event_list*));
    event_list* projection_post_death_events = malloc(sizeof(event_list*));
    projection_pre_death_events->list = NULL;
    projection_post_death_events->list = NULL;
    int dir_bytes = sizeof(int) * ps->directive_len;
    int stack_bytes = sizeof(int) * ps->stack_len;
    int* projection_directive = malloc(dir_bytes);
    int* projection_stack = malloc(stack_bytes);
    memcpy(projection_directive, ps->directive, dir_bytes);
    memcpy(projection_stack, ps->stack, stack_bytes);
    
    projection->alive = 1;
    projection->death_msg = NULL;
    projection->team = ps->team;
    projection->name = ps->name;
    projection->id = _gs->player_count++;
    projection->stack = projection_stack;
    projection->stack_len = ps->stack_len;
    projection->sp = ps->sp;
    projection->path = strdup(ps->path);
    projection->directive = projection_directive;
    projection->directive_len = ps->directive_len;
    projection->dp = ps->dp;
    projection->x = ps->x;
    projection->y = ps->y;
    projection->pre_death_events = projection_pre_death_events;
    projection->post_death_events = projection_post_death_events;
    projection->resources = projection_registry;
    add_player(_gs->players, projection);
    _gs->team_states[projection->team].members_alive++;

    projection_death_args* args = malloc(sizeof(projection_death_args));
    args->player_id = projection->id;
    args->remaining = 5;
    add_event(_gs->events, &projection_death_event, args);
}

#endif