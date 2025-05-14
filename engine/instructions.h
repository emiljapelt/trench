#ifndef H_INSTRUCTIONS
#define H_INSTRUCTIONS

#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <stdio.h>

#include "game_state.h"
#include "player.h"
#include "visual.h"
#include "util.h"
#include "event_list.h"
#include "events.h"
#include "resource_registry.h"
#include "fields.h"

typedef enum {
  Meta_PlayerX = 0,
  Meta_PlayerY = 1,
  Meta_BoardX = 2,
  Meta_BoardY = 3,
  Meta_Resource = 4,
  Meta_PlayerID = 5,
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
  Instr_FieldProp = 24,
  Instr_Assign = 25,
  Instr_GoToIf = 26,
  Instr_GoTo = 27,
  Instr_Move = 28,
  Instr_PlantTree = 29,
  Instr_Trench = 30,
  Instr_Fortify = 31,
  Instr_Bomb = 32,
  Instr_Shoot = 33,
  Instr_Wait = 34,
  Instr_Pass = 35,
  Instr_Look = 36,
  Instr_Mine = 37,
  Instr_Chop = 38,
  Instr_Read = 39,
  Instr_Write = 40,
  Instr_Projection = 41,
  Instr_Freeze = 42,
  Instr_Fireball = 43,
  Instr_Meditate = 44,
  Instr_Dispel = 45,
  Instr_Disarm = 46,
  Instr_ManaDrain = 47,
  Instr_PagerSet = 48,
  Instr_PagerWrite = 49,
  Instr_PagerRead = 50,
  Instr_Wall = 51,
  Instr_Bridge = 52,
  Instr_Collect = 53,
  Instr_Say = 54,
} instruction;

// all instructions return 1 if the board should be updated, and 0 if not.

int instr_shoot(player_state* ps) {
    if(!spend_resource(ps->resources, "shot", 1)) return 0;
    
    direction d = (direction)ps->stack[--ps->sp];
    int x = ps->x;
    int y = ps->y;
    
    move_coord(&x, &y, d, 1);
    int limit = _gr->instr.shoot.range;
    while (limit-- && in_bounds(x,y)) { 
        set_overlay(x,y,BULLET);
        set_color_overlay(x,y,FORE,color_predefs.yellow);
        print_board(); wait(0.02);

        if (fields.is_obstruction(x,y)) {
            fields.damage_field(x, y, KINETIC_DMG & PROJECTILE_DMG, "Got shot");
            return 1;
        }
        else if (fields.has_player(x,y) && !(fields.is_cover(x,y) || fields.is_shelter(x,y))) {
            for(int i = 0; i < _gs->players->count; i++) {
                player_state* player = get_player(_gs->players, i);
                if (player->alive && player->x == x && player->y == y) 
                    death_mark_player(player, "Got shot");
            }
            return 1;
        }

        move_coord(&x, &y, d, 1);
    }
    return 1;
}

int instr_look(player_state* ps) {
    direction d = (direction)ps->stack[--ps->sp];
    int offset = ps->directive[ps->dp];
    ps->dp++;

    int x = ps->x;
    int y = ps->y;
    int i = 0;
    incr:
    i++;
    if (i > _gr->instr.look.range) { ps->stack[ps->sp++] = 0; return 0; }
    move_coord(&x, &y, d, 1);
    if (!in_bounds(x,y)) { ps->stack[ps->sp++] = 0; return 0; }
    field_scan scan = fields.scan(x,y);
    field_scan* bypass = &scan;
    if ((*(int*)bypass) & (1 << offset)) { ps->stack[ps->sp++] = i; return 0; }
    if (fields.is_obstruction(x,y)) { ps->stack[ps->sp++] = 0; return 0; }
    goto incr;
}

int instr_scan(player_state* ps) {
    int x = ps->x;
    int y = ps->y;
    direction p = (direction)ps->stack[--ps->sp];
    direction d = (direction)ps->stack[--ps->sp];
    int result = 0;

    if (_gr->instr.bomb.range >= 0 && p > _gr->instr.bomb.range) p = _gr->instr.bomb.range;
    move_coord(&x, &y, d, p);
    if (in_bounds(x, y)) {   
        field_scan scan = fields.scan(x,y);
        field_scan* bypass = &scan;
        result = *(int*)bypass;
    }

    ps->stack[ps->sp++] = result;
    return 0;
}

