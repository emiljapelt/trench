#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>

#include <caml/mlvalues.h>
#include <caml/callback.h>
#include <caml/alloc.h>

#include "compiler_wrapper.h"

#include "util.h"
#include "game_state.h"
#include "visual.h"
#include "resource_registry.h"
#include "player_list.h"
#include "fields.h"
#include "log.h"

field_state* create_board(char* map_data, const int x, const int y) {
    int size = sizeof(field_state)*x*y;
    field_state* brd = malloc(size);

    for (int _x = 0; _x < x; _x++) 
    for (int _y = 0; _y < y; _y++) {
        brd[(_y * x) + _x] = (field_state) {
            .overlays = 0,
            .type = EMPTY,
            .data = NULL,
            .player_data = 0,
            .entities = array_list.create(10),
            .enter_events = array_list.create(10),
            .exit_events = array_list.create(10),
        };

        zero_out_registry(&brd[(_y * x) + _x].resources);
        set_resource_entry(&brd[(_y * x) + _x].resources, R_Wood, 0, -1);
        set_resource_entry(&brd[(_y * x) + _x].resources, R_Clay, 0, -1);
        set_resource_entry(&brd[(_y * x) + _x].resources, R_Ammo, 0, -1);
        set_resource_entry(&brd[(_y * x) + _x].resources, R_Sapling, 0, -1);
        set_resource_entry(&brd[(_y * x) + _x].resources, R_BearTrap, 0, -1);
        set_resource_entry(&brd[(_y * x) + _x].resources, R_Explosive, 0, -1);
        set_resource_entry(&brd[(_y * x) + _x].resources, R_Metal, 0, -1);

        if (map_data) switch(map_data[(_y * x) + _x]) {
            case '+': {
                field_data* data = malloc(sizeof(field_data));
                data->trench.fortified = 0;
                brd[(_y * x) + _x].type = TRENCH;
                brd[(_y * x) + _x].data = data;
                break;
            }
            case 'w': {
                field_data* data = malloc(sizeof(field_data));
                data->wall.fortified = 0;
                brd[(_y * x) + _x].type = WALL;
                brd[(_y * x) + _x].data = data;
                break;
            }
            case '~':
                brd[(_y * x) + _x].type = OCEAN;
                add_event(brd[(_y * x) + _x].enter_events, FIELD_EVENT, events.ocean_drowning, NULL);
                break;
            case 'T': 
                brd[(_y * x) + _x].type = TREE;
                break;
            case 'C':
                field_data* data = malloc(sizeof(field_data));
                data->clay_pit.amount = 0;
                brd[(_y * x) + _x].type = CLAY;
                brd[(_y * x) + _x].data = data;
                add_event(brd[(_y * x) + _x].exit_events, FIELD_EVENT, events.clay_spread, NULL);
                break;
            case 'M':
                brd[(_y * x) + _x].type = MOUNTAIN;
                break;
        }
    }

    return brd;
}

