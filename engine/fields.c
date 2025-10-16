#include "fields.h"

#include <stdlib.h>
#include <stdio.h>

#include "game_state.h"
#include "damage.h"

field_state* get_field(const int x, const int y) {
    return _gs->board + ((y * _gs->board_x) + x);
}

void set_field(const int x, const int y, field_state* f) {
    _gs->board[(y * _gs->board_x) + x] = *f;
}



void build_trench(field_state* field) {
    field_data* data = malloc(sizeof(field_data));
    data->trench.fortified = 0;

    field->type = TRENCH;
    field->data = data;
}

void build_wall(field_state* field) {
    field_data* data = malloc(sizeof(field_data));
    data->wall.fortified = 0;

    field->type = WALL;
    field->data = data;
}

void build_tree(field_state* field) {
    field->type = TREE;
}

void build_clay_pit(field_state* field) {
    field_data* data = malloc(sizeof(field_data));
    data->clay_pit.amount = 0;

    field->type = CLAY;
    field->data = data;

    add_event(field->exit_events, FIELD_EVENT, events.clay_spread, NULL);
}

void build_ocean(field_state* field) {
    field->type = OCEAN;

    add_event(field->enter_events, FIELD_EVENT, events.ocean_drowning, NULL);
}




int has_player(field_state* field) {
    entity_list_t* list = field->entities;
    for(int i = 0; i < list->count; i++)
        if (get_entity(list, i)->type == ENTITY_PLAYER) return 1;
    return 0;
}

int has_trap(field_state* field) {
    return 
        (field->enter_events->count || field->exit_events->count)
            ? 1 : 0;
}

field_properties scan_field(field_type type, field_data* data) {
    field_properties result = {0};
    switch (type) {
        case TRENCH:
            result.is_trench = 1;
            result.cover = 1;
            break;
        case WALL:
            result.is_wall = 1;
            result.obstruction = 1;
            result.flammable = 1;
            break;
        case TREE:
            result.is_tree = 1;
            result.obstruction = 1;
            result.flammable = 1;
            break;
        case CLAY:
            result.is_clay = 1;
            break;
        case OCEAN:
            result.is_ocean = 1;
            break;
        case BRIDGE:
            result.is_bridge = 1;
            break;
        case ICE_BLOCK: {
            result = scan_field(data->ice_block.inner_type, data->ice_block.inner);
            result.is_ice_block = 1;
            result.obstruction = 1;
            break;
        }
        case EMPTY:
        default:
            result.is_empty = 1;
            break;
    }
    return result;
}

field_properties properties_of_field(field_state* field) {
    field_properties result = scan_field(field->type, field->data);
    result.player = has_player(field);
    result.trapped = has_trap(field);
    return result;
}

field_properties properties(const int x, const int y) {
    return properties_of_field(fields.get(x,y));
}

void destroy_field(field_state* field, char* death_msg) {
    remove_events_of_kind(field->enter_events, FIELD_EVENT);
    remove_events_of_kind(field->exit_events, FIELD_EVENT);

    switch (field->type) {
        case OCEAN:
            break;
        case TREE: {
            field->type = EMPTY;
            break;
        }
        case BRIDGE: {
            build_ocean(field);
            break;
        }
        case WALL: {
            if (field->data->wall.fortified) {
                field->data->wall.fortified = 0;
                return;
            }
            field->type = EMPTY;
            free(field->data);
            break;
        }
        case TRENCH:{
            if (field->data->trench.fortified) {
                field->data->trench.fortified = 0;
                return;
            }
            field->type = EMPTY;
            free(field->data);
            break;
        }
        case CLAY:
        case ICE_BLOCK: {
            field->type = EMPTY;
            free(field->data);
            break;
        }
    }
    for(int i = 0; i < field->entities->count; i++) {
        entity_t* entity = get_entity(field->entities, i);
        if (entity->type == ENTITY_PLAYER)
            death_mark_player(entity->player, death_msg);
    }
}

void remove_field(field_state* field) {
    remove_events_of_kind(field->enter_events, FIELD_EVENT);
    remove_events_of_kind(field->exit_events, FIELD_EVENT);

    switch (field->type) {
        case OCEAN:
        case TREE: {
            field->type = EMPTY;
            break;
        }
        case BRIDGE: {
            build_ocean(field);
            break;
        }
        case CLAY:
        case WALL:
        case TRENCH: {
            field->type = EMPTY;
            free(field->data);
            break;
        }
        case ICE_BLOCK: {
            field_data* old_data = field->data;
            field->data = old_data->ice_block.inner;
            field->type = old_data->ice_block.inner_type;
            free(old_data);
            break;
        }
    }
}

void damage_field(field_state* field, damage_t d_type, char* death_msg) {
    field_properties props = properties_of_field(field);
    if (props.flammable && IS_DAMAGE_TYPE(FIRE_DMG, d_type))
        destroy_field(field, death_msg);
    else if (props.meltable && IS_DAMAGE_TYPE(FIRE_DMG, d_type))
        remove_field(field);
}

int fortify_field(field_state* field) {
    switch (field->type) {
        case TRENCH: {
            field->data->trench.fortified = 1;
            return 1;
        }
        case WALL: {
            field->data->wall.fortified = 1;
            return 1;
        }
        default: return 0;
    }
}

const fields_namespace fields = {
    .get = &get_field,
    .set = &set_field,
    .properties = &properties,
    .properties_of_field = &properties_of_field,
    .destroy_field = &destroy_field,
    .damage_field = &damage_field,
    .remove_field = &remove_field,
    .fortify_field = &fortify_field,
    .build = {
        .trench = &build_trench,
        .wall = &build_wall,
        .tree = &build_tree,
        .clay_pit = &build_clay_pit,
        .ocean = &build_ocean,
    },
};