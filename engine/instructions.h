#ifndef H_INSTRUCTIONS
#define H_INSTRUCTIONS

#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <stdio.h>

#include "game_state.h"
#include "player.h"
#include "visual.h"
#include "vehicles.h"
#include "util.h"
#include "event_list.h"
#include "events.h"
#include "resource_registry.h"
#include "fields.h"
#include "entity.h"
#include "log.h"

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
  Instr_Mount = 55,
  Instr_Dismount = 56,
  Instr_Boat = 57,
  Instr_BearTrap = 58,
  Instr_Call = 59,
  Instr_Return = 60,
  Instr_Declare = 61,
  Instr_GlobalAccess = 62,
  Instr_GlobalAssign = 63,
  Instr_Index = 64,
  Instr_ThrowClay = 65,
  Instr_ClayGolem = 66,
} instruction;

const char* stack_overflow_msg = "Had an aneurysm (STACK_OVERFLOW)";
const char* frame_break_msg = "Had an aneurysm (FRAME_BREAK)";
const char* div_zero_msg = "Had an aneurysm (DIV_BY_ZERO)";
const char* null_call_msg = "Had an aneurysm (NULL_CALL)";
const char* out_of_bounds_msg = "Had an aneurysm (OUT_OF_BOUNDS)";

// all instructions return 1 if the board should be updated, and 0 if not.

int limit_range(int given, int limit) {
    if (given < 0) return 0;
    if (given <= limit) return given;
    else return limit;
}

int global_scope_sp(player_state* ps, int bp, int sp) {
    while (bp != 0) {
        sp = bp - 2;
        bp = ps->stack[bp - 2];
    }
    return sp;
}



int instr_shoot(player_state* ps) {
    direction d = (direction)ps->stack[--ps->sp];

    if(!spend_resource(ps->resources, "ammo", 1)) {
        ps->stack[ps->sp++] = 0;
        return 0;
    }
    
    int x, y;
    location_coords(ps->location, &x, &y);
    
    int limit = _gr->settings.shoot.range;
    while (limit-- && in_bounds(x,y)) { 
        move_coord(&x, &y, d, 1);
        set_overlay(x,y,BULLET);
        set_color_overlay(x,y,FORE,color_predefs.yellow);
        print_board(); wait(0.02);

        if (fields.is_obstruction(x,y)) {
            fields.damage_field(x, y, KINETIC_DMG & PROJECTILE_DMG, "Got shot");
            ps->stack[ps->sp++] = 1;
            return 1;
        }
        else if (fields.has_player(x,y) && !(fields.is_cover(x,y) || fields.is_shelter(x,y))) {
            field_state* field = get_field(x,y);
            for(int i = 0; i < field->entities->count; i++) {
                entity_t* e = get_entity(field->entities, i);
                if (e->type == ENTITY_PLAYER) {
                    death_mark_player(e->player, "Got shot");
                    break;
                }
            }
            ps->stack[ps->sp++] = 1;
            return 1;
        }
    }
    ps->stack[ps->sp++] = 1;
    return 1;
}

int instr_look(player_state* ps) {
    int offset = ps->stack[--ps->sp]; //ps->directive[ps->dp];
    direction d = (direction)ps->stack[--ps->sp];
    //ps->dp++;

    int x, y;
    location_coords(ps->location, &x, &y);
    int i = 0;
    incr:
    i++;
    if (_gr->settings.look.range >= 0 && i > _gr->settings.look.range) { ps->stack[ps->sp++] = 0; return 0; }
    move_coord(&x, &y, d, 1);
    if (!in_bounds(x,y)) { ps->stack[ps->sp++] = 0; return 0; }
    field_scan scan = fields.scan(x,y);
    field_scan* bypass = &scan;
    if ((*(int*)bypass) & (1 << offset)) { ps->stack[ps->sp++] = i; return 0; }
    if (fields.is_obstruction(x,y)) { ps->stack[ps->sp++] = 0; return 0; }
    goto incr;
}