int instr_mine(player_state* ps) {
    if(!spend_resource(ps->resources, "bomb", 1)) return 0;

    int x = ps->x;
    int y = ps->y;
    direction d = (direction)ps->stack[--ps->sp];
    move_coord(&x, &y, d, 1);
    if (!in_bounds(x,y)) return 0;
    char kill = 0;

    for(int i = 0; i < _gs->players->count; i++) {
        player_state* player = get_player(_gs->players, i);
        if (player->x == x && player->y == y) { 
            death_mark_player(player, "Hit by a thrown mine"); 
            kill = 1; 
        }
    }

    if (!kill) {
        set_overlay(x,y,MINE);
        add_event(
            get_field(x,y)->exit_events,
            PHYSICAL_EVENT,
            events.mine, NULL
        );
    }
    return 1;
}

int instr_move(player_state* ps) { 
    direction d = (direction)ps->stack[--ps->sp];

    if (fields.is_obstruction(ps->x, ps->y)) return 0;
    int x = ps->x;
    int y = ps->y;
    move_coord(&x, &y, d, 1);
    if (!in_bounds(x,y) || !fields.is_walkable(x,y)) return 0;

    update_events(ps, get_field(ps->x,ps->y)->exit_events);
    if (!ps->death_msg) {
        ps->x = x;
        ps->y = y;
        update_events(ps, get_field(x,y)->enter_events);
    }
    return 1;
}

int instr_chop(player_state* ps) {
    int x = ps->x;
    int y = ps->y;
    direction d = (direction)ps->stack[--ps->sp];
    move_coord(&x, &y, d, 1);
    field_state* field = get_field(x,y);

    if (field->type == EMPTY)
        for(int i = 0; i < _gs->players->count; i++) {
            player_state* player = get_player(_gs->players, i);
            if (player->x == x && player->y == y) 
                death_mark_player(player, "Chopped to pieces");
        }
    else if (field->type == TREE) {
        fields.remove_field(x,y);
        add_resource(ps->resources, "wood", _gr->instr.chop.wood_gain);
        int got_sapling = rand() % 100 > _gr->instr.chop.sapling_chance;
        if (got_sapling) add_resource(ps->resources, "sapling", 1);
    }

    return 1;
}

int instr_trench(player_state* ps) {
    int x = ps->x;
    int y = ps->y;
    direction d = (direction)ps->stack[--ps->sp];
    move_coord(&x, &y, d, 1);
    if (!in_bounds(x, y)) return 0;

    field_state* field = get_field(x,y);
    switch (field->type) {
        case EMPTY: {
            fields.build.trench(x,y);
            return 1;
        }
    }
    return 0;
}

int instr_fortify(player_state* ps) {
    if(!spend_resource(ps->resources, "wood", _gr->instr.fortify.cost)) return 0;
    int x = ps->x;
    int y = ps->y;
    direction d = (direction)ps->stack[--ps->sp];
    move_coord(&x, &y, d, 1);
    return fields.fortify_field(x,y);
}

int instr_random_int(player_state* ps) { 
    ps->stack[ps->sp++] = rand();
    return 0;
}

int instr_random_range(player_state* ps) {
    int num = ps->directive[ps->dp];
    int pick = ps->stack[ps->sp - ((rand() % num)+1)];
    ps->sp -= num;
    ps->stack[ps->sp++] = pick;
    ps->dp++;
    return 0;
}

int instr_place(player_state* ps) {
    ps->stack[ps->sp++] = ps->directive[ps->dp];
    ps->dp++;
    return 0;
}

int instr_bomb(player_state* ps) {
    int p = ps->stack[--ps->sp];
    direction d = (direction)ps->stack[--ps->sp];
    if(!spend_resource(ps->resources, "bomb", 1)) return 0;

    if (_gr->instr.bomb.range >= 0 && p > _gr->instr.bomb.range) p = _gr->instr.bomb.range;
    int x = ps->x;
    int y = ps->y;
    move_coord(&x, &y, d, p);

    if (!in_bounds(x,y)) return 0;

    bomb_event_args* args = malloc(sizeof(bomb_event_args));
    args->x = x;
    args->y = y;
    args->player_id = ps->id;
    add_event(_gs->events, PHYSICAL_EVENT, events.bomb, args);
    set_color_overlay(x,y,FORE,color_predefs.red);
    set_overlay(x,y,TARGET);
    return 1;
}

int meta_player_x(player_state* ps) {
    ps->stack[ps->sp++] = ps->x;
    return 0;
}
int meta_player_y(player_state* ps) {
    ps->stack[ps->sp++] = ps->y;
    return 0;
}
int meta_board_x(player_state* ps) {
    ps->stack[ps->sp++] = _gs->board_x;
    return 0;
}
int meta_board_y(player_state* ps) {
    ps->stack[ps->sp++] = _gs->board_y;
    return 0;
}
int meta_resource(player_state* ps) {
    int index = ps->directive[ps->dp];
    ps->dp++;
    ps->stack[ps->sp++] = peek_resource_index(ps->resources, index);
    return 0;
}
int meta_player_id(player_state* ps) {
    ps->stack[ps->sp++] = ps->id;
    return 0;
}

