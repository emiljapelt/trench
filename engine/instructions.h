#ifndef INSTRUCTIONS_H
#define INSTRUCTIONS_H

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
  Instr_Bits =          34,
  Instr_TCall =         35,

  Instr_DEBUG = 99,
} instruction;

// all instructions return 1 if the board should be updated, and 0 if not.

#pragma endregion

#pragma region HELPERS

int global_scope_sp(exec_context* c, int bp, int sp) {
    while (bp != 0) {
        sp = bp - 2;
        bp = c->stack[bp - 2];
    }
    return sp;
}

int is_true(int a) {
    return a > 0 ? 1 : 0;
}

void handle_error(exec_context* c, const char* msg) {
    switch (c->type) {
        case ENTITY_CONTEXT: {
            switch (c->entity->type) {
                case ENTITY_PLAYER: {
                    kill_player(c->entity->player, msg);
                    break;
                }

                case ENTITY_VEHICLE: {
                    break; // Vehicles does execture anything, yet
                }
            }
        }

        case EVENT_CONTEXT: {
            c->event->cancelled = 1;
            _log(ERROR, "Event execution failed: %s", msg);
            break;
        }
    }
}

#pragma endregion

#pragma region LANGUAGE_CORE

int instr_random_int(exec_context* c) { 
    if (c->sp + 1 >= _gr->stack_size) {
        handle_error(c, stack_overflow_msg);
        return 0;
    }

    c->stack[c->sp++] = rand();
    return 0;
}

int instr_place(exec_context* c) {
    if (c->sp + 1 >= _gr->stack_size) {
        handle_error(c, stack_overflow_msg);
        return 0;
    }

    c->stack[c->sp++] = c->program[c->dp];
    c->dp++;

    return 0;
}

int instr_goto(exec_context* c) {
    c->dp = c->program[c->dp];
    return 0;
}

int instr_goto_if(exec_context* c) {
    int v = c->stack[--c->sp];
    if (is_true(v))
        c->dp = c->program[c->dp];
    else 
        c->dp++;
    return 0;
}

int instr_eq(exec_context* c) {
    int v0 = c->stack[--c->sp];
    int v1 = c->stack[--c->sp];
    c->stack[c->sp++] = v0 == v1;
    return 0;
}

int instr_lt(exec_context* c) {
    int v0 = c->stack[--c->sp];
    int v1 = c->stack[--c->sp];
    c->stack[c->sp++] = v0 < v1;
    return 0;
}

int instr_sub(exec_context* c) {
    int v0 = c->stack[--c->sp];
    int v1 = c->stack[--c->sp];
    c->stack[c->sp++] = v1 - v0;
    return 0;
}

int instr_add(exec_context* c) {
    int v0 = c->stack[--c->sp];
    int v1 = c->stack[--c->sp];
    c->stack[c->sp++] = v0 + v1;
    return 0;
}

int instr_mul(exec_context* c) {
    int v0 = c->stack[--c->sp];
    int v1 = c->stack[--c->sp];
    c->stack[c->sp++] = v0 * v1;
    return 0;
}

int instr_div(exec_context* c) { 
    int v0 = c->stack[--c->sp];
    int v1 = c->stack[--c->sp];
    if (v1 == 0) { handle_error(c, div_zero_msg); return 0; }
    c->stack[c->sp++] = v0 / v1;
    return 0;
}

int instr_mod(exec_context* c) {
    int v0 = c->stack[--c->sp];
    int v1 = c->stack[--c->sp];
    if (v1 == 0) { handle_error(c, div_zero_msg); return 0; }
    c->stack[c->sp++] = ((v0%v1) + v1)%v1;
    return 0;
}

int instr_not(exec_context* c) {
    int v = c->stack[--c->sp];
    c->stack[c->sp++] = !is_true(v);
    return 0;
}

int instr_or(exec_context* c) {
    int v0 = c->stack[--c->sp];
    int v1 = c->stack[--c->sp];
    c->stack[c->sp++] = is_true(v0) || is_true(v1);
    return 0;
}

int instr_and(exec_context* c) {
    int v0 = c->stack[--c->sp];
    int v1 = c->stack[--c->sp];
    c->stack[c->sp++] = is_true(v0) && is_true(v1);
    return 0;
}

