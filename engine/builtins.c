#include "builtins.h"

#include "util.h"
#include "log.h"
#include "game_state.h"
#include "fields.h"
#include "resource_registry.h"

#include <stdlib.h>
#include <stdio.h>


#pragma region BUILTIN_VARIABLES

#pragma endregion


#pragma region BUILTIN_FUNCTIONS

int builtin_shoot(player_state* ps) {
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

int builtin_look(player_state* ps) {
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
int builtin_scan(player_state* ps) {
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

int builtin_mine(player_state* ps) {
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

int builtin_move(player_state* ps) { 
    direction d = (direction)ps->stack[--ps->sp];

    if (ps->location.type == VEHICLE_LOCATION) {

        // THIS IS NOT UP TO SPEC, PLAYER NEEDS MORE INFO, REWORK VECHILE MOVEMENT LOGIC
        int result = get_vehicle_move_func(ps->location.vehicle->type)(ps->location.vehicle, d);
        ps->stack[ps->sp++] = result;
        return result;
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

int builtin_chop(player_state* ps) {
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
        int got_sapling = rand() % 100 < _gr->settings.chop.sapling_chance;
        if (got_sapling) add_resource(&ps->resources, R_Sapling, 1);
    }

    ps->stack[ps->sp++] = INSTR_SUCCESS;
    return 1;
}

int builtin_trench(player_state* ps) {
    int p = ps->stack[--ps->sp];
    direction d = (direction)ps->stack[--ps->sp];
    int x, y;

    p = clamp(p, _gr->settings.trench.range, 0);
    location_coords(ps->location, &x, &y);
    move_coord(&x, &y, d, p);
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

int builtin_fortify(player_state* ps) {
    int p = ps->stack[--ps->sp];
    direction d = (direction)ps->stack[--ps->sp];

    if(!spend_resource(&ps->resources, R_Wood, _gr->settings.fortify.cost)) {
        ps->stack[ps->sp++] = INSTR_MISSING_RESOURCE;
        return 0;
    } 

    int x, y;
    p = clamp(p, _gr->settings.fortify.range, 0);
    location_coords(ps->location, &x, &y);
    move_coord(&x, &y, d, p);

    if (!in_bounds(x, y)) {
        ps->stack[ps->sp++] = INSTR_OUT_OF_BOUNDS;
        return 0;
    }

    int result = fields.fortify_field(fields.get(x,y));

    ps->stack[ps->sp++] = result ? INSTR_SUCCESS : INSTR_INVALID_TARGET;
    return result;
}

int builtin_bomb(player_state* ps) {
    int p = ps->stack[--ps->sp];
    direction d = (direction)ps->stack[--ps->sp];
    if(!spend_resource(&ps->resources, R_Explosive, 1)) {
        ps->stack[ps->sp++] = INSTR_MISSING_RESOURCE;
        return 0;
    }

    p = clamp(p, _gr->settings.bomb.range, 0);
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

int builtin_write(player_state* ps) {
    location_field(ps->location)->player_data = ps->stack[--ps->sp];
    ps->stack[ps->sp++] = INSTR_SUCCESS;
    return 0;
}

int builtin_read(player_state* ps) {
    if (ps->sp + 1 >= _gr->stack_size) {
        death_mark_player(ps, stack_overflow_msg);
        return 0;
    }

    ps->stack[ps->sp++] = location_field(ps->location)->player_data;
    return 0;
}

int builtin_projection(player_state* ps) {
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

int builtin_freeze(player_state* ps) {
    int p = ps->stack[--ps->sp];
    direction d = (direction)ps->stack[--ps->sp];

    if(!spend_resource(&ps->resources, R_Mana, _gr->settings.freeze.cost)) {
        ps->stack[ps->sp++] = INSTR_MISSING_RESOURCE;
        return 0;
    }

    p = clamp(p, _gr->settings.freeze.range, 0);
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

int builtin_fireball(player_state* ps) {
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

int builtin_meditate(player_state* ps) {
    add_resource(&ps->resources, R_Mana, _gr->settings.meditate.amount);
    field_state* field = location_field(ps->location);
    field->background_color = MAGIC_PURPLE;
    field->overlays |= BACKGROUND_COLOR_OVERLAY;

    ps->stack[ps->sp++] = INSTR_SUCCESS;
    return 1;
}

int builtin_dispel(player_state* ps) {
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

int builtin_disarm(player_state* ps) {
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

int builtin_mana_drain(player_state* ps) {
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

int builtin_pager_set(player_state* ps) {
    int new_channel = ps->stack[--ps->sp];
    int change = new_channel == ps->pager_channel;
    ps->pager_channel = new_channel;
    ps->stack[ps->sp++] = change;
    return 0;
}

int builtin_pager_read(player_state* ps) {
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

int builtin_pager_write(player_state* ps) {
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

int builtin_wall(player_state* ps) {
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

    ps->stack[ps->sp++] = INSTR_INVALID_TARGET;
    return 0;
}

int builtin_plant_tree(player_state* ps) {
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

int builtin_bridge(player_state* ps) {
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

int builtin_collect(player_state* ps) {
    int p = ps->stack[--ps->sp];
    direction d = (direction)ps->stack[--ps->sp];
    int x, y;
    p = clamp(p, _gr->settings.collect.range, 0);
    location_coords(ps->location, &x, &y);
    move_coord(&x, &y, d, p);

    if (!in_bounds(x, y)) {
        ps->stack[ps->sp++] = INSTR_OUT_OF_BOUNDS;
        return 0;
    }
    field_state* field = fields.get(x,y);

    int success = 0;
    int change = 0;
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
                    change = 1;
                    break;
                }
                default: {
                    int collected = field->data->clay_pit.amount > _gr->settings.clay_pit.collect_max ? _gr->settings.clay_pit.collect_max : field->data->clay_pit.amount;
                    field->data->clay_pit.amount -= collected;
                    add_resource(&ps->resources, R_Clay, collected);
                    success = 1;
                    change = 1;
                    break;
                }
            }
            break;
    }

    ps->stack[ps->sp++] = success;
    return change;
}

int builtin_say(player_state* ps) {
    int v = ps->stack[--ps->sp];
    char msg[100 + 1];
    snprintf(msg, 100, "%s#%i: %i\n", ps->name, ps->id, v);
    print_to_feed(msg);
    _log(INFO, msg);
    ps->stack[ps->sp++] = INSTR_SUCCESS;
    return 0;
}

int builtin_mount(player_state* ps) {
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

int builtin_dismount(player_state* ps) {
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

    if (ps->location.type != VEHICLE_LOCATION) {
        ps->stack[ps->sp++] = INSTR_INVALID_TARGET;
        return 1;
    }


    free(remove_entity(ps->location.vehicle->entities, ps->id));
    field_state* field = fields.get(x,y);
    location prev_loc = ps->location;
    ps->location = field_location_from_field(field);
    add_entity(field->entities, entity.of_player(ps));
    update_events(entity.of_player(ps), fields.get(x,y)->enter_events, (situation){ .type = MOVEMENT_SITUATION, .movement.loc = prev_loc });

    ps->stack[ps->sp++] = INSTR_SUCCESS;
    return 1;
}

int builtin_boat(player_state* ps) {
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

int builtin_bear_trap(player_state* ps) {
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

int builtin_throw_clay(player_state* ps) {
    int p = ps->stack[--ps->sp];
    direction d = (direction)ps->stack[--ps->sp];

    if(!spend_resource(&ps->resources, R_Clay, _gr->settings.throw_clay.cost)) {
        ps->stack[ps->sp++] = INSTR_MISSING_RESOURCE;
        return 0;
    }

    p = clamp(p, _gr->settings.throw_clay.range, 0);
    
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

int builtin_clay_golem(player_state* ps) {

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

int builtin_drop(player_state* ps) {
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

int builtin_take(player_state* ps) {
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

int builtin_mine_shaft(player_state* ps) {
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

    if (!connections(x,y, MOUNTAIN)) {
        ps->stack[ps->sp++] = INSTR_INVALID_TARGET;
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

int builtin_craft(player_state* ps) {
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

int builtin_count(player_state* ps) {
    int resource = ps->stack[--ps->sp];
    ps->stack[ps->sp++] = peek_resource(&ps->resources, resource);
    return 0;
}

#pragma endregion


int handle_builtin_function(player_state* ps, builtin_func func_addr) {
    switch (func_addr) {
        case BUILTIN_SHOOT: return builtin_shoot(ps);
        case BUILTIN_LOOK: return builtin_look(ps);
        case BUILTIN_SCAN: return builtin_scan(ps);
        case BUILTIN_MINE: return builtin_mine(ps);
        case BUILTIN_MOVE: return builtin_move(ps);
        case BUILTIN_CHOP: return builtin_chop(ps);
        case BUILTIN_TRENCH: return builtin_trench(ps);
        case BUILTIN_FORTIFY: return builtin_fortify(ps);
        case BUILTIN_BOMB: return builtin_bomb(ps);
        case BUILTIN_WRITE: return builtin_write(ps);
        case BUILTIN_READ: return builtin_read(ps);
        case BUILTIN_PROJECTION: return builtin_projection(ps);
        case BUILTIN_FREEZE: return builtin_freeze(ps);
        case BUILTIN_FIREBALL: return builtin_fireball(ps);
        case BUILTIN_MEDITATE: return builtin_meditate(ps);
        case BUILTIN_DISPEL: return builtin_dispel(ps);
        case BUILTIN_DISARM: return builtin_disarm(ps);
        case BUILTIN_MANA_DRAIN: return builtin_mana_drain(ps);
        case BUILTIN_PAGER_SET: return builtin_pager_set(ps);
        case BUILTIN_PAGER_READ: return builtin_pager_read(ps);
        case BUILTIN_PAGER_WRITE: return builtin_pager_write(ps);
        case BUILTIN_WALL: return builtin_wall(ps);
        case BUILTIN_PLANT_TREE: return builtin_plant_tree(ps);
        case BUILTIN_BRIDGE: return builtin_bridge(ps);
        case BUILTIN_COLLECT: return builtin_collect(ps);
        case BUILTIN_SAY: return builtin_say(ps);
        case BUILTIN_MOUNT: return builtin_mount(ps);
        case BUILTIN_DISMOUNT: return builtin_dismount(ps);
        case BUILTIN_BOAT: return builtin_boat(ps);
        case BUILTIN_BEAR_TRAP: return builtin_bear_trap(ps);
        case BUILTIN_THROW_CLAY: return builtin_throw_clay(ps);
        case BUILTIN_CLAY_GOLEM: return builtin_clay_golem(ps);
        case BUILTIN_DROP: return builtin_drop(ps);
        case BUILTIN_TAKE: return builtin_take(ps);
        case BUILTIN_MINE_SHAFT: return builtin_mine_shaft(ps);
        case BUILTIN_CRAFT: return builtin_craft(ps);
        case BUILTIN_COUNT: return builtin_count(ps);
    }
}

int is_action(builtin_func func_addr) {
    switch (func_addr) {
        case BUILTIN_SHOOT:
        case BUILTIN_MINE:
        case BUILTIN_MOVE:
        case BUILTIN_CHOP:
        case BUILTIN_TRENCH:
        case BUILTIN_FORTIFY:
        case BUILTIN_BOMB:
        case BUILTIN_PROJECTION:
        case BUILTIN_FREEZE:
        case BUILTIN_FIREBALL:
        case BUILTIN_MEDITATE:
        case BUILTIN_DISPEL:
        case BUILTIN_DISARM:
        case BUILTIN_MANA_DRAIN:
        case BUILTIN_WALL:
        case BUILTIN_PLANT_TREE:
        case BUILTIN_BRIDGE:
        case BUILTIN_COLLECT:
        case BUILTIN_MOUNT:
        case BUILTIN_DISMOUNT:
        case BUILTIN_BOAT:
        case BUILTIN_BEAR_TRAP:
        case BUILTIN_THROW_CLAY:
        case BUILTIN_CLAY_GOLEM:
        case BUILTIN_MINE_SHAFT:
        case BUILTIN_CRAFT:
        case BUILTIN_COUNT:
            return 1;
        default: 
            return 0;
    }
}