int instr_access(player_state* ps) {
    int num = ps->stack[ps->sp-1];
    if (num < 0 || num >= ps->stack_len) {
        death_mark_player(ps, "Had an aneurysm");
        return 0;
    }
    ps->stack[ps->sp-1] = ps->stack[num];
    return 0;
}

int instr_goto(player_state* ps) {
    ps->dp = ps->directive[ps->dp];
    return 0;
}

int instr_goto_if(player_state* ps) {
    int v = ps->stack[--ps->sp];
    if (v)
        ps->dp = ps->directive[ps->dp];
    else ps->dp++;
    return 0;
}

int instr_eq(player_state* ps) {
    int v0 = ps->stack[--ps->sp];
    int v1 = ps->stack[--ps->sp];
    ps->stack[ps->sp++] = v0 == v1;
    return 0;
}

int instr_lt(player_state* ps) {
    int v0 = ps->stack[--ps->sp];
    int v1 = ps->stack[--ps->sp];
    ps->stack[ps->sp++] = v0 < v1;
    return 0;
}

int instr_sub(player_state* ps) {
    int v0 = ps->stack[--ps->sp];
    int v1 = ps->stack[--ps->sp];
    ps->stack[ps->sp++] = v1 - v0;
    return 0;
}

int instr_add(player_state* ps) {
    int v0 = ps->stack[--ps->sp];
    int v1 = ps->stack[--ps->sp];
    ps->stack[ps->sp++] = v0 + v1;
    return 0;
}

int instr_mul(player_state* ps) {
    int v0 = ps->stack[--ps->sp];
    int v1 = ps->stack[--ps->sp];
    ps->stack[ps->sp++] = v0 * v1;
    return 0;
}

int instr_div(player_state* ps) { 
    int v0 = ps->stack[--ps->sp];
    int v1 = ps->stack[--ps->sp];
    if (v1 == 0) { death_mark_player(ps, "Mental break down (div by 0)"); return 0; }
    ps->stack[ps->sp++] = v0 / v1;
    return 0;
}

int instr_mod(player_state* ps) {
    int v0 = ps->stack[--ps->sp];
    int v1 = ps->stack[--ps->sp];
    if (v1 == 0) { death_mark_player(ps, "Mental break down (div by 0)"); return 0; }
    ps->stack[ps->sp++] = ((v0%v1) + v1)%v1;
    return 0;
}

int instr_not(player_state* ps) {
    int v = ps->stack[--ps->sp];
    ps->stack[ps->sp++] = !v;
    return 0;
}

int instr_or(player_state* ps) {
    int v0 = ps->stack[--ps->sp];
    int v1 = ps->stack[--ps->sp];
    ps->stack[ps->sp++] = v0 || v1;
    return 0;
}

int instr_and(player_state* ps) {
    int v0 = ps->stack[--ps->sp];
    int v1 = ps->stack[--ps->sp];
    ps->stack[ps->sp++] = v0 && v1;
    return 0;
}

int instr_assign(player_state* ps) { 
    int v = ps->stack[--ps->sp];
    int target = ps->stack[--ps->sp];
    if (target < 0 || target >= ps->stack_len) {
        death_mark_player(ps, "Had an aneurysm");
        return 0;
    }
    ps->stack[target] = v;
    return 0;
}

int instr_field_prop(player_state* ps) { 
    int v = ps->stack[--ps->sp];
    int offset = ps->directive[ps->dp];
    ps->dp++;
    ps->stack[ps->sp++] = v & (1 << offset);
    return 0;
}

int instr_dec_stack(player_state* ps) {
    ps->sp--;
    return 0;
}

int instr_copy(player_state* ps) {
    ps->stack[ps->sp] = ps->stack[ps->sp - 1];
    ps->sp++;
    return 0;
}

int instr_swap(player_state* ps) {
    int v0 = ps->stack[--ps->sp];
    int v1 = ps->stack[--ps->sp];
    ps->stack[ps->sp++] = v0;
    ps->stack[ps->sp++] = v1;
    return 0;
}

int instr_write(player_state* ps) {
    get_field(ps->x, ps->y)->player_data = ps->stack[--ps->sp];
    return 0;
}

