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

#pragma region DEFINITIONS

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
  Instr_Take = 67,
  Instr_Drop = 68,
  Instr_MineShaft = 69,
  Instr_Craft = 70,
} instruction;

typedef enum {
    INSTR_SUCCESS = 1,
    // -----
    INSTR_ERROR = 0,
    INSTR_MISSING_RESOURCE = -1,
    INSTR_OUT_OF_BOUNDS = -2,
    INSTR_INVALID_TARGET = -3,
    INSTR_OUT_OF_RANGE = -4,
    INSTR_OBSTRUCTED = -5,
    INSTR_MISSING_SPACE = -6
} instruction_result;

const char* stack_overflow_msg = "Had an aneurysm (STACK_OVERFLOW)";
const char* frame_break_msg = "Had an aneurysm (FRAME_BREAK)";
const char* div_zero_msg = "Had an aneurysm (DIV_BY_ZERO)";
const char* null_call_msg = "Had an aneurysm (NULL_CALL)";
const char* out_of_bounds_msg = "Had an aneurysm (OUT_OF_BOUNDS)";

// all instructions return 1 if the board should be updated, and 0 if not.

#pragma endregion

#pragma region HELPERS

int limit_range(int given, int limit) {
    if (given < 0) return 0;
    if (limit < 0) return given;
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

#pragma endregion

#pragma region LANGUAGE_CORE

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
    if (v > 0)
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
    int offset = ps->stack[--ps->sp];
    int v = ps->stack[--ps->sp];
    ps->stack[ps->sp++] = v & (1 << offset) ? 1 : 0;
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


#pragma endregion

#pragma region META_VARIABLES

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

    int resource = ps->stack[--ps->sp];
    ps->stack[ps->sp++] = peek_resource(&ps->resources, resource);
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

#pragma endregion

#pragma region BUILTIN_FUNCTIONS

int instr_shoot(player_state* ps) {
    direction d = (direction)ps->stack[--ps->sp];

    if(!spend_resource(&ps->resources, R_Ammo, 1)) {
        ps->stack[ps->sp++] = INSTR_MISSING_RESOURCE;
        return 0;
    }
    
    int x, y;
    location_coords(ps->location, &x, &y);
    
    int limit = _gr->settings.shoot.range;
    while (limit--) { 
        move_coord(&x, &y, d, 1);
        if (!in_bounds(x,y)) break;

        field_state* field = fields.get(x,y);
        unsigned int props = fields.properties_of_field(field);

        set_overlay(field, BULLET);
        set_color_overlay(field, FORE, YELLOW);
        print_board(); wait(0.02);


        if (props & PROP_OBSTRUCTION) {
            fields.damage_field(field, KINETIC_DMG | PROJECTILE_DMG, "Got shot");
            ps->stack[ps->sp++] = INSTR_SUCCESS;
            return 1;
        }
        else if ((props & PROP_PLAYER) && !((props & PROP_COVER) || (props & PROP_SHELTER))) {
            for(int i = 0; i < field->entities->count; i++) {
                entity_t* e = get_entity(field->entities, i);
                if (e->type == ENTITY_PLAYER) {
                    death_mark_player(e->player, "Got shot");
                    break;
                }
            }
            ps->stack[ps->sp++] = INSTR_SUCCESS;
            return 1;
        }
    }
    ps->stack[ps->sp++] = INSTR_SUCCESS;
    return 1;
}

int instr_look(player_state* ps) {
    int offset = ps->stack[--ps->sp];
    direction d = (direction)ps->stack[--ps->sp];

    int x, y;
    location_coords(ps->location, &x, &y);
    int i = 0;
    incr:
    i++;
    if (_gr->settings.look.range >= 0 && i > _gr->settings.look.range) { ps->stack[ps->sp++] = INSTR_OUT_OF_RANGE; return 0; }
    move_coord(&x, &y, d, 1);
    if (!in_bounds(x,y)) { ps->stack[ps->sp++] = INSTR_OUT_OF_BOUNDS; return 0; }
    unsigned int props = fields.properties(x,y);
    if (props & (1 << offset)) { ps->stack[ps->sp++] = i; return 0; }
    if (props & PROP_OBSTRUCTION) { ps->stack[ps->sp++] = INSTR_OBSTRUCTED; return 0; }
    goto incr;
}

// Returns the empty result on failure
int instr_scan(player_state* ps) {
    int p = ps->stack[--ps->sp];
    direction d = (direction)ps->stack[--ps->sp];
    int x, y;
    location_coords(ps->location, &x, &y);
    int result = 0;

    if (_gr->settings.scan.range >= 0 && p > _gr->settings.scan.range) { ps->stack[ps->sp++] = 0; return 0; }
    move_coord(&x, &y, d, p);
    if (in_bounds(x, y)) {   
        result = fields.properties(x,y);
    }

    ps->stack[ps->sp++] = result;
    return 0;
}

int instr_mine(player_state* ps) {
    direction d = (direction)ps->stack[--ps->sp];
    if(!spend_resource(&ps->resources, R_Explosive, 1)) {
        ps->stack[ps->sp++] = INSTR_MISSING_RESOURCE;
        return 0;
    }

    int x, y;
    location_coords(ps->location, &x, &y);
    move_coord(&x, &y, d, 1);
    if (!in_bounds(x,y)) {
        ps->stack[ps->sp++] = INSTR_OUT_OF_BOUNDS;
        return 0;
    }
    char kill = 0;

    field_state* field = fields.get(x,y);
    for(int i = 0; i < field->entities->count; i++) {
        entity_t* e = get_entity(field->entities, i);
        if (e->type == ENTITY_PLAYER) {
            death_mark_player(e->player, "Hit by a thrown mine"); 
            kill = 1; 
        }
    }

    if (!kill) {
        set_overlay(field, MINE);
        set_color_overlay(field, FORE, WHITE);
        
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
    ps->stack[ps->sp++] = INSTR_SUCCESS;
    return 1;
}

int instr_move(player_state* ps) { 
    direction d = (direction)ps->stack[--ps->sp];

    if (ps->location.type == VEHICLE_LOCATION) {
        return get_vehicle_move_func(ps->location.vehicle->type)(ps->location.vehicle, d);
    }

    int x, y;
    location_coords(ps->location, &x, &y);
    if (fields.properties(x,y) & PROP_OBSTRUCTION) {
        ps->stack[ps->sp++] = INSTR_OBSTRUCTED;
        return 0;
    }
    move_coord(&x, &y, d, 1);
    if(!in_bounds(x,y)) {
        ps->stack[ps->sp++] = INSTR_OUT_OF_BOUNDS;
        return 0; 
    }
    if(fields.properties(x,y) & PROP_OBSTRUCTION) {
        ps->stack[ps->sp++] = INSTR_OBSTRUCTED;
        return 0; 
    }

    move_player_to_location(ps, field_location_from_coords(x,y));

    ps->stack[ps->sp++] = INSTR_SUCCESS;
    return 1;
}

int instr_chop(player_state* ps) {
    direction d = (direction)ps->stack[--ps->sp];
    int x, y;
    location_coords(ps->location, &x, &y);
    move_coord(&x, &y, d, 1);
    if (!in_bounds(x,y)) {
        ps->stack[ps->sp++] = INSTR_OBSTRUCTED;
        return 0; 
    }

    field_state* field = fields.get(x,y);

    if (field->type == EMPTY)
        for(int i = 0; i < field->entities->count; i++) {
            entity_t* e = get_entity(field->entities, i);
            if (e->type == ENTITY_PLAYER) {
                death_mark_player(e->player, "Chopped to pieces");
                break;
            }
        }
    else if (field->type == TREE) {
        fields.remove_field(field);
        add_resource(&ps->resources, R_Wood, _gr->settings.chop.wood_gain);
        int got_sapling = rand() % 100 > _gr->settings.chop.sapling_chance;
        if (got_sapling) add_resource(&ps->resources, R_Sapling, 1);
    }

    ps->stack[ps->sp++] = INSTR_SUCCESS;
    return 1;
}

int instr_trench(player_state* ps) {
    direction d = (direction)ps->stack[--ps->sp];
    int x, y;
    location_coords(ps->location, &x, &y);
    move_coord(&x, &y, d, 1);
    if (!in_bounds(x, y)) {
        ps->stack[ps->sp++] = INSTR_OUT_OF_BOUNDS;
        return 0;
    }

    field_state* field = fields.get(x,y);
    switch (field->type) {
        case EMPTY: {
            fields.build.trench(field);
            ps->stack[ps->sp++] = INSTR_SUCCESS;
            return 1;
        }
    }
    ps->stack[ps->sp++] = INSTR_INVALID_TARGET;
    return 0;
}

int instr_fortify(player_state* ps) {
    if(!spend_resource(&ps->resources, R_Wood, _gr->settings.fortify.cost)) {
        ps->stack[ps->sp++] = INSTR_MISSING_RESOURCE;
        return 0;
    } 
    int x, y;
    location_coords(ps->location, &x, &y);
    direction d = (direction)ps->stack[--ps->sp];
    move_coord(&x, &y, d, 1);
    int result = fields.fortify_field(fields.get(x,y));
    ps->stack[ps->sp++] = result;
    return result;
}

int instr_bomb(player_state* ps) {
    int p = ps->stack[--ps->sp];
    direction d = (direction)ps->stack[--ps->sp];
    if(!spend_resource(&ps->resources, R_Explosive, 1)) {
        ps->stack[ps->sp++] = INSTR_MISSING_RESOURCE;
        return 0;
    }

    p = limit_range(p, _gr->settings.bomb.range);
    int x, y;
    location_coords(ps->location, &x, &y);
    move_coord(&x, &y, d, p);

    if (!in_bounds(x,y)) {
        ps->stack[ps->sp++] = INSTR_OUT_OF_BOUNDS;
        return 0;
    }

    field_state* field = fields.get(x,y);

    bomb_event_args* args = malloc(sizeof(bomb_event_args));
    args->x = x;
    args->y = y;
    args->player_id = ps->id;
    add_event(_gs->events, PHYSICAL_EVENT, events.bomb, args);
    set_color_overlay(field, FORE, RED);
    set_overlay(field, TARGET);

    ps->stack[ps->sp++] = INSTR_SUCCESS;
    return 1;
}

int instr_write(player_state* ps) {
    location_field(ps->location)->player_data = ps->stack[--ps->sp];
    ps->stack[ps->sp++] = INSTR_SUCCESS;
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
    if (!spend_resource(&ps->resources, R_Mana, _gr->settings.projection.cost)) {
        ps->stack[ps->sp++] = INSTR_MISSING_RESOURCE;
        return 0;
    }  
 
    if (
        !ps->is_original_player
        || ps->location.type == VEHICLE_LOCATION && !(get_vehicle_capacity(ps->location.vehicle->type) > ps->location.vehicle->entities->count)
    ) {
        ps->stack[ps->sp++] = INSTR_ERROR;
        return 0;
    }  

    player_state* projection = copy_player_state(ps);
    copy_resource_registry(&ps->resources, &projection->resources);

    add_player(_gs->players, projection);
    if (projection->team)
        projection->team->members_alive++;

    move_player_to_location(projection, ps->location);

    player_event_args* args = malloc(sizeof(player_event_args));
    args->player_id = projection->id;
    add_event(_gs->events, MAGICAL_EVENT, events.projection_upkeep, args);

    ps->stack[ps->sp++] = INSTR_SUCCESS;
    projection->stack[projection->sp++] = INSTR_ERROR;
    return 0;
}

int instr_freeze(player_state* ps) {
    int p = ps->stack[--ps->sp];
    direction d = (direction)ps->stack[--ps->sp];
    if(!spend_resource(&ps->resources, R_Mana, _gr->settings.freeze.cost)) {
        ps->stack[ps->sp++] = INSTR_MISSING_RESOURCE;
        return 0;
    }

    p = limit_range(p, _gr->settings.freeze.range);
    int x, y;
    location_coords(ps->location, &x, &y);
    move_coord(&x, &y, d, p);

    if (!in_bounds(x,y)) {
        ps->stack[ps->sp++] = INSTR_OUT_OF_BOUNDS;
        return 0;
    }
    field_state* field = fields.get(x,y);
    if(field->type == ICE_BLOCK) {
        field->data->ice_block.melt_event_args->remaining += _gr->settings.freeze.refreeze;
        ps->stack[ps->sp++] = INSTR_SUCCESS;
        return 0;
    }

    ice_block_melt_event_args* args = malloc(sizeof(ice_block_melt_event_args));
    args->x = x;
    args->y = y;
    args->player_id = ps->id;
    args->remaining = _gr->settings.freeze.duration;
    add_event(_gs->events, NONE_EVENT, events.ice_block_melt, args);

    set_color_overlay(field, FORE, ICE_BLUE);
    set_color_overlay(field, BACK, BLACK);
    set_overlay(field, SNOWFLAKE);
    print_board(); wait(1);

    field_data* new_data = malloc(sizeof(field_data));
    new_data->ice_block.inner = field->data;
    new_data->ice_block.inner_type = field->type;
    new_data->ice_block.melt_event_args = args;
    field->type = ICE_BLOCK;
    field->data = new_data;

    ps->stack[ps->sp++] = INSTR_SUCCESS;
    return 1;
}

int instr_fireball(player_state* ps) {
    direction d = (direction)ps->stack[--ps->sp];
    if(!spend_resource(&ps->resources, R_Mana, _gr->settings.fireball.cost)) {
        ps->stack[ps->sp++] = INSTR_MISSING_RESOURCE;
        return 0;
    }

    int x, y;
    location_coords(ps->location, &x, &y);

    int limit = _gr->settings.fireball.range;
    while(limit--) {
        move_coord(&x, &y, d, 1);
        if (!in_bounds(x,y)) break;

        _log(DEBUG, ": %i, %i", x, y);

        field_state* field = fields.get(x,y);
        unsigned int props = fields.properties_of_field(field);

        if (props & PROP_OBSTRUCTION) {
            fields.damage_field(field, FIRE_DMG | PROJECTILE_DMG, "Hit by a fireball");
            ps->stack[ps->sp++] = INSTR_SUCCESS;
            return 1;
        }
        else if ((props & PROP_PLAYER) && !((props & PROP_COVER) || (props & PROP_SHELTER))) {
            for(int i = 0; i < field->entities->count; i++) {
                entity_t* e = get_entity(field->entities, i);
                if (e->type = ENTITY_PLAYER) {
                    death_mark_player(e->player, "Hit by a fireball");
                    break;
                }
            }
            ps->stack[ps->sp++] = INSTR_SUCCESS;
            return 1;
        }

        set_color_overlay(field, FORE, RED);
        set_overlay(field, FILLED_CIRCLE);
        print_board(); wait(0.05);
    }

    ps->stack[ps->sp++] = INSTR_SUCCESS;
    return 0;
}

int instr_meditate(player_state* ps) {
    add_resource(&ps->resources, R_Mana, _gr->settings.meditate.amount);
    field_state* field = location_field(ps->location);
    field->background_color = MAGIC_PURPLE;
    field->overlays |= BACKGROUND_COLOR_OVERLAY;

    ps->stack[ps->sp++] = INSTR_SUCCESS;
    return 1;
}

int instr_dispel(player_state* ps) {
    direction d = (direction)ps->stack[--ps->sp];
    if(!spend_resource(&ps->resources, R_Mana, _gr->settings.dispel.cost)) {
        ps->stack[ps->sp++] = INSTR_MISSING_RESOURCE;
        return 0;
    } 

    int x, y;
    location_coords(ps->location, &x, &y);
    move_coord(&x, &y, d, 1);
    if (!in_bounds(x,y)) {
        ps->stack[ps->sp++] = INSTR_OUT_OF_BOUNDS;
        return 0;
    }

    field_state* field = fields.get(x,y);
    remove_events_of_kind(field->enter_events, MAGICAL_EVENT);
    remove_events_of_kind(field->exit_events, MAGICAL_EVENT);

    set_overlay(field, LARGE_X);
    set_color_overlay(field, FORE, MAGIC_PURPLE);
    print_board(); wait(0.5);
    ps->stack[ps->sp++] = INSTR_SUCCESS;
    return 1;
}

int instr_disarm(player_state* ps) {
    direction d = (direction)ps->stack[--ps->sp];

    int x, y;
    location_coords(ps->location, &x, &y);
    move_coord(&x, &y, d, 1);
    if (!in_bounds(x,y)) {
        ps->stack[ps->sp++] = INSTR_OUT_OF_BOUNDS;
        return 0;
    }

    field_state* field = fields.get(x,y);
    remove_events_of_kind(field->enter_events, PHYSICAL_EVENT);
    remove_events_of_kind(field->exit_events, PHYSICAL_EVENT);

    set_overlay(field, LARGE_X);
    print_board(); wait(0.5);
    ps->stack[ps->sp++] = INSTR_SUCCESS;
    return 1;
}

int instr_mana_drain(player_state* ps) {
    direction d = (direction)ps->stack[--ps->sp];
    if(!spend_resource(&ps->resources, R_Mana, _gr->settings.mana_drain.cost)) {
        ps->stack[ps->sp++] = INSTR_MISSING_RESOURCE;
        return 0;
    }

    int x, y;
    location_coords(ps->location, &x, &y);
    move_coord(&x, &y, d, 1);
    if (!in_bounds(x,y)) {
        ps->stack[ps->sp++] = INSTR_OUT_OF_BOUNDS;
        return 0;
    }

    field_state* field = fields.get(x,y);

    set_overlay(field, EMPTY_DIAMOND);
    set_color_overlay(field, FORE, MAGIC_PURPLE);
    add_event(
        field->enter_events,
        MAGICAL_EVENT,
        events.mana_drain, NULL
    );
    ps->stack[ps->sp++] = INSTR_SUCCESS;
    return 1;
}

int instr_pager_set(player_state* ps) {
    int new_channel = ps->stack[--ps->sp];
    int change = new_channel == ps->pager_channel;
    ps->pager_channel = new_channel;
    ps->stack[ps->sp++] = change;
    return 0;
}

int instr_pager_read(player_state* ps) {
    if (ps->sp + 1 >= _gr->stack_size) {
        death_mark_player(ps, stack_overflow_msg);
        return 0;
    }

    if (ps->pager_msgs->count <= 0) 
        ps->stack[ps->sp++] = INSTR_ERROR;
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
    int hits = 0;
    for(int i = 0; i < _gs->players->count; i++) {
        player_state* other = array_list.get(_gs->players, i);
        if (other->id != ps->id && other->pager_channel == channel) {
            hits++;
            array_list.add(other->pager_msgs, *(void**)msg_bypass);
        }
    }
    ps->stack[ps->sp++] = hits > 0 ? INSTR_SUCCESS : INSTR_ERROR;
    return 0;
}

int instr_wall(player_state* ps) {
    direction d = (direction)ps->stack[--ps->sp];
    int x, y;
    location_coords(ps->location, &x, &y);
    move_coord(&x, &y, d, 1);

    if (!spend_resource(&ps->resources, R_Wood, _gr->settings.wall.cost)) {
        ps->stack[ps->sp++] = INSTR_MISSING_RESOURCE;
        return 0;
    }

    if (!in_bounds(x, y)) {
        ps->stack[ps->sp++] = INSTR_OUT_OF_BOUNDS;
        return 0;
    }

    field_state* field = fields.get(x,y);
    switch (field->type) {
        case EMPTY: {
            fields.build.wall(field);
            ps->stack[ps->sp++] = INSTR_SUCCESS;
            return 1;
        }
    }

    ps->stack[ps->sp++] = INSTR_ERROR;
    return 0;
}

int instr_plant_tree(player_state* ps) {
    direction d = (direction)ps->stack[--ps->sp];
    
    if (!spend_resource(&ps->resources, R_Sapling, 1)) {
        ps->stack[ps->sp++] = INSTR_MISSING_RESOURCE;
        return 0;
    } 
    
    int x, y;
    location_coords(ps->location, &x, &y);
    move_coord(&x, &y, d, 1);
    if (!in_bounds(x, y)) {
        ps->stack[ps->sp++] = INSTR_OUT_OF_BOUNDS;
        return 0;
    }

    field_countdown_args* args = malloc(sizeof(field_countdown_args));
    args->x = x;
    args->y = y;
    args->player_id = ps->id;
    args->remaining = _gr->settings.plant_tree.delay;

    add_event(_gs->events, PHYSICAL_EVENT, events.tree_grow, args);
    ps->stack[ps->sp++] = INSTR_SUCCESS;
    return 0;
}

int instr_bridge(player_state* ps) {
    direction d = (direction)ps->stack[--ps->sp];
    
    if (!spend_resource(&ps->resources, R_Wood, _gr->settings.bridge.cost)) {
        ps->stack[ps->sp++] = INSTR_MISSING_RESOURCE;
        return 0;
    }

    int x, y;
    location_coords(ps->location, &x, &y);
    move_coord(&x, &y, d, 1);
    if (!in_bounds(x, y)) {
        ps->stack[ps->sp++] = INSTR_OUT_OF_BOUNDS;
        return 0;
    }

    field_state* field = fields.get(x,y);
    if (field->type != OCEAN) {
        ps->stack[ps->sp++] = INSTR_INVALID_TARGET;
        return 0;
    }

    remove_events_of_kind(field->enter_events, FIELD_EVENT);
    remove_events_of_kind(field->exit_events, FIELD_EVENT);

    field->type = BRIDGE;
    ps->stack[ps->sp++] = INSTR_SUCCESS;
    return 1;
}

int instr_collect(player_state* ps) {
    direction d = (direction)ps->stack[--ps->sp];
    int x, y;
    location_coords(ps->location, &x, &y);
    move_coord(&x, &y, d, 1);

    if (!in_bounds(x, y)) {
        ps->stack[ps->sp++] = INSTR_OUT_OF_BOUNDS;
        return 0;
    }
    field_state* field = fields.get(x,y);

    int success = 0;
    switch (field->type) {
        case TREE:
            add_resource(&ps->resources, R_Sapling, 1);
            success = 1;
            break;
        case MINE_SHAFT:
            add_resource(&ps->resources, R_Metal, 1);
            success = 1;
            break;
        case CLAY:
            switch (field->data->clay_pit.amount) {
                case 0: {
                    fields.remove_field(field);
                    add_resource(&ps->resources, R_Clay, 1);
                    success = 1;
                    break;
                }
                default: {
                    int collected = field->data->clay_pit.amount > _gr->settings.clay_pit.collect_max ? _gr->settings.clay_pit.collect_max : field->data->clay_pit.amount;
                    field->data->clay_pit.amount -= collected;
                    add_resource(&ps->resources, R_Clay, collected);
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
    _log(DEBUG, msg);
    ps->stack[ps->sp++] = INSTR_SUCCESS;
    return 0;
}

int instr_mount(player_state* ps) {
    int d = ps->stack[--ps->sp];
    int x, y;
    location_coords(ps->location, &x, &y);
    move_coord(&x, &y, d, 1);

    if (!in_bounds(x,y)) {
        ps->stack[ps->sp++] = INSTR_OUT_OF_BOUNDS;
        return 0;
    } 
    field_state* field = fields.get(x,y);
    
    vehicle_state* vehicle = NULL;
    for(int i = 0; i < field->entities->count; i++) {
        entity_t* e = get_entity(field->entities, i);
        if (e->type == ENTITY_VEHICLE) 
        vehicle = e->vehicle;
    }
    if (vehicle == NULL)  {
        ps->stack[ps->sp++] = INSTR_INVALID_TARGET;
        return 0;
    }

    if (!(vehicle->entities->count < get_vehicle_capacity(vehicle->type))) {
        ps->stack[ps->sp++] = INSTR_INVALID_TARGET;
        return 0;
    } 

    remove_entity(location_field(ps->location)->entities, ps->id);
    ps->location = vehicle_location(vehicle);
    add_entity(vehicle->entities, entity.of_player(ps));
    ps->stack[ps->sp++] = INSTR_SUCCESS;
    return 1;
}

int instr_dismount(player_state* ps) {
    int d = ps->stack[--ps->sp];
    if (!ps->location.type == VEHICLE_LOCATION) {
        ps->stack[ps->sp++] = INSTR_ERROR;
        return 0;
    }
    int x, y;
    location_coords(ps->location, &x, &y);
    move_coord(&x, &y, d, 1);

    if (!in_bounds(x,y)) {
        ps->stack[ps->sp++] = INSTR_OUT_OF_BOUNDS;
        return 0;
    }

    remove_entity(ps->location.vehicle->entities, ps->id);
    field_state* field = fields.get(x,y);
    location prev_loc = ps->location;
    ps->location = field_location_from_field(field);
    add_entity(field->entities, entity.of_player(ps));
    update_events(entity.of_player(ps), fields.get(x,y)->enter_events, (situation){ .type = MOVEMENT_SITUATION, .movement.loc = prev_loc });

    ps->stack[ps->sp++] = INSTR_SUCCESS;
    return 1;
}

int instr_boat(player_state* ps) {
    int d = ps->stack[--ps->sp];
    
    if (!spend_resource(&ps->resources, R_Wood, _gr->settings.boat.cost)) {
        ps->stack[ps->sp++] = INSTR_MISSING_RESOURCE;
        return 0;
    }
    
    int x, y;
    location_coords(ps->location, &x, &y);
    move_coord(&x, &y, d, 1);
    if (!in_bounds(x,y)) {
        ps->stack[ps->sp++] = INSTR_OUT_OF_BOUNDS;
        return 0;
    }
    field_state* field = fields.get(x,y);
    if (field->type != OCEAN) {
        ps->stack[ps->sp++] = INSTR_INVALID_TARGET;
        return 0;
    }

    

    vehicle_state* boat = malloc(sizeof(vehicle_state));
    boat->id = _gs->id_counter++;
    boat->entities = array_list.create(get_vehicle_capacity(VEHICLE_BOAT));
    boat->location = field_location_from_field(field);
    boat->type = VEHICLE_BOAT;
    boat->destroy = 0;

    zero_out_registry(&boat->resources);
    set_resource_entry(&boat->resources, R_Wood, 0, _gr->settings.boat.wood_cap);
    set_resource_entry(&boat->resources, R_Clay, 0, _gr->settings.boat.clay_cap);
    set_resource_entry(&boat->resources, R_Ammo, 0, _gr->settings.boat.ammo_cap);
    set_resource_entry(&boat->resources, R_Sapling, 0, _gr->settings.boat.sapling_cap);
    set_resource_entry(&boat->resources, R_BearTrap, 0, _gr->settings.boat.beartrap_cap);
    set_resource_entry(&boat->resources, R_Explosive, 0, _gr->settings.boat.explosive_cap);
    set_resource_entry(&boat->resources, R_Metal, 0, _gr->settings.boat.metal_cap);

    add_entity(field->entities, entity.of_vehicle(boat));
    ps->stack[ps->sp++] = INSTR_SUCCESS;
    return 1;
}

int instr_bear_trap(player_state* ps) {
    direction d = (direction)ps->stack[--ps->sp];
    if(!spend_resource(&ps->resources, R_BearTrap, 1)) {
        ps->stack[ps->sp++] = INSTR_MISSING_RESOURCE;
        return 0;
    }

    int x, y;
    location_coords(ps->location, &x, &y);
    move_coord(&x, &y, d, 1);
    if (!in_bounds(x,y)) {
        ps->stack[ps->sp++] = INSTR_OUT_OF_BOUNDS;
        return 0;
    }

    field_state* field = fields.get(x,y);
    field->symbol = BEAR_TRAP;
    field->foreground_color = WHITE;
    add_event(
        field->enter_events,
        PHYSICAL_EVENT,
        events.bear_trap_trigger, NULL
    );

    ps->stack[ps->sp++] = INSTR_SUCCESS;
    return 1;
}

int instr_throw_clay(player_state* ps) {
    int p = ps->stack[--ps->sp];
    direction d = (direction)ps->stack[--ps->sp];

    if(!spend_resource(&ps->resources, R_Clay, _gr->settings.throw_clay.cost)) {
        ps->stack[ps->sp++] = INSTR_MISSING_RESOURCE;
        return 0;
    }

    p = limit_range(p, _gr->settings.throw_clay.range);
    
    int x, y;
    location_coords(ps->location, &x, &y);
    
    while (p--) { 
        move_coord(&x, &y, d, 1);
        if (!in_bounds(x,y)) {
            ps->stack[ps->sp++] = INSTR_SUCCESS;
            return 1;
        }

        field_state* field = fields.get(x,y);
        unsigned int props = fields.properties_of_field(field);

        set_overlay(field, FILLED_CIRCLE);
        set_color_overlay(field, FORE, CLAY_BROWN);
        print_board(); wait(0.02);
        
        if (props & PROP_OBSTRUCTION) {
            fields.damage_field(field, KINETIC_DMG | PROJECTILE_DMG, "Got shot");
            ps->stack[ps->sp++] = INSTR_SUCCESS;
            return 1;
        }
        else if ((props & PROP_PLAYER) && !((props & PROP_COVER) || (props & PROP_SHELTER))) {
            for(int i = 0; i < field->entities->count; i++) {
                entity_t* e = get_entity(field->entities, i);
                if (e->type == ENTITY_PLAYER) {
                    death_mark_player(e->player, "Got shot");
                    break;
                }
            }
            ps->stack[ps->sp++] = INSTR_SUCCESS;
            return 1;
        }    
    }

    // clayify field
    field_state* field = fields.get(x,y);
    switch (field->type) {
        case EMPTY:
            fields.build.clay_pit(field);
            break;
        case CLAY:
            if (field->data->clay_pit.amount < _gr->settings.clay_pit.contain_limit)
                field->data->clay_pit.amount++;
            break;
    }

    ps->stack[ps->sp++] = INSTR_SUCCESS;
    return 1;
}

int instr_clay_golem(player_state* ps) {

    if (!spend_resource(&ps->resources, R_Clay, _gr->settings.clay_golem.cost)) {
        ps->stack[ps->sp++] = INSTR_MISSING_RESOURCE;
        return 0;
    }  

    if (
        !ps->is_original_player
        || ps->location.type == VEHICLE_LOCATION && !(get_vehicle_capacity(ps->location.vehicle->type) > ps->location.vehicle->entities->count)
    ) {
        ps->stack[ps->sp++] = INSTR_ERROR;
        return 0;
    }  

    player_state* golem = copy_player_state(ps);
    golem->team = NULL;
    copy_empty_resource_registry(&default_resource_registry, &golem->resources);

    add_player(_gs->players, golem);

    move_player_to_location(golem, ps->location);

    ps->stack[ps->sp++] = INSTR_SUCCESS;
    golem->stack[golem->sp++] = INSTR_ERROR;
    return 0;
}

int instr_drop(player_state* ps) {
    int resource = ps->stack[--ps->sp];
    int drop_amount = ps->stack[--ps->sp];

    int held_amount = peek_resource(&ps->resources, resource);
    if (held_amount == 0) {
        ps->stack[ps->sp++] = INSTR_MISSING_RESOURCE;
        return 0;
    }

    if (drop_amount > held_amount)
        drop_amount = held_amount;

    spend_resource(&ps->resources, resource, drop_amount);

    int success = 0;
    location loc = ps->location;

    switch (loc.type) {
        case FIELD_LOCATION:
            add_resource(&loc.field->resources, resource, drop_amount);
            success = 1;
            break;
        case VEHICLE_LOCATION:
            int remaining_space = remaining_resource_space(&loc.vehicle->resources, resource);
            if (remaining_space >= drop_amount) {
                add_resource(&loc.vehicle->resources, resource, drop_amount);
            }
            else {
                add_resource(&loc.vehicle->resources, resource, remaining_space);
                field_state* field  = location_field(loc);
                add_resource(&field->resources, resource, drop_amount - remaining_space);
            }
            success = 1;
            break;
    }

    ps->stack[ps->sp++] = success;
    return 0;
}

int instr_take(player_state* ps) {
    int resource = ps->stack[--ps->sp];
    int take_amount = ps->stack[--ps->sp];

    int success = 0;
    location loc = ps->location;

    int remaining_space = remaining_resource_space(&ps->resources, resource);

    if (remaining_space == 0) {
        ps->stack[ps->sp++] = INSTR_MISSING_SPACE;
        return 0;
    }

    if (remaining_space != -1 && take_amount > remaining_space)
        take_amount = remaining_space;

    switch (loc.type) {
        case FIELD_LOCATION: {
            int available_resource = peek_resource(&loc.field->resources, resource);
            if (available_resource != 0) {
                if (take_amount > available_resource)
                    take_amount = available_resource;
                spend_resource(&loc.field->resources, resource, take_amount);
                add_resource(&ps->resources, resource, take_amount);
                success = 1;
            }
            break;
        }
        case VEHICLE_LOCATION: {
            int available_resource = peek_resource(&loc.vehicle->resources, resource);
            if (available_resource != 0) {
                if (take_amount > available_resource)
                    take_amount = available_resource;
                spend_resource(&loc.vehicle->resources, resource, take_amount);
                add_resource(&ps->resources, resource, take_amount);
                success = 1;
            }
            break;
        }
    }

    ps->stack[ps->sp++] = success;
    return 0;
}

int instr_mine_shaft(player_state* ps) {
    direction d = (direction)ps->stack[--ps->sp];

    if(!spend_resource(&ps->resources, R_Wood, _gr->settings.mine_shaft.cost)) {
        ps->stack[ps->sp++] = INSTR_MISSING_RESOURCE;
        return 0;
    }

    int x, y;
    location_coords(ps->location, &x, &y);
    move_coord(&x, &y, d, 1);
    if (!in_bounds(x, y)) {
        ps->stack[ps->sp++] = INSTR_OUT_OF_BOUNDS;
        return 0;
    }

    field_state* field = fields.get(x,y);
    switch (field->type) {
        case EMPTY: {
            fields.build.mine_shaft(field);
            ps->stack[ps->sp++] = INSTR_SUCCESS;
            return 1;
        }
    }
    ps->stack[ps->sp++] = INSTR_ERROR;
    return 0;
}

int instr_craft(player_state* ps) {
    int resource = ps->stack[--ps->sp];
    int success = 0;

    switch (resource) {
        case R_Ammo:
            if (spend_resource(&ps->resources, R_Metal, 1)) {
                success = 1;
                add_resource(&ps->resources, R_Ammo, _gr->settings.craft.ammo_per_metal);
            }
            break;
        case R_BearTrap:
            if (spend_resource(&ps->resources, R_Metal, 1)) {
                success = 1;
                add_resource(&ps->resources, R_BearTrap, _gr->settings.craft.beartraps_per_metal);
            }
            break;
    }

    ps->stack[ps->sp++] = success;
    return 0;
}

#pragma endregion

#endif