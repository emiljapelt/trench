#ifndef EVENTS_H
#define EVENTS_H

#include "player.h"
#include "entity.h"

typedef enum situation_type {
    NO_SITUATION,
    MOVEMENT_SITUATION,
} situation_type;

typedef struct situation {
    situation_type type : 8;
    union {
        struct {
            location loc;
        } movement;
    };
} situation;


// Return 1 if the event is finished, otherwise return 0.
typedef int (*event_function)(entity_t* e, void* data, situation situ);

typedef struct player_event_args {
    int player_id;
} player_event_args;

typedef struct bomb_event_args {
    int x;
    int y;
    int player_id;
} bomb_event_args;

typedef struct countdown_args {
    int player_id;
    int remaining;
} countdown_args;

typedef struct field_countdown_args {
    int player_id;
    int remaining;
    int x;
    int y;
} field_countdown_args;

typedef struct field_args {
    int x;
    int y;
} field_args;

typedef struct ice_block_melt_event_args {
    int x;
    int y;
    int player_id;
    int remaining;
} ice_block_melt_event_args;


typedef struct events_namespace {
    event_function bomb;
    event_function mine;
    event_function projection_upkeep;
    event_function ice_block_melt;
    event_function mana_drain;
    event_function tree_grow;
    event_function ocean_drowning;
    event_function bear_trap_trigger;
    event_function bear_trap_escape;
    event_function clay_spread;
} events_namespace;
extern const events_namespace events;

#endif