int instr_scan(player_state* ps) {
    direction p = (direction)ps->stack[--ps->sp];
    direction d = (direction)ps->stack[--ps->sp];
    int x, y;
    location_coords(ps->location, &x, &y);
    int result = 0;

    if (_gr->settings.bomb.range >= 0 && p > _gr->settings.bomb.range) p = _gr->settings.bomb.range;
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
    direction d = (direction)ps->stack[--ps->sp];
    if(!spend_resource(ps->resources, "explosive", 1)) {
        ps->stack[ps->sp++] = 0;
        return 0;
    }

    int x, y;
    location_coords(ps->location, &x, &y);
    move_coord(&x, &y, d, 1);
    if (!in_bounds(x,y)) {
        ps->stack[ps->sp++] = 0;
        return 0;
    }
    char kill = 0;

    field_state* field = get_field(x,y);
    for(int i = 0; i < field->entities->count; i++) {
        entity_t* e = get_entity(field->entities, i);
        if (e->type == ENTITY_PLAYER) {
            death_mark_player(e->player, "Hit by a thrown mine"); 
            kill = 1; 
        }
    }

    if (!kill) {
        field->symbol_overlay = MINE;
        field->foreground_color_overlay = color_predefs.white;
        field_args* args = malloc(sizeof(field_args));
        args->x = x;
        args->y = y;
        add_event(
            field->exit_events,
            PHYSICAL_EVENT,
            events.mine, 
            args
        );
    }
    ps->stack[ps->sp++] = 1;
    return 1;
}

int instr_move(player_state* ps) { 
    direction d = (direction)ps->stack[--ps->sp];

    if (ps->location.type == VEHICLE_LOCATION) {
        return get_vehicle_move_func(ps->location.vehicle->type)(ps->location.vehicle, d);
    }

    int x, y;
    location_coords(ps->location, &x, &y);
    if (fields.is_obstruction(x, y)) {
        ps->stack[ps->sp++] = 0;
        return 0;
    }
    move_coord(&x, &y, d, 1);
    if(!in_bounds(x,y) || fields.is_obstruction(x, y)) {
        ps->stack[ps->sp++] = 0;
        return 0; 
    }

    move_player_to_location(ps, field_location_from_coords(x,y));

    ps->stack[ps->sp++] = 1;
    return 1;
}

int instr_chop(player_state* ps) {
    direction d = (direction)ps->stack[--ps->sp];
    int x, y;
    location_coords(ps->location, &x, &y);
    move_coord(&x, &y, d, 1);
    field_state* field = get_field(x,y);

    if (field->type == EMPTY)
        for(int i = 0; i < field->entities->count; i++) {
            entity_t* e = get_entity(field->entities, i);
            if (e->type == ENTITY_PLAYER) {
                death_mark_player(e->player, "Chopped to pieces");
                break;
            }
        }
    else if (field->type == TREE) {
        fields.remove_field(x,y);
        add_resource(ps->resources, "wood", _gr->settings.chop.wood_gain);
        int got_sapling = rand() % 100 > _gr->settings.chop.sapling_chance;
        if (got_sapling) add_resource(ps->resources, "sapling", 1);
    }

    return 1;
}

int instr_trench(player_state* ps) {
    direction d = (direction)ps->stack[--ps->sp];
    int x, y;
    location_coords(ps->location, &x, &y);
    move_coord(&x, &y, d, 1);
    if (!in_bounds(x, y)) {
        ps->stack[ps->sp++] = 0;
        return 0;
    }

    field_state* field = get_field(x,y);
    switch (field->type) {
        case EMPTY: {
            fields.build.trench(x,y);
            ps->stack[ps->sp++] = 1;
            return 1;
        }
    }
    ps->stack[ps->sp++] = 0;
    return 0;
}

int instr_fortify(player_state* ps) {
    if(!spend_resource(ps->resources, "wood", _gr->settings.fortify.cost)) {
        ps->stack[ps->sp++] = 0;
        return 0;
    } 
    int x, y;
    location_coords(ps->location, &x, &y);
    direction d = (direction)ps->stack[--ps->sp];
    move_coord(&x, &y, d, 1);
    int result = fields.fortify_field(x,y);
    ps->stack[ps->sp++] = result;
    return result;
}

int instr_random_int(player_state* ps) { 
    if (ps->sp + 1 >= _gr->stack_size) {
        death_mark_player(ps, stack_overflow_msg);
        return 0;
    }

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
    if (ps->sp + 1 >= _gr->stack_size) {
        death_mark_player(ps, stack_overflow_msg);
        return 0;
    }

    ps->stack[ps->sp++] = ps->directive[ps->dp];
    ps->dp++;
    return 0;
}

