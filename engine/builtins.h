#ifndef BUILTINS_H
#define BUILTINS_H

#include "player.h"

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
} builtin_result;

typedef enum {
    BUILTIN_SHOOT = -1,
    BUILTIN_LOOK = -2,
    BUILTIN_SCAN = -3,
    BUILTIN_MINE = -4,
    BUILTIN_MOVE = -5,
    BUILTIN_CHOP = -6,
    BUILTIN_TRENCH = -7,
    BUILTIN_FORTIFY = -8,
    BUILTIN_BOMB = -9,
    BUILTIN_WRITE = -10,
    BUILTIN_READ = -11,
    BUILTIN_PROJECTION = -12,
    BUILTIN_FREEZE = -13,
    BUILTIN_FIREBALL = -14,
    BUILTIN_MEDITATE = -15,
    BUILTIN_DISPEL = -16,
    BUILTIN_DISARM = -17,
    BUILTIN_MANA_DRAIN = -18,
    BUILTIN_PAGER_SET = -19,
    BUILTIN_PAGER_READ = -20,
    BUILTIN_PAGER_WRITE = -21,
    BUILTIN_WALL = -22,
    BUILTIN_PLANT_TREE = -23,
    BUILTIN_BRIDGE = -24,
    BUILTIN_COLLECT = -25,
    BUILTIN_SAY = -26,
    BUILTIN_MOUNT = -27,
    BUILTIN_DISMOUNT = -28,
    BUILTIN_BOAT = -29,
    BUILTIN_BEAR_TRAP = -30,
    BUILTIN_THROW_CLAY = -31,
    BUILTIN_CLAY_GOLEM = -32,
    BUILTIN_DROP = -33,
    BUILTIN_TAKE = -34,
    BUILTIN_MINE_SHAFT = -35,
    BUILTIN_CRAFT = -36,
    BUILTIN_COUNT = -37,
} builtin_func;


int handle_builtin_function(player_state* ps, builtin_func func_addr);
int is_action(builtin_func func_addr);

#endif