int instr_move_sp(exec_context* c) {
    int amount = c->program[c->dp++];
    c->sp += amount;
    if (c->sp < 0 || c->sp >= _gr->stack_size) {
        handle_error(c, stack_overflow_msg);
        return 0;
    }
    // Guard? untested

    return 0;
}

int instr_copy(exec_context* c) {
    if (c->sp + 1 >= _gr->stack_size) {
        handle_error(c, stack_overflow_msg);
        return 0;
    }

    c->stack[c->sp] = c->stack[c->sp - 1];
    c->sp++;
    return 0;
}

int instr_swap(exec_context* c) {
    int v0 = c->stack[--c->sp];
    int v1 = c->stack[--c->sp];
    c->stack[c->sp++] = v0;
    c->stack[c->sp++] = v1;
    return 0;
}

int instr_call(exec_context* c) {
    int arg_count = c->stack[--c->sp];
    int func_addr = c->stack[--c->sp];

    if (func_addr == 0) {
        handle_error(c, null_call_msg);
        return 0;
    }

    // args + stack frame note
    if (c->sp + arg_count + 2 >= _gr->stack_size) {
        handle_error(c, stack_overflow_msg);
        return 0;
    }

    if (func_addr < 0) { // builtin handling
        if (is_action(func_addr) && !use_resource(1, &c->actions)) {
            c->dp--;
            c->steps = 0;
            c->stack[c->sp++] = func_addr;
            c->stack[c->sp++] = arg_count;
            return 0;
        }
        return handle_builtin_function(c, func_addr);
    }

    int args[arg_count];
    memcpy(args, &c->stack[c->sp - arg_count], sizeof(int) * arg_count);

    c->sp -= arg_count;
    int old_bp = c->bp;
    int old_dp = c->dp;

    c->stack[c->sp++] = old_bp;
    c->stack[c->sp++] = old_dp;
    c->bp = c->sp;
    
    memcpy(&c->stack[c->sp], args, sizeof(int) * arg_count);
    c->sp += arg_count;
    c->dp = func_addr;
    _log(DEBUG, "call addr: %i, args: %i", func_addr, arg_count);
    return 0;
}

// Guard
int instr_return(exec_context* c) {
    int size = c->program[c->dp++];
    int ret_start = c->sp - size;
    int old_dp = c->stack[c->bp - 1];
    int old_bp = c->stack[c->bp - 2];

    c->dp = old_dp;
    c->sp = c->bp - 2;
    c->bp = old_bp;

    memcpy(c->stack + c->sp, c->stack + ret_start, sizeof(int) * size);
    c->sp += size;
    return 0;
}

int instr_declare(exec_context* c) {
    int n = c->program[c->dp];
    if (c->sp + n >= _gr->stack_size) {
        handle_error(c, stack_overflow_msg);
        return 0;
    }
    memset(c->stack + c->sp, 0, sizeof(int) * n);
    c->sp += n;
    c->dp++;
    return 0;
}

int instr_index(exec_context* c) {
    int index = c->stack[--c->sp];
    int target = c->stack[--c->sp];
    int size = c->program[c->dp++];
    int take = c->program[c->dp++]; // Not needed anymore??

    if (index < 0 || index >= size || index + take > size) {
        handle_error(c, out_of_bounds_msg);
        return 0;
    }

    //ps->stack[ps->sp++] = target + index * elem_size;
    c->stack[c->sp++] = target + index;

    return 0;
}


int instr_binor(exec_context* c) {
    int v0 = c->stack[--c->sp];
    int v1 = c->stack[--c->sp];
    c->stack[c->sp++] = v0 | v1;
    return 0;
}

int instr_binnot(exec_context* c) {
    int v = c->stack[--c->sp];
    c->stack[c->sp++] = ~v;
    return 0;
}

int instr_binand(exec_context* c) {
    int v0 = c->stack[--c->sp];
    int v1 = c->stack[--c->sp];
    c->stack[c->sp++] = v0 & v1;
    return 0;
}

int instr_bits(exec_context* c) {
    int v = c->stack[--c->sp];
    int count = 0;
    while (v > 0) {           // until all bits are zero
        if ((v & 1) == 1)     // check lower bit
            count++;
        v >>= 1;              // shift bits, removing lower bit
    }

    c->stack[c->sp++] = count;
    return 0;
}

