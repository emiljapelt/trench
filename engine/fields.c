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
    return field->enter_events->count + field->exit_events->count;
}

unsigned int scan_field(field_type type, field_data* data) {
    unsigned int result = 0;
    switch (type) {
        case TRENCH:
            result = PROP_IS_TRENCH | PROP_COVER;
            break;
        case WALL:
            result = PROP_IS_WALL | PROP_OBSTRUCTION | PROP_FLAMMABLE;
            break;
        case TREE:
            result = PROP_IS_TREE | PROP_OBSTRUCTION | PROP_FLAMMABLE;
            break;
        case CLAY:
            result = PROP_IS_CLAY;
            break;
        case OCEAN:
            result = PROP_IS_OCEAN;
            break;
        case BRIDGE:
            result = PROP_IS_BRIDGE;
            break;
        case ICE_BLOCK: {
            result = PROP_IS_ICE_BLOCK | PROP_OBSTRUCTION | scan_field(data->ice_block.inner_type, data->ice_block.inner);
            break;
        }
        case EMPTY:
        default:
            result = PROP_IS_EMPTY;
            break;
    }
    return result;
}

unsigned int properties_of_field(field_state* field) {
    unsigned int result = scan_field(field->type, field->data);
    if (has_player(field)) result |= PROP_PLAYER;
    if (has_trap(field)) result |= PROP_TRAPPED;
    return result;
}

unsigned int properties(const int x, const int y) {
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
    unsigned int props = properties_of_field(field);
    if ((props & PROP_FLAMMABLE) && IS_DAMAGE_TYPE(FIRE_DMG, d_type))
        destroy_field(field, death_msg);
    else if ((props & PROP_MELTABLE) && IS_DAMAGE_TYPE(FIRE_DMG, d_type))
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


// ** VISUALS

static inline char line_connects(int x, int y, field_type field) {
    return (!in_bounds(x,y)) 
        ? 0 
        : fields.get(x,y)->type == field;
}

int connections(int x, int y, field_type field) {
    return 
        (line_connects(x,y-1,field) << 3) | // N
        (line_connects(x+1,y,field) << 2) | // E
        (line_connects(x,y+1,field) << 1) | // S
        line_connects(x-1,y,field);         // W 
}

field_visual get_field_data_visual(const int x, const int y, const field_type type, const field_data* data) {
    field_visual result = {
        .background_color = color_lookup[BLACK],
        .foreground_color = color_lookup[DARK_GREY],
        .mod = 0,
        .symbol = symbol_lookup[MIDDOT]
    };

    switch (type) {
        case TRENCH: {
            int char_idx = connections(x,y,TRENCH);
            result.symbol = (data->trench.fortified) 
                ? symbol_lookup[fortified_connector_lookup[char_idx]] 
                : symbol_lookup[connector_lookup[char_idx]];
            result.foreground_color = color_lookup[WHITE];
            break;
        }
        case ICE_BLOCK: {
            result.background_color = color_lookup[ICE_BLUE];
            result.foreground_color = color_lookup[WHITE];
            result.symbol = (data->ice_block.inner_type == EMPTY) 
                ? " " 
                : get_field_data_visual(x, y, data->ice_block.inner_type, data->ice_block.inner).symbol;
            break;
        }
        case TREE: {
            result.foreground_color = color_lookup[GREEN];
            result.symbol = symbol_lookup[TREE_VISUAL];
            result.mod = BOLD;
            break;
        }
        case WALL: {
            int char_idx = connections(x,y,WALL);
            result.foreground_color = color_lookup[WOOD_BROWN];
            result.symbol = (data->wall.fortified) 
                ? symbol_lookup[fortified_connector_lookup[char_idx]]
                : symbol_lookup[connector_lookup[char_idx]];
            break;
        }
        case BRIDGE: {
            int char_idx = ~connections(x,y,OCEAN);
            result.symbol = symbol_lookup[fortified_connector_lookup[char_idx]];
            result.foreground_color = color_lookup[WOOD_BROWN];
            result.background_color = color_lookup[BLUE];
            break;
        }
        case OCEAN: {
            result.foreground_color = color_lookup[ICE_BLUE];
            result.background_color = color_lookup[BLUE];
            result.symbol = "~";
            break;
        }
        case CLAY: {
            result.background_color = color_lookup[CLAY_BROWN];
            result.foreground_color = color_lookup[WOOD_BROWN];
            switch (data->clay_pit.amount) {
                case 0:
                    result.symbol = " ";
                    break;
                case 1:
                    result.symbol = symbol_lookup[SINGLE_DOT];
                    break;
                case 2:
                    result.symbol = symbol_lookup[DOUBLE_DOT];
                    break;
                case 3:
                    result.symbol = symbol_lookup[TRIPLE_DOT];
                    break;
                case 4:
                    result.symbol = symbol_lookup[QUAD_DOT];
                    break;
                case 5:
                    result.symbol = symbol_lookup[PENTA_DOT];
                    break;
                default:
                    result.symbol = symbol_lookup[ASTERIX];
                    break;
            }
            break;
        }
        case EMPTY: {
            //field_state* field = fields.get(x,y);
            //if (field->entities->count > 0) {
            //    entity_t* e = peek_entity(field->entities);
            //    switch (e->type) {
            //        case ENTITY_PLAYER:
            //            result.symbol = PERSON;
            //            result.foreground_color = e->player->team ? e->player->team->color : color_predefs.white;
            //            break;
            //        case ENTITY_VEHICLE:
            //            switch (e->vehicle->type) {
            //                case VEHICLE_BOAT:
            //                    result.foreground_color = color_predefs.white;
            //                    result.symbol = BOAT_VISUAL;
            //                    break;
            //            }
            //            break;
            //    }
            //}
            break;
        }
    }

    field_state* field = fields.get(x,y);
    if (field->entities->count > 0) {
        entity_t* e = peek_entity(field->entities);
        switch (e->type) {
            case ENTITY_PLAYER:
                result.symbol = symbol_lookup[PERSON];
                result.foreground_color = e->player->team ? *e->player->team->color : color_lookup[WHITE];
                break;
            case ENTITY_VEHICLE:
                switch (e->vehicle->type) {
                    case VEHICLE_BOAT:
                        result.foreground_color = color_lookup[WHITE];
                        result.symbol = symbol_lookup[BOAT_VISUAL];
                        break;
                }
                break;
        }
    }
    return result;
}

field_visual get_field_visual(const int x, const int y, field_state* field) {

    field_visual result = get_field_data_visual(x,y,field->type,field->data);

    //if (field->vehicle) {
    //    switch (field->vehicle->type) {
    //        case VEHICLE_BOAT: {
    //            result.foreground_color = color_predefs.white;
    //            result.symbol = BOAT_VISUAL;
    //            //result.mod = BOLD;
    //            break;
    //        }
    //    }
    //}

    if (field->overlays & SYMBOL_OVERLAY) {
        result.symbol = symbol_lookup[field->symbol];
    }
    if (field->overlays & FOREGROUND_COLOR_OVERLAY) {
        result.foreground_color = color_lookup[field->foreground_color];
    }
    if (field->overlays & BACKGROUND_COLOR_OVERLAY) {
        result.background_color = color_lookup[field->background_color];
    }
    if (field->overlays & MOD_OVERLAY) {
        result.mod = field->mod;
    }

    // clear overlays
    field->overlays = 0;

    return result; 
}

// **




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