int instr_bomb(player_state* ps) {
    int p = ps->stack[--ps->sp];
    direction d = (direction)ps->stack[--ps->sp];
    if(!spend_resource(ps->resources, "explosive", 1)) {
        ps->stack[ps->sp++] = 0;
        return 0;
    }

    p = limit_range(p, _gr->settings.bomb.range);
    int x, y;
    location_coords(ps->location, &x, &y);
    move_coord(&x, &y, d, p);

    if (!in_bounds(x,y)) {
        ps->stack[ps->sp++] = 0;
        return 0;
    }

    bomb_event_args* args = malloc(sizeof(bomb_event_args));
    args->x = x;
    args->y = y;
    args->player_id = ps->id;
    add_event(_gs->events, PHYSICAL_EVENT, events.bomb, args);
    set_color_overlay(x,y,FORE,color_predefs.red);
    set_overlay(x,y,TARGET);

    ps->stack[ps->sp++] = 1;
    return 1;
}

int meta_player_x(player_state* ps) {
    if (ps->sp + 1 >= _gr->stack_size) {
        death_mark_player(ps, stack_overflow_msg);
        return 0;
    }

    int x, y;
    location_coords(ps->location, &x, &y);
    ps->stack[ps->sp++] = x;
    return 0;
}
int meta_player_y(player_state* ps) {
    if (ps->sp + 1 >= _gr->stack_size) {
        death_mark_player(ps, stack_overflow_msg);
        return 0;
    }

    int x, y;
    location_coords(ps->location, &x, &y);
    ps->stack[ps->sp++] = y;
    return 0;
}
int meta_board_x(player_state* ps) {
    if (ps->sp + 1 >= _gr->stack_size) {
        death_mark_player(ps, stack_overflow_msg);
        return 0;
    }

    ps->stack[ps->sp++] = _gs->board_x;
    return 0;
}
int meta_board_y(player_state* ps) {
    if (ps->sp + 1 >= _gr->stack_size) {
        death_mark_player(ps, stack_overflow_msg);
        return 0;
    }

    ps->stack[ps->sp++] = _gs->board_y;
    return 0;
}
int meta_resource(player_state* ps) {
    if (ps->sp + 1 >= _gr->stack_size) {
        death_mark_player(ps, stack_overflow_msg);
        return 0;
    }

    int index = ps->directive[ps->dp];
    ps->dp++;
    ps->stack[ps->sp++] = peek_resource_index(ps->resources, index);
    return 0;
}
int meta_player_id(player_state* ps) {
    if (ps->sp + 1 >= _gr->stack_size) {
        death_mark_player(ps, stack_overflow_msg);
        return 0;
    }

    ps->stack[ps->sp++] = ps->id;
    return 0;
}

int instr_access(player_state* ps) {
    int n = ps->stack[ps->sp-1];
    if (n < 0 || n >= (ps->sp - ps->bp)) {
        death_mark_player(ps, frame_break_msg);
        return 0;
    }
    ps->stack[ps->sp-1] = ps->stack[ps->bp + n];
    _log(DEBUG, "ACCESS: v: %i, bp: %i, target: %i", ps->stack[ps->sp-1], ps->bp, n);
    return 0;
}

int instr_global_access(player_state* ps) {
    int n = ps->stack[ps->sp-1];
    _log(DEBUG, "GLOBAL ACCESS: bp: %i, target: %i", ps->bp, n);
    if (n < 0 || n >= global_scope_sp(ps, ps->bp, ps->sp)) {
        death_mark_player(ps, frame_break_msg);
        return 0;
    }
    ps->stack[ps->sp-1] = ps->stack[n];
    _log(DEBUG, "found %i", ps->stack[ps->sp-1]);
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
    else 
        ps->dp++;
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
    if (v1 == 0) { death_mark_player(ps, div_zero_msg); return 0; }
    ps->stack[ps->sp++] = v0 / v1;
    return 0;
}