int instr_read(player_state* ps) {
    ps->stack[ps->sp++] = get_field(ps->x, ps->y)->player_data;
    return 0;
}

int instr_projection(player_state* ps) {
    if(!spend_resource(ps->resources, "mana", _gr->instr.projection.cost)) return 0;

    player_state* projection = copy_player_state(ps);

    add_player(_gs->players, projection);
    projection->team->members_alive++;

    countdown_args* args = malloc(sizeof(countdown_args));
    args->player_id = projection->id;
    args->remaining = _gr->instr.projection.duration;
    add_event(_gs->events, MAGICAL_EVENT, events.projection_death, args);

    return 0;
}

int instr_freeze(player_state* ps) {
    int p = ps->stack[--ps->sp];
    direction d = (direction)ps->stack[--ps->sp];
    if(!spend_resource(ps->resources, "mana", _gr->instr.freeze.cost)) return 0;

    if (_gr->instr.freeze.range >= 0 &&p > _gr->instr.freeze.range) p = _gr->instr.freeze.range;
    int x = ps->x;
    int y = ps->y;
    move_coord(&x, &y, d, p);

    if(!in_bounds(x,y)) return 0;
    field_state* field = get_field(x,y);
    if(field->type == ICE_BLOCK) return 0;

    ice_block_melt_event_args* args = malloc(sizeof(ice_block_melt_event_args));
    args->x = x;
    args->y = y;
    args->player_id = ps->id;
    args->remaining = _gr->instr.freeze.duration;
    add_event(_gs->events, NONE_EVENT, events.ice_block_melt, args);

    set_color_overlay(x,y,FORE,color_predefs.ice_blue);
    set_color_overlay(x,y,BACK,color_predefs.black);
    set_overlay(x,y,SNOWFLAKE);
    print_board(); wait(1);

    field_data* new_data = malloc(sizeof(field_data));
    new_data->ice_block.inner = field->data;
    new_data->ice_block.inner_type = field->type;
    field->type = ICE_BLOCK;
    field->data = new_data;

    return 1;
}

int instr_fireball(player_state* ps) {
    direction d = (direction)ps->stack[--ps->sp];
    if(!spend_resource(ps->resources, "mana", _gr->instr.fireball.cost)) return 0;
    int x = ps->x;
    int y = ps->y;

    move_coord(&x, &y, d, 1);
    int limit = _gr->instr.fireball.range;
    while(limit-- && in_bounds(x,y)) {
        if (fields.is_obstruction(x,y)) {
            fields.damage_field(x, y, FIRE_DMG & PROJECTILE_DMG, "Hit by a fireball");
            return 1;
        }
        else if (fields.has_player(x,y) && !(fields.is_cover(x,y) || fields.is_shelter(x,y))) {
            for(int i = 0; i < _gs->players->count; i++) {
                player_state* player = get_player(_gs->players, i);
                if (player->alive && player->x == x && player->y == y) 
                    death_mark_player(player, "Hit by a fireball");
            }
            return 1;
        }

        set_color_overlay(x,y,FORE,color_predefs.red);
        set_overlay(x,y,FILLED_CIRCLE);
        print_board(); wait(0.05);
        move_coord(&x, &y, d, 1);
    }
}

int instr_meditate(player_state* ps) {
    add_resource(ps->resources, "mana", _gr->instr.meditate.amount);
    set_color_overlay(ps->x,ps->y,BACK,color_predefs.magic_purple);
    return 1;
}

int instr_dispel(player_state* ps) {
    direction d = (direction)ps->stack[--ps->sp];
    if(!spend_resource(ps->resources, "mana", _gr->instr.dispel.cost)) return 0;

    int x = ps->x;
    int y = ps->y;
    move_coord(&x, &y, d, 1);
    if (!in_bounds(x,y)) return 0;

    field_state* field = get_field(x,y);
    for (int i = 0; i < field->enter_events->count; i++) {
        event* e = get_event(field->enter_events,i);
        if (e->kind == MAGICAL_EVENT)
            e->func = NULL;
    }
    for (int i = 0; i < field->exit_events->count; i++) {
        event* e = get_event(field->exit_events,i);
        if (e->kind == MAGICAL_EVENT)
            e->func = NULL;
    }

    set_overlay(x,y,LARGE_X);
    set_color_overlay(x,y,FORE,color_predefs.magic_purple);
    print_board(); wait(0.5);
    return 1;
}

