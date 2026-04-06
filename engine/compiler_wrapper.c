#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>

#include <caml/mlvalues.h>
#include <caml/callback.h>
#include <caml/alloc.h>
#include <caml/bigarray.h>

#include "compiler_wrapper.h"

#include "util.h"
#include "game_state.h"
#include "visual.h"
#include "resource_registry.h"
#include "fields.h"
#include "log.h"

field_state* create_map(char* map_data, const int x, const int y) {
    int size = sizeof(field_state)*x*y;
    field_state* map = malloc(size);

    for (int _x = 0; _x < x; _x++) 
    for (int _y = 0; _y < y; _y++) {
        map[(_y * x) + _x] = (field_state) {
            .overlays = 0,
            .type = EMPTY,
            .data = NULL,
            .player_data = 0,
            .entities = array_list.create(10),
            .enter_events = array_list.create(10),
            .exit_events = array_list.create(10),
        };

        zero_out_registry(&map[(_y * x) + _x].resources);
        set_resource_entry(&map[(_y * x) + _x].resources, R_Wood, 0, -1);
        set_resource_entry(&map[(_y * x) + _x].resources, R_Clay, 0, -1);
        set_resource_entry(&map[(_y * x) + _x].resources, R_Ammo, 0, -1);
        set_resource_entry(&map[(_y * x) + _x].resources, R_Sapling, 0, -1);
        set_resource_entry(&map[(_y * x) + _x].resources, R_BearTrap, 0, -1);
        set_resource_entry(&map[(_y * x) + _x].resources, R_Explosive, 0, -1);
        set_resource_entry(&map[(_y * x) + _x].resources, R_Metal, 0, -1);

        if (map_data) switch(map_data[(_y * x) + _x]) {
            case '+': {
                field_data* data = malloc(sizeof(field_data));
                data->trench.fortified = 0;
                map[(_y * x) + _x].type = TRENCH;
                map[(_y * x) + _x].data = data;
                break;
            }
            case 'w': {
                field_data* data = malloc(sizeof(field_data));
                data->wall.fortified = 0;
                map[(_y * x) + _x].type = WALL;
                map[(_y * x) + _x].data = data;
                break;
            }
            case '~':
                map[(_y * x) + _x].type = OCEAN;
                add_event(map[(_y * x) + _x].enter_events, FIELD_EVENT, events.ocean_drowning, NULL);
                break;
            case 'T': 
                map[(_y * x) + _x].type = TREE;
                break;
            case 'C':
                field_data* data = malloc(sizeof(field_data));
                data->clay_pit.amount = 0;
                map[(_y * x) + _x].type = CLAY;
                map[(_y * x) + _x].data = data;
                add_event(map[(_y * x) + _x].exit_events, FIELD_EVENT, events.clay_spread, NULL);
                break;
            case 'M':
                map[(_y * x) + _x].type = MOUNTAIN;
                break;
        }
    }

    return map;
}
/*

CONSIDER GOING BACK TO ARRAY FROM BIGARRAY?????

*/
directive_info load_directive_to_struct(value comp, int stack_size) {

    int dir_len = ((int*)Caml_ba_array_val(comp)->data)[0];
    //int dir_len = Int_val(Field(comp, 0));
    int* stack = malloc(sizeof(int) * stack_size);
    int* dir = malloc(sizeof(int) * dir_len);

    for(int i = 0; i < dir_len; i++) {

        dir[i] = ((int*)Caml_ba_array_val(comp)->data)[i+1];
        //fprintf(stderr, "%i ", dir[i]);
    }

    return (directive_info) {
        .stack = stack,
        .directive = dir,
        .dir_len = dir_len
    };
}

int compile_player(const char* path, int stack_size, int size_limit, directive_info* result) {
    static const value* compile_player_closure = NULL;
    if(compile_player_closure == NULL) compile_player_closure = caml_named_value("compile_player_file");
    value callback_result = caml_callback2(*compile_player_closure, caml_copy_string(path), Val_int(size_limit));
    
    switch (Tag_val(callback_result)) {
        case 0: { // Ok
            value comp = Field(callback_result, 0);
            *result = load_directive_to_struct(comp, stack_size);
            return 1; 
        }
        case 1: { // Error
            printf("%s", String_val(Field(callback_result, 0)));
            return 0;
        }
    }
}

int streq(const char* a, const char* b) {
    return strcmp(a,b) == 0;
}