int instr_mod(player_state* ps) {
    int v0 = ps->stack[--ps->sp];
    int v1 = ps->stack[--ps->sp];
    if (v1 == 0) { death_mark_player(ps, div_zero_msg); return 0; }
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
    int n = ps->stack[--ps->sp];
    _log(DEBUG, "ASSIGN: v: %i, bp: %i, target: %i", v, ps->bp, n);
    if (n < 0 || n >= (ps->sp - ps->bp)) {
        death_mark_player(ps, frame_break_msg);
        return 0;
    }
    ps->stack[ps->bp + n] = v;
    return 0;
}

int instr_global_assign(player_state* ps) { 
    int v = ps->stack[--ps->sp];
    int n = ps->stack[--ps->sp];
    _log(DEBUG, "GLOBAL ASSIGN: v: %i, bp: %i, target: %i", v, ps->bp, n);
    if (n < 0 || n >= global_scope_sp(ps, ps->bp, ps->sp)) {
        death_mark_player(ps, frame_break_msg);
        return 0;
    }
    ps->stack[n] = v;
    return 0;
}

int instr_field_prop(player_state* ps) { 
    int offset = ps->stack[--ps->sp];//ps->directive[ps->dp];
    int v = ps->stack[--ps->sp];
    //ps->dp++;
    ps->stack[ps->sp++] = v & (1 << offset);
    return 0;
}

int instr_dec_stack(player_state* ps) {
    ps->sp--;
    return 0;
}

int instr_copy(player_state* ps) {
    if (ps->sp + 1 >= _gr->stack_size) {
        death_mark_player(ps, stack_overflow_msg);
        return 1;
    }

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
    location_field(ps->location)->player_data = ps->stack[--ps->sp];
    ps->stack[ps->sp++] = 1;
    return 0;
}

int instr_read(player_state* ps) {
    if (ps->sp + 1 >= _gr->stack_size) {
        death_mark_player(ps, stack_overflow_msg);
        return 0;
    }

    ps->stack[ps->sp++] = location_field(ps->location)->player_data;
    return 0;
}

int instr_projection(player_state* ps) {
    if (
        !spend_resource(ps->resources, "mana", _gr->settings.projection.cost)
        || !ps->is_original_player
        || ps->location.type == VEHICLE_LOCATION && !(get_vehicle_capacity(ps->location.vehicle->type) > ps->location.vehicle->entities->count)
    ) {
        ps->stack[ps->sp++] = 0;
        return 0;
    }  

    player_state* projection = copy_player_state(ps);
    projection->resources = copy_resource_registry(ps->resources);

    add_player(_gs->players, projection);
    if (projection->team)
        projection->team->members_alive++;

    move_player_to_location(projection, ps->location);

    player_event_args* args = malloc(sizeof(player_event_args));
    args->player_id = projection->id;
    add_event(_gs->events, MAGICAL_EVENT, events.projection_upkeep, args);

    ps->stack[ps->sp++] = 1;
    projection->stack[projection->sp++] = 0;
    return 0;
}

int instr_freeze(player_state* ps) {
    int p = ps->stack[--ps->sp];
    direction d = (direction)ps->stack[--ps->sp];
    if(!spend_resource(ps->resources, "mana", _gr->settings.freeze.cost)) {
        ps->stack[ps->sp++] = 0;
        return 0;
    }

    p = limit_range(p, _gr->settings.freeze.range);
    int x, y;
    location_coords(ps->location, &x, &y);
    move_coord(&x, &y, d, p);

    if(!in_bounds(x,y)) {
        ps->stack[ps->sp++] = 0;
        return 0;
    }
    field_state* field = get_field(x,y);
    if(field->type == ICE_BLOCK) {
        ps->stack[ps->sp++] = 1;
        return 0;
    }

    ice_block_melt_event_args* args = malloc(sizeof(ice_block_melt_event_args));
    args->x = x;
    args->y = y;
    args->player_id = ps->id;
    args->remaining = _gr->settings.freeze.duration;
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

    ps->stack[ps->sp++] = 1;
    return 1;
}