directive_info load_directive_to_struct(value comp, int stack_size) {
    int dir_len = Int_val(Field(comp, 0));
    int* stack = malloc(sizeof(int) * stack_size);
    int* dir = malloc(sizeof(int) * dir_len);

    for(int i = 0; i < dir_len; i++)
        dir[i] = Int_val(Field(comp, i+1));

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

void load_settings_struct(game_rules* gr, value settings) {
    {
        value fireball_settings = Field(settings, 0);
        gr->settings.fireball.range = Int_val(Field(fireball_settings, 0));
        gr->settings.fireball.cost = Int_val(Field(fireball_settings, 1));
    }

    {
        value shoot_settings = Field(settings, 1);
        gr->settings.shoot.range = Int_val(Field(shoot_settings, 0));
    }

    {
        value bomb_settings = Field(settings, 2);
        gr->settings.bomb.range = Int_val(Field(bomb_settings, 0));
    }

    {
        value meditate_settings = Field(settings, 3);
        gr->settings.meditate.amount = Int_val(Field(meditate_settings, 0));
    }

    {
        value dispel_settings = Field(settings, 4);
        gr->settings.dispel.cost = Int_val(Field(dispel_settings, 0));
    }

    {
        value mana_drain_settings = Field(settings, 5);
        gr->settings.mana_drain.cost = Int_val(Field(mana_drain_settings, 0));
    }

    {
        value wall_settings = Field(settings, 6);
        gr->settings.wall.cost = Int_val(Field(wall_settings, 0));
    }

    {
        value plant_tree_settings = Field(settings, 7);
        gr->settings.plant_tree.delay = Int_val(Field(plant_tree_settings, 0));
    }

    {
        value bridge_settings = Field(settings, 8);
        gr->settings.bridge.cost = Int_val(Field(bridge_settings, 0));
    }

    {
        value chop_settings = Field(settings, 9);
        gr->settings.chop.wood_gain = Int_val(Field(chop_settings, 0));
        gr->settings.chop.sapling_chance = Int_val(Field(chop_settings, 1));
    }

    {
        value fortify_settings = Field(settings, 10);
        gr->settings.fortify.cost = Int_val(Field(fortify_settings, 0));
    }

    {
        value projection_settings = Field(settings, 11);
        gr->settings.projection.cost = Int_val(Field(projection_settings, 0));
        gr->settings.projection.upkeep = Int_val(Field(projection_settings, 1));
    }

    {
        value freeze_settings = Field(settings, 12);
        gr->settings.freeze.cost = Int_val(Field(freeze_settings, 0));
        gr->settings.freeze.duration = Int_val(Field(freeze_settings, 1));
        gr->settings.freeze.range = Int_val(Field(freeze_settings, 2));
        gr->settings.freeze.refreeze = Int_val(Field(freeze_settings, 3));
    }

    {
        value look_settings = Field(settings, 13);
        gr->settings.look.range = Int_val(Field(look_settings, 0));
    }

    {
        value scan_settings = Field(settings, 14);
        gr->settings.scan.range = Int_val(Field(scan_settings, 0));
    }

    {
        value boat_settings = Field(settings, 15);
        gr->settings.boat.capacity = Int_val(Field(boat_settings, 0));
        gr->settings.boat.cost = Int_val(Field(boat_settings, 1));
        gr->settings.boat.wood_cap = Int_val(Field(boat_settings, 2));
        gr->settings.boat.clay_cap = Int_val(Field(boat_settings, 3));
        gr->settings.boat.ammo_cap = Int_val(Field(boat_settings, 4));
        gr->settings.boat.sapling_cap = Int_val(Field(boat_settings, 5));
        gr->settings.boat.beartrap_cap = Int_val(Field(boat_settings, 6));
        gr->settings.boat.explosive_cap = Int_val(Field(boat_settings, 7));
        gr->settings.boat.metal_cap = Int_val(Field(boat_settings, 8));
    }

    {
        value program_settings = Field(settings, 16);
        gr->stack_size = Int_val(Field(program_settings, 0));
        gr->program_size_limit = Int_val(Field(program_settings, 1));
    }

    {
        value throw_clay_settings = Field(settings, 17);
        gr->settings.throw_clay.range = Int_val(Field(throw_clay_settings, 0));
        gr->settings.throw_clay.cost = Int_val(Field(throw_clay_settings, 1));
    }

    {
        value clay_pit_settings = Field(settings, 18);
        gr->settings.clay_pit.spread_limit = Int_val(Field(clay_pit_settings, 0));
        gr->settings.clay_pit.contain_limit = Int_val(Field(clay_pit_settings, 1));
        gr->settings.clay_pit.collect_max = Int_val(Field(clay_pit_settings, 2));
    }

    {
        value clay_golem_settings = Field(settings, 19);
        gr->settings.clay_golem.cost = Int_val(Field(clay_golem_settings, 0));
    }

    {
        value mine_shaft_settings = Field(settings, 20);
        gr->settings.mine_shaft.cost = Int_val(Field(mine_shaft_settings, 0));
    }

    {
        value craft_settings = Field(settings, 21);
        gr->settings.craft.ammo_per_metal = Int_val(Field(craft_settings, 0));
        gr->settings.craft.beartraps_per_metal = Int_val(Field(craft_settings, 1));
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
                .debug = Bool_val(Field(unwrapped_result, 14)),
                .viewport = {
                    .x = 0,
                    .y = 0,
                    .width = Int_val(Field(Field(unwrapped_result, 15), 0)),
                    .height = Int_val(Field(Field(unwrapped_result, 15), 1)),
                    .automatic = 0,
                },
                .started = Bool_val(Field(unwrapped_result, 16)),
            };

            load_settings_struct(gr, Field(unwrapped_result, 13));

            value map = Field(unwrapped_result, 4);
            field_state* board = NULL;
            int board_x;
            int board_y;
            switch (Tag_val(map)) {
                case 0: { // EmptyMap
                    board_x = Int_val(Field(map, 0));
                    board_y = Int_val(Field(map, 1));
                    board = create_board(NULL, board_x, board_y);
                    break;
                }
                case 1: { // FileMap
                    char* map_data = strdup(String_val(Field(map, 0)));
                    board_x = Int_val(Field(Field(map, 1), 0));
                    board_y = Int_val(Field(Field(map, 1), 1));
                    board = create_board(map_data, board_x, board_y);
                    free(map_data);
                    break;
                }
            }

            int player_count = Int_val(Field(unwrapped_result, 5));
            int team_count = Int_val(Field(unwrapped_result, 7));
            int feed_size = 2000;

            *gs = (game_state) {
                .round = 1,
                .board_x = board_x,
                .board_y = board_y,
                .id_counter = 0,
                .players = array_list.create(player_count + 1),
                .board = board,
                .feed_point = 0,
                .feed_buffer = malloc(feed_size+1),
                .team_count = team_count,
                .team_states = malloc(sizeof(team_state) * team_count),
                .events = array_list.create(10),
            };

            // Center board in viewport;
            gr->viewport.x = -(gr->viewport.width / 2) + (board_x / 2);
            gr->viewport.y = -(gr->viewport.height / 2) + (board_y / 2);

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
                
                directive_info di;

                char* file_path = strdup(String_val(Field(player_info, 3)));
                int success = compile_player(file_path, gr->stack_size, gr->program_size_limit, &di);

                if (!success) exit(1);              
                
                player_state* player = malloc(sizeof(player_state));
                int player_x = Int_val(Field(Field(player_info, 2), 0));
                int player_y = Int_val(Field(Field(player_info, 2), 1));
                player->alive = 1;
                player->is_original_player = 1;
                player->death_msg = NULL;
                player->team = &gs->team_states[Int_val(Field(player_info, 0))];
                player->name = strdup(String_val(Field(player_info, 1)));
                player->id = gs->id_counter++;
                player->stack = di.stack;
                player->bp = 0;
                player->sp = 0;
                player->path = file_path;
                player->directive = di.directive;
                player->directive_len = di.dir_len;
                player->dp = 0;
                player->location = (location) { .type = VOID_LOCATION };
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
                add_player(gs->players, player);
                move_player_to_location(player, field_location_from_coords(player_x, player_y));
            }
            memset(gs->feed_buffer, 0, feed_size+1);

            return 1;
        }
        case 1: { // Error
            printf("%s", String_val(Field(callback_result, 0)));
            return 0;
        }
    }
}