void ignore(int value, char* setting) {
    _log(WARN, "'%i' does not make sense for setting '%s', ignoring it", value, setting);
}

void load_settings_struct(game_rules* gr, int settings_count, value settings) {
    
    // defaults
    gr->program_size_limit = 0;
    gr->stack_size = 1000;
    gr->settings = (game_settings) {
        .fireball = { .range = 5, .cost = 10, },
        .shoot  = { .range = 6 },
        .bomb = { .range = 4 },
        .meditate = { .amount = 20 },
        .dispel = { .cost = 5 },
        .mana_drain = { .cost = 20 },
        .wall = { .cost = 10 },
        .plant_tree = { .delay = 3 },
        .bridge = { .cost = 20 },
        .chop = { .sapling_chance = 30, .wood_gain = 10, },
        .fortify = { .cost = 5, .range = 1, },
        .projection = { .cost = 50, .upkeep = 10 },
        .freeze = { .cost = 25, .duration = 2, .range = 5, .refreeze = 0, },
        .look = { .range = -1, },
        .scan = { .range = -1, },
        .boat = { .cost = 30, .capacity = 4, .wood_cap = 50, .clay_cap = 50, .ammo_cap = 100, .sapling_cap = 20, .beartrap_cap = 20, .explosive_cap = 10, .metal_cap = 10 },
        .throw_clay = { .range = 3, .cost = 1, },
        .clay_pit = { .spread_limit = 1, .contain_limit = 100, .collect_max = 5, },
        .clay_golem = { .cost = 5, },
        .mine_shaft = { .cost = 10, },
        .craft = { .ammo_per_metal = 3, .beartraps_per_metal = 1, },
        .trench = { .range = 1, },
        .collect = { .range = 1, },
        .obliviate = { .cost = 20, .range = 2, },
        .blink = { .cost = 10, .duration = 2, },
    };

    for(int i = 0; i < settings_count; i++) {
        //value setting = Field(settings, i);
        const char* key = String_val(Field(Field(settings, i), 0));
        int val = Int_val(Field(Field(settings, i), 1));

        if (streq(key, "fireball.range")) gr->settings.fireball.range = val;
        else if (streq(key, "fireball.cost")) gr->settings.fireball.cost = val;
        else if (streq(key, "shoot.range")) gr->settings.shoot.range = val;
        else if (streq(key, "bomb.range")) gr->settings.bomb.range = val;
        else if (streq(key, "meditate.amount")) gr->settings.meditate.amount = val;
        else if (streq(key, "dispel.cost")) gr->settings.dispel.cost = val;
        else if (streq(key, "mana_drain.cost")) gr->settings.mana_drain.cost = val;
        else if (streq(key, "wall.cost")) gr->settings.wall.cost = val;
        else if (streq(key, "plant_tree.delay")) gr->settings.plant_tree.delay = val;
        else if (streq(key, "bridge.cost")) gr->settings.bridge.cost = val;
        else if (streq(key, "chop.sapling_chance")) gr->settings.chop.sapling_chance = val;
        else if (streq(key, "chop.wood_gain")) gr->settings.chop.wood_gain = val;
        else if (streq(key, "fortify.cost")) gr->settings.fortify.cost = val;
        else if (streq(key, "fortify.range")) gr->settings.fortify.range = val;
        else if (streq(key, "projection.cost")) gr->settings.projection.cost = val;
        else if (streq(key, "projection.upkeep")) gr->settings.projection.upkeep = val;
        else if (streq(key, "freeze.cost")) gr->settings.freeze.cost = val;
        else if (streq(key, "freeze.duration")) gr->settings.freeze.duration = val;
        else if (streq(key, "freeze.range")) gr->settings.freeze.range = val;
        else if (streq(key, "freeze.refreeze")) gr->settings.freeze.refreeze = val;
        else if (streq(key, "look.range")) gr->settings.look.range = val;
        else if (streq(key, "scan.range")) gr->settings.scan.range = val;
        else if (streq(key, "boat.cost")) gr->settings.boat.cost = val;
        else if (streq(key, "boat.capacity")) gr->settings.boat.capacity = val;
        else if (streq(key, "boat.wood_cap")) gr->settings.boat.wood_cap = val;
        else if (streq(key, "boat.clay_cap")) gr->settings.boat.clay_cap = val;
        else if (streq(key, "boat.ammo_cap")) gr->settings.boat.ammo_cap = val;
        else if (streq(key, "boat.sapling_cap")) gr->settings.boat.sapling_cap = val;
        else if (streq(key, "boat.beartrap_cap")) gr->settings.boat.beartrap_cap = val;
        else if (streq(key, "boat.explosive_cap")) gr->settings.boat.explosive_cap = val;
        else if (streq(key, "boat.metal_cap")) gr->settings.boat.metal_cap = val;
        else if (streq(key, "program.stack_size")) {
            if (val > 0) ignore(val, "program.stack_size");
            else gr->stack_size = val;
        }
        else if (streq(key, "program.size_limit")) gr->program_size_limit = val;
        else if (streq(key, "throw_clay.range")) gr->settings.throw_clay.range = val;
        else if (streq(key, "throw_clay.cost")) gr->settings.throw_clay.cost = val;
        else if (streq(key, "clay.spread_limit")) gr->settings.clay_pit.spread_limit = val;
        else if (streq(key, "clay.contain_limit")) gr->settings.clay_pit.contain_limit = val;
        else if (streq(key, "clay.collect_max")) gr->settings.clay_pit.collect_max = val;
        else if (streq(key, "clay_golem.cost")) gr->settings.clay_golem.cost = val;
        else if (streq(key, "mine_shaft.cost")) gr->settings.mine_shaft.cost = val;
        else if (streq(key, "craft.ammo_per_metal")) gr->settings.craft.ammo_per_metal = val;
        else if (streq(key, "craft.beartraps_per_metal")) gr->settings.craft.beartraps_per_metal = val;
        else if (streq(key, "trench.range")) gr->settings.trench.range = val;
        else if (streq(key, "collect.range")) gr->settings.collect.range = val;
        else if (streq(key, "obliviate.cost")) gr->settings.obliviate.cost = val;
        else if (streq(key, "obliviate.range")) gr->settings.obliviate.range = val;
        else if (streq(key, "blink.cost")) gr->settings.blink.cost = val;
        else if (streq(key, "blink.duration")) gr->settings.blink.duration = val;

        else _log(WARN, "Unknown setting: %s", key);
    }
}