int instr_fireball(player_state* ps) {
    direction d = (direction)ps->stack[--ps->sp];
    if(!spend_resource(ps->resources, "mana", _gr->settings.fireball.cost)) {
        ps->stack[ps->sp++] = 0;
        return 0;
    }

    int x, y;
    location_coords(ps->location, &x, &y);

    int limit = _gr->settings.fireball.range;
    while(limit-- && in_bounds(x,y)) {
        move_coord(&x, &y, d, 1);
        if (fields.is_obstruction(x,y)) {
            fields.damage_field(x, y, FIRE_DMG & PROJECTILE_DMG, "Hit by a fireball");
            ps->stack[ps->sp++] = 1;
            return 1;
        }
        else if (fields.has_player(x,y) && !(fields.is_cover(x,y) || fields.is_shelter(x,y))) {
            field_state* field = get_field(x,y);
            for(int i = 0; i < field->entities->count; i++) {
                entity_t* e = get_entity(field->entities, i);
                if (e->type = ENTITY_PLAYER) {
                    death_mark_player(e->player, "Hit by a fireball");
                    break;
                }
            }
            ps->stack[ps->sp++] = 1;
            return 1;
        }

        set_color_overlay(x,y,FORE,color_predefs.red);
        set_overlay(x,y,FILLED_CIRCLE);
        print_board(); wait(0.05);
    }

    ps->stack[ps->sp++] = 1;
    return 0;
}

int instr_meditate(player_state* ps) {
    add_resource(ps->resources, "mana", _gr->settings.meditate.amount);
    location_field(ps->location)->background_color_overlay = color_predefs.magic_purple;
    ps->stack[ps->sp++] = 1;
    return 1;
}

int instr_dispel(player_state* ps) {
    direction d = (direction)ps->stack[--ps->sp];
    if(!spend_resource(ps->resources, "mana", _gr->settings.dispel.cost)) {
        ps->stack[ps->sp++] = 0;
        return 0;
    } 

    int x, y;
    location_coords(ps->location, &x, &y);
    move_coord(&x, &y, d, 1);
    if (!in_bounds(x,y)) {
        ps->stack[ps->sp++] = 0;
        return 0;
    }

    field_state* field = get_field(x,y);
    remove_events_of_kind(field->enter_events, MAGICAL_EVENT);
    remove_events_of_kind(field->exit_events, MAGICAL_EVENT);

    set_overlay(x,y,LARGE_X);
    set_color_overlay(x,y,FORE,color_predefs.magic_purple);
    print_board(); wait(0.5);
    ps->stack[ps->sp++] = 1;
    return 1;
}

int instr_disarm(player_state* ps) {
    direction d = (direction)ps->stack[--ps->sp];

    int x, y;
    location_coords(ps->location, &x, &y);
    move_coord(&x, &y, d, 1);
    if (!in_bounds(x,y)) {
        ps->stack[ps->sp++] = 0;
        return 0;
    }

    field_state* field = get_field(x,y);
    remove_events_of_kind(field->enter_events, PHYSICAL_EVENT);
    remove_events_of_kind(field->exit_events, PHYSICAL_EVENT);

    set_overlay(x,y,LARGE_X);
    print_board(); wait(0.5);
    ps->stack[ps->sp++] = 1;
    return 1;
}

int instr_mana_drain(player_state* ps) {
    direction d = (direction)ps->stack[--ps->sp];
    if(!spend_resource(ps->resources, "mana", _gr->settings.mana_drain.cost)) {
        ps->stack[ps->sp++] = 0;
        return 0;
    }

    int x, y;
    location_coords(ps->location, &x, &y);
    move_coord(&x, &y, d, 1);
    if (!in_bounds(x,y)) {
        ps->stack[ps->sp++] = 1;
        return 0;
    }

    set_overlay(x,y,EMPTY_DIAMOND);
    set_color_overlay(x,y,FORE,color_predefs.magic_purple);
    add_event(
        get_field(x,y)->enter_events,
        MAGICAL_EVENT,
        events.mana_drain, NULL
    );
    ps->stack[ps->sp++] = 1;
    return 1;
}

int instr_pager_set(player_state* ps) {
    ps->pager_channel = ps->stack[--ps->sp];
    ps->stack[ps->sp++] = 1;
    return 0;
}

