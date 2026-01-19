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
#include "builtins.h"

#pragma region DEFINITIONS

typedef enum {
  Meta_PlayerX = 0,
  Meta_PlayerY = 1,
  Meta_BoardX = 2,
  Meta_BoardY = 3,
  Meta_PlayerID = 4,
  Instr_Add = 5,
  Instr_Sub = 6,
  Instr_Mul = 7,
  Instr_And = 8,
  Instr_Or = 9,
  Instr_Eq = 10,
  Instr_Not = 11,
  Instr_Lt = 12,
  Instr_Div = 13,
  Instr_Mod = 14,
  Instr_Random = 15,
  Instr_RandomSet = 16,
  Instr_Place = 17,
  Instr_Access = 18,
  Instr_Swap = 19,
  Instr_Copy = 20,
  Instr_DecStack = 21,
  Instr_FieldProp = 22,
  Instr_Assign = 23,
  Instr_GoToIf = 24,
  Instr_GoTo = 25,
  Instr_Wait = 26,
  Instr_Pass = 27,
  Instr_Call = 28,
  Instr_Return = 29,
  Instr_Declare = 30,
  Instr_GlobalAccess = 31,
  Instr_GlobalAssign = 32,
  Instr_Index = 33,
  Meta_Round = 34,

  Instr_DEBUG = 99,
} instruction;

// all instructions return 1 if the board should be updated, and 0 if not.

#pragma endregion

#pragma region HELPERS

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
    ps->stack[ps->sp++] = (v > 0) ? 0 : 1;
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

    if (func_addr < 0) { // builtin handling
        if (is_action(func_addr) && !use_resource(1, &ps->remaining_actions)) {
            ps->dp--;
            ps->remaining_steps = 0;
            ps->stack[ps->sp++] = func_addr;
            ps->stack[ps->sp++] = arg_count;
            return 0;
        }
        return handle_builtin_function(ps, func_addr);
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

int meta_player_id(player_state* ps) {
    if (ps->sp + 1 >= _gr->stack_size) {
        death_mark_player(ps, stack_overflow_msg);
        return 0;
    }

    ps->stack[ps->sp++] = ps->id;
    return 0;
}

int meta_round(player_state* ps) {
    if (ps->sp + 1 >= _gr->stack_size) {
        death_mark_player(ps, stack_overflow_msg);
        return 0;
    }

    ps->stack[ps->sp++] = _gs->round;
    return 0;
}


/* NOT USED BUT KEEP IT AROUND FOR NOW */
/*
int instr_debug(player_state* ps) {

    char buf[10000];
    memset(buf, 0, 10000);

    
    for(int i = 0; i < ps->sp; i++) {
        char int_buf[10];
        memset(int_buf, 0, 10);
        sprintf(int_buf, ", %i", ps->stack[i]);
        strcat(buf, int_buf);
    }
    _log(DEBUG, "Player %i, dp: %i, sp: %i", ps->id, ps->dp, ps->sp);
    _log(DEBUG, buf);

    ps->stack[ps->sp++] = 1;
    return 0;
}
*/


#pragma endregion

#endif