void setup_default_resource_registry(value resources) {
    for(int r = 0; r < RESOURCE_COUNT; r++) {
        value resource_info = Field(resources, r);
        int resource = Int_val(Field(resource_info, 0));
        if (resource < 0 || resource >= RESOURCE_COUNT) continue;
        default_resource_registry.resource[resource].amount = Int_val(Field(resource_info, 1));
        default_resource_registry.resource[resource].max = Int_val(Field(resource_info, 2));
    }
}

int compile_game(const char* path, game_rules* gr, game_state* gs) {
    static const value* compile_game_closure = NULL;
    if(compile_game_closure == NULL) compile_game_closure = caml_named_value("compile_game_file");
    value callback_result = caml_callback(*compile_game_closure, caml_copy_string(path));

    switch (Tag_val(callback_result)) {
        case 0: { // Ok

            value unwrapped_result = Field(callback_result, 0);
            
            int seed;
            if (Is_some(Field(unwrapped_result, 11)) ) {
                seed = Int_val(Some_val(Field(unwrapped_result, 11)));
                srand(seed);
            }
            else {
                srand((unsigned) time(NULL));
                seed = rand();
                srand(seed);
            }

            _log(INFO, "seed: %i", seed);            

            *gr = (game_rules) {
                .actions = Int_val(Field(unwrapped_result, 0)),
                .steps = Int_val(Field(unwrapped_result, 1)),
                .mode = Int_val(Field(unwrapped_result, 2)),
                .nuke = Int_val(Field(unwrapped_result, 3)),
                .exec_mode = Int_val(Field(unwrapped_result, 9)),
                .seed = seed,
                .time_scale = (float)Double_val(Field(unwrapped_result, 12)),
                .stack_size = 1000,
                .debug = Bool_val(Field(unwrapped_result, 15)),
                .viewport = {
                    .x = 0,
                    .y = 0,
                    .width = Int_val(Field(Field(unwrapped_result, 16), 0)),
                    .height = Int_val(Field(Field(unwrapped_result, 16), 1)),
                    .automatic = 0,
                },
                .started = Bool_val(Field(unwrapped_result, 17)),
            };

            load_settings_struct(gr, Int_val(Field(unwrapped_result, 13)), Field(unwrapped_result, 14));

            value _map = Field(unwrapped_result, 4);
            field_state* map = NULL;
            int map_width;
            int map_height;
            switch (Tag_val(_map)) {
                case 0: { // EmptyMap
                    map_width = Int_val(Field(_map, 0));
                    map_height = Int_val(Field(_map, 1));
                    map = create_map(NULL, map_width, map_height);
                    break;
                }
                case 1: { // FileMap
                    char* map_data = strdup(String_val(Field(_map, 0)));
                    map_width = Int_val(Field(Field(_map, 1), 0));
                    map_height = Int_val(Field(Field(_map, 1), 1));
                    map = create_map(map_data, map_width, map_height);
                    free(map_data);
                    break;
                }
            }

            int player_count = Int_val(Field(unwrapped_result, 5));
            int team_count = Int_val(Field(unwrapped_result, 7));

            *gs = (game_state) {
                .round = 1,
                .map_width = map_width,
                .map_height = map_height,
                .id_counter = 0,
                .entities = array_list.create(player_count + 1),
                .map = map,
                .feed = malloc(FEED_WIDTH * gr->viewport.height),
                .team_count = team_count,
                .team_states = malloc(sizeof(team_state) * team_count),
                .events = array_list.create(10),
            };

            memset(gs->feed, ' ', FEED_WIDTH * gr->viewport.height);

            // Center board in viewport;
            gr->viewport.x = -(gr->viewport.width / 2) + (map_width / 2);
            gr->viewport.y = -(gr->viewport.height / 2) + (map_height / 2);

            for(int i = 0; i < team_count; i++) {
                value team_info = Field(Field(unwrapped_result, 8),i);
                gs->team_states[i].team_name = strdup(String_val(Field(team_info, 0)));
                gs->team_states[i].color = malloc(sizeof(color));
                *gs->team_states[i].color = (color) { 
                    .r = (Int_val(Field(Field(team_info, 1), 0))),
                    .g = (Int_val(Field(Field(team_info, 1), 1))),
                    .b = (Int_val(Field(Field(team_info, 1), 2))),
                };
                gs->team_states[i].members_alive = Int_val(Field(team_info, 2));
            }

            setup_default_resource_registry(Field(unwrapped_result, 10));

            for(int i = 0; i < player_count; i++) {
                value player_info = Field(Field(unwrapped_result, 6),i);
                
                player_state* player = malloc(sizeof(player_state));
                int player_x = Int_val(Field(Field(player_info, 2), 0));
                int player_y = Int_val(Field(Field(player_info, 2), 1));
                player->is_original_player = 1;
                player->team = &gs->team_states[Int_val(Field(player_info, 0))];
                player->name = strdup(String_val(Field(player_info, 1)));
                player->bp = 0;
                player->sp = 0;
                player->path = strdup(String_val(Field(player_info, 3)));
                player->dp = 0;
                player->remaining_steps = gr->steps;
                player->remaining_actions = gr->actions;
                player->pager_channel = 0;
                player->pager_msgs = array_list.create(10);
                player->pre_death_events = array_list.create(10);
                player->post_death_events = array_list.create(10);
                copy_resource_registry(&default_resource_registry, &player->resources);
                player->extra_files = array_list.create(Int_val(Field(player_info, 4)));
                for(int f = 0; f < player->extra_files->size; f++) {
                    array_list.add(player->extra_files, strdup(String_val(Field((Field(player_info, 5)), f))));
                }
                entity_t* e = entity_of_player(player);
                add_entity(gs->entities, e);
                move_entity_to_location(e, field_location_from_coords(player_x, player_y));
            }

            // Load player directives after loading all information from the compiled game file.
            // This seems to prevent the OCaml GC from wrecking the data
            for(int i = 0; i < gs->entities->count; i++) {
                directive_info di;
                entity_t* entity = get_entity(gs->entities, i);
                if (entity->type == ENTITY_PLAYER) {
                    int success = compile_player(entity->player->path, gr->stack_size, gr->program_size_limit, &di);
                    if (!success) exit(1);
                    entity->player->stack = di.stack;
                    entity->player->directive = di.directive;
                    entity->player->directive_len = di.dir_len;
                }
            }

            return 1;
        }
        case 1: { // Error
            printf("%s", String_val(Field(callback_result, 0)));
            return 0;
        }
    }
}