int instr_load(exec_context* c, int base) {
    int addr = c->stack[--c->sp];
    int size = c->program[c->dp++];

    _log(DEBUG, "LOAD:: base: %i, addr: %i, size: %i", base, addr, size);

    if (c->sp + size >= _gr->stack_size) {
        handle_error(c, stack_overflow_msg);
        return 0;
    }

    memcpy(c->stack + c->sp, c->stack + addr + base, sizeof(int) * size);

    c->sp += size;
    
    return 0;
}

int instr_bp(exec_context* c) {
    c->stack[c->sp++] = c->bp;
    return 0;
}

int instr_store(exec_context* c, int base) {
    int addr = c->stack[--c->sp];
    int size = c->program[c->dp++];

    _log(DEBUG, "STORE:: base: %i, addr: %i, size: %i", base, addr, size);

    c->sp -= size;

    memcpy(c->stack + addr + base, c->stack + c->sp, sizeof(int) * size);

    return 0;
}

int instr_extract(exec_context* c) {
    int index = c->stack[--c->sp];
    int size = c->program[c->dp++];
    int take = c->program[c->dp++];

    _log(DEBUG, "EXTRACT:: index: %i, size: %i, take: %i", index, size, take);

    if (take > size || index < 0 || index + take > size) {
        handle_error(c, stack_overflow_msg);
        return 0;
    }

    c->sp -= size;

    memcpy(c->stack + c->sp, c->stack + c->sp + index, sizeof(int) * take);

    c->sp += take;

    return 0;
}

// TODO: Renumber
typedef enum {
    META_ENTITY_X =     0,
    META_ENTITY_Y =     1,
    META_ENTITY_ID =    2,
    META_ROUND =        5,
    META_ACTIONS =      6,
} meta_value;

int instr_meta(exec_context* c) {
    meta_value meta = (meta_value)c->program[c->dp++];

    if (c->sp + 1 >= _gr->stack_size) {
        handle_error(c, stack_overflow_msg);
        return 0;
    }

    // TODO: Reintroduce some player related 
    switch (meta) {
        case META_ENTITY_X: {
            if (c->type == ENTITY_CONTEXT) {
                int x, y;
                if (location_coords(c->entity->location, &x, &y))
                    c->stack[c->sp++] = x;
                else 
                    c->stack[c->sp++] = -1;
            }
            else 
                c->stack[c->sp++] = -1;
            break;
        }
        case META_ENTITY_Y: {
            if (c->type == ENTITY_CONTEXT) {
                int x, y;
                if (location_coords(c->entity->location, &x, &y))
                    c->stack[c->sp++] = y;
                else 
                    c->stack[c->sp++] = -1;
            }
            else 
                c->stack[c->sp++] = -1;
            break;
        }
        case META_ENTITY_ID:
            if (c->type == ENTITY_CONTEXT)
                c->stack[c->sp++] = c->entity->id;
            else 
                c->stack[c->sp++] = -1;
            break;
        /*case META_BOARD_WIDTH: // Unused
            c->stack[c->sp++] = _gs->map_width;
            break;
        case META_BOARD_HEIGHT: // Unused
            c->stack[c->sp++] = _gs->map_height;
            break;*/
        case META_ROUND:
            c->stack[c->sp++] = _gs->round;
            break;
        case META_ACTIONS:
            c->stack[c->sp++] = c->actions;
            break;
        default:
            c->stack[c->sp++] = 0;
            break;
    }

    return 0;
}

int instr_tcall(exec_context* c) {
    int arg_count = c->stack[--c->sp];
    int func_addr = c->stack[--c->sp];

    if (func_addr == 0) {
        handle_error(c, null_call_msg);
        return 0;
    }

    // Check the check for correctness :)
    if (c->bp + arg_count + 1 >= _gr->stack_size) {
        handle_error(c, stack_overflow_msg);
        return 0;
    }

    if (func_addr < 0) { // builtin handling
        if (is_action(func_addr) && !use_resource(1, &c->actions)) {
            c->dp--;
            c->steps = 0;
            c->stack[c->sp++] = func_addr;
            c->stack[c->sp++] = arg_count;
            return 0;
        }
        return handle_builtin_function(c, func_addr);
    }

    int args[arg_count];
    memcpy(args, &c->stack[c->sp - arg_count], sizeof(int) * arg_count);

    c->sp = c->bp;
    memcpy(&c->stack[c->sp], args, sizeof(int) * arg_count);
    c->sp += arg_count;
    c->dp = func_addr;
    _log(DEBUG, "tcall addr: %i, args: %i", func_addr, arg_count);
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