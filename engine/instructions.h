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
  Instr_Add =           0,
  Instr_Sub =           1,
  Instr_Mul =           2,
  Instr_And =           3,
  Instr_Or =            4,
  Instr_Eq =            5,
  Instr_Not =           6,
  Instr_Lt =            7,
  Instr_Div =           8,
  Instr_Mod =           9,
  Instr_BinOr =         10,
  Instr_BinNot =        11,
  Instr_BinAnd =        12,
  Instr_Random =        13,
  Instr_RandomSet =     14,
  Instr_GoToIf =        15,
  Instr_GoTo =          16,
  Instr_Wait =          17,
  Instr_Pass =          18,
  Instr_Call =          19,
  Instr_Return =        20,
  Instr_Declare =       21,
  Instr_Place =         22,
  Instr_Swap =          23,
  Instr_Copy =          24,
  Instr_MoveSP =        25,
  Instr_BP =            26,
  Instr_Index =         27,
  Instr_Extract =       28,
  Instr_LoadGlobal =    29,
  Instr_StoreGlobal =   30,
  Instr_LoadLocal =     31,
  Instr_StoreLocal =    32,
  Instr_Meta =          33,

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

int is_true(int a) {
    return a > 0 ? 1 : 0;
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

int instr_goto(player_state* ps) {
    ps->dp = ps->directive[ps->dp];
    return 0;
}

int instr_goto_if(player_state* ps) {
    int v = ps->stack[--ps->sp];
    if (is_true(v))
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
    ps->stack[ps->sp++] = !is_true(v);
    return 0;
}

int instr_or(player_state* ps) {
    int v0 = ps->stack[--ps->sp];
    int v1 = ps->stack[--ps->sp];
    ps->stack[ps->sp++] = is_true(v0) || is_true(v1);
    return 0;
}

int instr_and(player_state* ps) {
    int v0 = ps->stack[--ps->sp];
    int v1 = ps->stack[--ps->sp];
    ps->stack[ps->sp++] = is_true(v0) && is_true(v1);
    return 0;
}

int instr_move_sp(player_state* ps) {
    int amount = ps->directive[ps->dp++];
    _log(DEBUG, "MOVE SP: %i", amount);
    ps->sp += amount;
    // Guard?

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
    int size = ps->directive[ps->dp++];
    int ret_start = ps->sp - size;
    int old_dp = ps->stack[ps->bp - 1];
    int old_bp = ps->stack[ps->bp - 2];

    ps->dp = old_dp;
    ps->sp = ps->bp - 2;
    ps->bp = old_bp;

    memcpy(ps->stack + ps->sp, ps->stack + ret_start, sizeof(int) * size);
    ps->sp += size;

    //ps->stack[ps->sp++] = ret;
    //_log(DEBUG, "RETURN: ret:%i old_dp: %i, old_bp: %i", ret, old_dp, old_bp);
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
    int index = ps->stack[--ps->sp];
    int target = ps->stack[--ps->sp];
    int array_size = ps->directive[ps->dp++];
    int elem_size = ps->directive[ps->dp++];

    _log(DEBUG, "INDEX:: index: %i, target: %i, array_size: %i, elem_size: %i", index, target, array_size, elem_size);

    if (index < 0 || index >= array_size) {
        death_mark_player(ps, out_of_bounds_msg);
        return 0;
    }

    ps->stack[ps->sp++] = target + index * elem_size;

    return 0;
}


int instr_binor(player_state* ps) {
    int v0 = ps->stack[--ps->sp];
    int v1 = ps->stack[--ps->sp];
    ps->stack[ps->sp++] = v0 | v1;
    return 0;
}

int instr_binnot(player_state* ps) {
    int v = ps->stack[--ps->sp];
    ps->stack[ps->sp++] = ~v;
    return 0;
}

int instr_binand(player_state* ps) {
    int v0 = ps->stack[--ps->sp];
    int v1 = ps->stack[--ps->sp];
    ps->stack[ps->sp++] = v0 & v1;
    return 0;
}



/*
Maybe loading negative addresses should work like functions do.
Such that builtin variables wont need to polute the ISA.

But... CISC or RISC ???
- Negative addresses reduce the ISA, but increases instructions used.
- Current solution increase ISA, but reduces instructions used.
*/

int instr_load(player_state* ps, int base) {
    int addr = ps->stack[--ps->sp];
    int size = ps->directive[ps->dp++];

    _log(DEBUG, "LOAD:: base: %i, addr: %i, size: %i", base, addr, size);

    if (ps->sp + size >= _gr->stack_size) {
        death_mark_player(ps, stack_overflow_msg);
        return 0;
    }

    memcpy(ps->stack + ps->sp, ps->stack + addr + base, sizeof(int) * size);

    ps->sp += size;
    
    return 0;
}

int instr_bp(player_state* ps) {
    ps->stack[ps->sp++] = ps->bp;
    return 0;
}

int instr_store(player_state* ps, int base) {
    int addr = ps->stack[--ps->sp];
    int size = ps->directive[ps->dp++];

    _log(DEBUG, "STORE:: base: %i, addr: %i, size: %i", base, addr, size);

    ps->sp -= size;

    memcpy(ps->stack + addr + base, ps->stack + ps->sp, sizeof(int) * size);

    return 0;
}

int instr_extract(player_state* ps) {
    int index = ps->stack[--ps->sp];
    int size = ps->directive[ps->dp++];
    int take = ps->directive[ps->dp++];

    _log(DEBUG, "EXTRACT:: index: %i, size: %i, take: %i", index, size, take);

    if (take > size || index < 0 || index + take > size) {
        death_mark_player(ps, out_of_bounds_msg);
        return 0;
    }

    ps->sp -= size;

    memcpy(ps->stack + ps->sp, ps->stack + ps->sp + index, sizeof(int) * take);

    ps->sp += take;

    return 0;
}

typedef enum {
    META_PLAYER_X =     0,
    META_PLAYER_Y =     1,
    META_PLAYER_ID =    2,
    META_BOARD_WIDTH =  3,
    META_BOARD_HEIGHT = 4,
    META_ROUND =        5,
} meta_value;

int instr_meta(player_state* ps) {
    meta_value meta = (meta_value)ps->directive[ps->dp++];

    if (ps->sp + 1 >= _gr->stack_size) {
        death_mark_player(ps, stack_overflow_msg);
        return 0;
    }

    switch (meta) {
        case META_PLAYER_X: {
            int x, y;
            location_coords(ps->entity->location, &x, &y);
            ps->stack[ps->sp++] = x;
            break;
        }
        case META_PLAYER_Y: {
            int x, y;
            location_coords(ps->entity->location, &x, &y);
            ps->stack[ps->sp++] = y;
            break;
        }
        case META_PLAYER_ID:
            ps->stack[ps->sp++] = ps->entity->id;
            break;
        case META_BOARD_WIDTH:
            ps->stack[ps->sp++] = _gs->map_width;
            break;
        case META_BOARD_HEIGHT:
            ps->stack[ps->sp++] = _gs->map_height;
            break;
        case META_ROUND:
            ps->stack[ps->sp++] = _gs->round;
            break;
        default:
            ps->stack[ps->sp++] = 0;
            break;
    }

    return 0;
}

#pragma endregion

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