int instr_disarm(player_state* ps) {
    direction d = (direction)ps->stack[--ps->sp];

    int x = ps->x;
    int y = ps->y;
    move_coord(&x, &y, d, 1);
    if (!in_bounds(x,y)) return 0;

    field_state* field = get_field(x,y);
    for (int i = 0; i < field->enter_events->count; i++) {
        event* e = get_event(field->enter_events,i);
        if (e->kind == PHYSICAL_EVENT)
            e->func = NULL;
    }
    for (int i = 0; i < field->exit_events->count; i++) {
        event* e = get_event(field->exit_events,i);
        if (e->kind == PHYSICAL_EVENT)
            e->func = NULL;
    }

    set_overlay(x,y,LARGE_X);
    print_board(); wait(0.5);
    return 1;
}

int instr_mana_drain(player_state* ps) {
    direction d = (direction)ps->stack[--ps->sp];
    if(!spend_resource(ps->resources, "mana", _gr->instr.mana_drain.cost)) return 0;

    int x = ps->x;
    int y = ps->y;
    move_coord(&x, &y, d, 1);
    if (!in_bounds(x,y)) return 0;

    set_overlay(x,y,EMPTY_DIAMOND);
    set_color_overlay(x,y,FORE,color_predefs.magic_purple);
    add_event(
        get_field(x,y)->enter_events,
        MAGICAL_EVENT,
        events.mana_drain, NULL
    );
    return 1;
}

int instr_pager_set(player_state* ps) {
    ps->pager_channel = ps->stack[--ps->sp];
    return 0;
}

int instr_pager_read(player_state* ps) {
    if (ps->pager_msgs->count <= 0) 
        ps->stack[ps->sp++] = 0;
    else {
        void* msg = array_list.get(ps->pager_msgs, 0);
        //printf("\tread: %i\n", msg); wait(1);
        void** msg_bypass = &msg;
        ps->stack[ps->sp++] = *(int*)msg_bypass;
        array_list.remove(ps->pager_msgs, 0, 0);
    }
    return 0;
}

int instr_pager_write(player_state* ps) {
    int msg = ps->stack[--ps->sp];
    int* msg_bypass = &msg;
    int channel = ps->pager_channel;
    for(int i = 0; i < _gs->players->count; i++) {
        player_state* other = array_list.get(_gs->players, i);
        if (other->id != ps->id && other->pager_channel == channel) {
            array_list.add(other->pager_msgs, *(void**)msg_bypass);
        }
    }
    return 0;
}

int instr_wall(player_state* ps) {
    int x = ps->x;
    int y = ps->y;
    direction d = (direction)ps->stack[--ps->sp];
    move_coord(&x, &y, d, 1);

    if (!in_bounds(x, y)) return 0;
    if (!spend_resource(ps->resources, "wood", _gr->instr.wall.cost)) return 0;

    field_state* field = get_field(x,y);
    switch (field->type) {
        case EMPTY: {
            fields.build.wall(x,y);
            return 1;
        }
    }
    return 0;
}

int instr_plant_tree(player_state* ps) {
    int x = ps->x;
    int y = ps->y;
    direction d = (direction)ps->stack[--ps->sp];
    move_coord(&x, &y, d, 1);

    if (!in_bounds(x, y)) return 0;
    if (!spend_resource(ps->resources, "sapling", 1)) return 0;

    field_countdown_args* args = malloc(sizeof(field_countdown_args));
    args->x = x;
    args->y = y;
    args->player_id = ps->id;
    args->remaining = _gr->instr.plant_tree.delay;

    add_event(_gs->events, PHYSICAL_EVENT, events.tree_grow, args);
    return 0;
}

int instr_bridge(player_state* ps) {
    int x = ps->x;
    int y = ps->y;
    direction d = (direction)ps->stack[--ps->sp];
    move_coord(&x, &y, d, 1);

    if (!in_bounds(x, y)) return 0;
    field_state* field = get_field(x,y);
    if (field->type != OCEAN) return 0;
    if (!spend_resource(ps->resources, "wood", _gr->instr.bridge.cost)) return 0;

    field->type = BRIDGE;
    return 1;
}

int instr_collect(player_state* ps) {
    int x = ps->x;
    int y = ps->y;
    direction d = (direction)ps->stack[--ps->sp];
    move_coord(&x, &y, d, 1);

    if (!in_bounds(x, y)) return 0;
    field_state* field = get_field(x,y);

    switch (field->type) {
        case TREE:
            add_resource(ps->resources, "sapling", 1);
            return 0;
        default:
            return 0;
    }
}

int instr_say(player_state* ps) {
    int v = ps->stack[--ps->sp];
    char msg[100 + 1];
    snprintf(msg, 100, "%s#%i says %i\n", ps->name, ps->id, v);
    print_to_feed(msg);
    return 0;
}

#endif