int instr_pager_read(player_state* ps) {
    if (ps->sp + 1 >= _gr->stack_size) {
        death_mark_player(ps, stack_overflow_msg);
        return 0;
    }

    if (ps->pager_msgs->count <= 0) 
        ps->stack[ps->sp++] = 0;
    else {
        void* msg = array_list.get(ps->pager_msgs, 0);
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
    ps->stack[ps->sp++] = 1;
    return 0;
}

int instr_wall(player_state* ps) {
    direction d = (direction)ps->stack[--ps->sp];
    int x, y;
    location_coords(ps->location, &x, &y);
    move_coord(&x, &y, d, 1);

    if (
        !spend_resource(ps->resources, "wood", _gr->settings.wall.cost)
        || !in_bounds(x, y)
    ) {
        ps->stack[ps->sp++] = 0;
        return 0;
    }

    field_state* field = get_field(x,y);
    switch (field->type) {
        case EMPTY: {
            fields.build.wall(x,y);
            ps->stack[ps->sp++] = 1;
            return 1;
        }
    }

    ps->stack[ps->sp++] = 0;
    return 0;
}

int instr_plant_tree(player_state* ps) {
    direction d = (direction)ps->stack[--ps->sp];
    int x, y;
    location_coords(ps->location, &x, &y);
    move_coord(&x, &y, d, 1);

    if (
        !in_bounds(x, y)
        || !spend_resource(ps->resources, "sapling", 1)
    ) {
        ps->stack[ps->sp++] = 0;
        return 0;
    } 

    field_countdown_args* args = malloc(sizeof(field_countdown_args));
    args->x = x;
    args->y = y;
    args->player_id = ps->id;
    args->remaining = _gr->settings.plant_tree.delay;

    add_event(_gs->events, PHYSICAL_EVENT, events.tree_grow, args);
    ps->stack[ps->sp++] = 1;
    return 0;
}

int instr_bridge(player_state* ps) {
    direction d = (direction)ps->stack[--ps->sp];
    int x, y;
    location_coords(ps->location, &x, &y);
    move_coord(&x, &y, d, 1);

    if (!in_bounds(x, y)) {
        ps->stack[ps->sp++] = 0;
        return 0;
    }
    field_state* field = get_field(x,y);
    if (
        field->type != OCEAN
        || !spend_resource(ps->resources, "wood", _gr->settings.bridge.cost)
    ) {
        ps->stack[ps->sp++] = 0;
        return 0;
    }

    remove_events_of_kind(field->enter_events, FIELD_EVENT);
    remove_events_of_kind(field->exit_events, FIELD_EVENT);

    field->type = BRIDGE;
    ps->stack[ps->sp++] = 1;
    return 1;
}

int instr_collect(player_state* ps) {
    direction d = (direction)ps->stack[--ps->sp];
    int x, y;
    location_coords(ps->location, &x, &y);
    move_coord(&x, &y, d, 1);

    if (!in_bounds(x, y)) {
        return 0;
        ps->stack[ps->sp++] = 0;
    }
    field_state* field = get_field(x,y);

    int success = 0;
    switch (field->type) {
        case TREE:
            add_resource(ps->resources, "sapling", 1);
            success = 1;
            break;
        case CLAY:
            switch (field->data->clay_pit.amount) {
                case 0: {
                    fields.remove_field(x,y);
                    add_resource(ps->resources, "clay", 1);
                    success = 1;
                    break;
                }
                default: {
                    int collected = field->data->clay_pit.amount > _gr->settings.clay_pit.collect_max ? _gr->settings.clay_pit.collect_max : field->data->clay_pit.amount;
                    field->data->clay_pit.amount -= collected;
                    add_resource(ps->resources, "clay", collected);
                    success = 1;
                    break;
                }
            }
            break;
    }

    ps->stack[ps->sp++] = success;
    return success;
}

int instr_say(player_state* ps) {
    int v = ps->stack[--ps->sp];
    char msg[100 + 1];
    snprintf(msg, 100, "%s#%i says %i\n", ps->name, ps->id, v);
    print_to_feed(msg);
    ps->stack[ps->sp++] = 1;
    return 0;
}

int instr_mount(player_state* ps) {
    int d = ps->stack[--ps->sp];
    int x, y;
    location_coords(ps->location, &x, &y);
    move_coord(&x, &y, d, 1);

    if (!in_bounds(x,y)) {
        ps->stack[ps->sp++] = 0;
        return 0;
    } 
    field_state* field = get_field(x,y);
    
    vehicle_state* vehicle = NULL;
    for(int i = 0; i < field->entities->count; i++) {
        entity_t* e = get_entity(field->entities, i);
        if (e->type == ENTITY_VEHICLE) 
        vehicle = e->vehicle;
    }
    if (vehicle == NULL)  {
        ps->stack[ps->sp++] = 0;
        return 0;
    }

    if (!(vehicle->entities->count < get_vehicle_capacity(vehicle->type))) {
        ps->stack[ps->sp++] = 0;
        return 0;
    } 

    remove_entity(location_field(ps->location)->entities, ps->id);
    ps->location = vehicle_location(vehicle);
    add_entity(vehicle->entities, entity.of_player(ps));
    ps->stack[ps->sp++] = 1;
    return 1;
}

int instr_dismount(player_state* ps) {
    int d = ps->stack[--ps->sp];
    if (!ps->location.type == VEHICLE_LOCATION) {
        ps->stack[ps->sp++] = 0;
        return 0;
    }
    int x, y;
    location_coords(ps->location, &x, &y);
    move_coord(&x, &y, d, 1);

    if (!in_bounds(x,y)) {
        ps->stack[ps->sp++] = 0;
        return 0;
    }

    remove_entity(ps->location.vehicle->entities, ps->id);
    field_state* field = get_field(x,y);
    location prev_loc = ps->location;
    ps->location = field_location_from_field(field);
    add_entity(field->entities, entity.of_player(ps));
    update_events(entity.of_player(ps), get_field(x,y)->enter_events, (situation){ .type = MOVEMENT_SITUATION, .movement.loc = prev_loc });

    ps->stack[ps->sp++] = 1;
    return 1;
}

int instr_boat(player_state* ps) {
    int d = ps->stack[--ps->sp];
    int x, y;
    location_coords(ps->location, &x, &y);
    move_coord(&x, &y, d, 1);

    if (!in_bounds(x,y)) {
        ps->stack[ps->sp++] = 0;
        return 0;
    }
    field_state* field = get_field(x,y);
    if (field->type != OCEAN) {
        ps->stack[ps->sp++] = 0;
        return 0;
    }

    if (!spend_resource(ps->resources, "wood", _gr->settings.boat.cost)) {
        ps->stack[ps->sp++] = 0;
        return 0;
    }

    vehicle_state* boat = malloc(sizeof(vehicle_state));
    boat->id = _gs->id_counter++;
    boat->entities = array_list.create(get_vehicle_capacity(VEHICLE_BOAT));
    boat->location = field_location_from_field(field);
    boat->type = VEHICLE_BOAT;
    boat->destroy = 0;
    add_entity(field->entities, entity.of_vehicle(boat));
    ps->stack[ps->sp++] = 1;
    return 1;
}

int instr_bear_trap(player_state* ps) {
    direction d = (direction)ps->stack[--ps->sp];
    // TODO: figure out a resource for this
    //if(!spend_resource(ps->resources, "explosive", 1)) return 0;

    int x, y;
    location_coords(ps->location, &x, &y);
    move_coord(&x, &y, d, 1);
    if (!in_bounds(x,y)) {
        ps->stack[ps->sp++] = 0;
        return 0;
    }
    field_state* field = get_field(x,y);
    
    field->symbol_overlay = BEAR_TRAP;
    field->foreground_color_overlay = color_predefs.white;
    add_event(
        field->enter_events,
        PHYSICAL_EVENT,
        events.bear_trap_trigger, NULL
    );

    ps->stack[ps->sp++] = 1;
    return 1;
}

int instr_call(player_state* ps) {
    int arg_count = ps->stack[--ps->sp];
    int func_addr = ps->stack[--ps->sp];

    if (func_addr == 0) {
        death_mark_player(ps, null_call_msg);
        return 0;
    }

    if (ps->sp + arg_count + 3 >= _gr->stack_size) {
        death_mark_player(ps, stack_overflow_msg);
        return 0;
    }

    int args[arg_count];
    memcpy(args, &ps->stack[ps->sp - arg_count], sizeof(int) * arg_count);

    ps->sp -= arg_count;
    int old_bp = ps->bp;
    int old_dp = ps->dp;

    ps->stack[ps->sp++] = old_bp;
    ps->stack[ps->sp++] = old_dp;
    ps->bp = ps->sp;
    ps->stack[ps->sp] = func_addr;
    memcpy(&ps->stack[ps->sp + 1], args, sizeof(int) * arg_count);
    ps->sp += arg_count + 1;
    ps->dp = func_addr;
    _log(DEBUG, "f_addr: %i, args: %i", func_addr, arg_count);
}

int instr_return(player_state* ps) {
    int ret = ps->stack[--ps->sp];
    int old_dp = ps->stack[ps->bp - 1];
    int old_bp = ps->stack[ps->bp - 2];

    ps->dp = old_dp;
    ps->sp = ps->bp - 2;
    ps->bp = old_bp;

    ps->stack[ps->sp++] = ret;
    _log(DEBUG, "RETURN: ret:%i old_dp: %i, old_bp: %i", ret, old_dp, old_bp);
}

int instr_declare(player_state* ps) {
    int n = ps->directive[ps->dp];
    if (ps->sp + n >= _gr->stack_size) {
        death_mark_player(ps, stack_overflow_msg);
        return 0;
    }
    memset(ps->stack + ps->sp, 0, sizeof(int) * n);
    ps->sp += n;
    ps->dp++;
    return 0;
}

int instr_index(player_state* ps) {
    int idx = ps->stack[--ps->sp];
    int arr = ps->stack[--ps->sp];
    int arr_size = ps->directive[ps->dp++];
    int elem_size = ps->directive[ps->dp++];

    _log(DEBUG, "idx: %i, arr: %i, arr_size: %i, elem_size: %i", idx, arr, arr_size, elem_size);

    if (idx < 0 || idx >= arr_size) {
        death_mark_player(ps, out_of_bounds_msg);
        return 0;
    }

    ps->stack[ps->sp++] = arr + idx * elem_size;

    return 0;
}

int instr_throw_clay(player_state* ps) {
    int p = ps->stack[--ps->sp];
    direction d = (direction)ps->stack[--ps->sp];

    if(!spend_resource(ps->resources, "clay", _gr->settings.throw_clay.cost)) {
        ps->stack[ps->sp++] = 0;
        return 0;
    }

    p = limit_range(p, _gr->settings.throw_clay.range);
    
    int x, y;
    location_coords(ps->location, &x, &y);
    
    while (p-- && in_bounds(x,y)) { 
        move_coord(&x, &y, d, 1);
        set_overlay(x,y,FILLED_CIRCLE);
        set_color_overlay(x,y,FORE,color_predefs.clay_brown);
        print_board(); wait(0.02);
        
        if (fields.is_obstruction(x,y)) {
            fields.damage_field(x, y, KINETIC_DMG & PROJECTILE_DMG, "Got shot");
            ps->stack[ps->sp++] = 1;
            return 1;
        }
        else if (fields.has_player(x,y) && !(fields.is_cover(x,y) || fields.is_shelter(x,y))) {
            field_state* field = get_field(x,y);
            for(int i = 0; i < field->entities->count; i++) {
                entity_t* e = get_entity(field->entities, i);
                if (e->type == ENTITY_PLAYER) {
                    death_mark_player(e->player, "Got shot");
                    break;
                }
            }
            ps->stack[ps->sp++] = 1;
            return 1;
        }    
    }

    // clayify field
    field_state* field = get_field(x,y);
    switch (field->type) {
        case EMPTY:
            fields.build.clay_pit(x,y);
            break;
        case CLAY:
            if (field->data->clay_pit.amount < _gr->settings.clay_pit.contain_limit)
                field->data->clay_pit.amount++;
            break;
    }


    ps->stack[ps->sp++] = 1;
    return 1;
}

int instr_clay_golem(player_state* ps) {
    if (
        !spend_resource(ps->resources, "clay", _gr->settings.clay_golem.cost)
        || !ps->is_original_player
        || ps->location.type == VEHICLE_LOCATION && !(get_vehicle_capacity(ps->location.vehicle->type) > ps->location.vehicle->entities->count)
    ) {
        ps->stack[ps->sp++] = 0;
        return 0;
    }  

    player_state* golem = copy_player_state(ps);
    golem->team = NULL;
    golem->resources = get_empty_resource_registy();

    add_player(_gs->players, golem);

    move_player_to_location(golem, ps->location);

    ps->stack[ps->sp++] = 1;
    golem->stack[golem->sp++] = 0;
    return 0;
}

#endif