#include <string.h>
#include <time.h>

#include <caml/mlvalues.h>
#include <caml/callback.h>
#include <caml/alloc.h>

#include "util.h"
#include "game_state.h"
#include "visual.h"
#include "resource_registry.h"

field_state* empty_board(const int x, const int y) {
    int size = sizeof(field_state)*x*y;
    field_state* brd = malloc(size);

    for (int _x = 0; _x < x; _x++) 
    for (int _y = 0; _y < y; _y++) {
        event_list* enter_events = malloc(sizeof(event_list*));
        event_list* exit_events = malloc(sizeof(event_list*));
        enter_events->list = NULL;
        exit_events->list = NULL;
        brd[(_y * x) + _x] = (field_state) {
            .color_overlay = 0,
            .mod_overlay = 0,
            .symbol_overlay = 0,
            .type = EMPTY,
            .enter_events = enter_events,
            .exit_events = exit_events,
        };
    }

    //memset(brd,0,size);
    return brd;
}

directive_info load_directive_to_struct(const char* directive) {
    int dl = 0;
    int dir_len;
    int rl = 0;
    int regs;
    int* stack = malloc(sizeof(int)*1000);
    
    while(directive[dl] != ':') dl++;
    dir_len = sub_str_to_int(directive,0,dl);

    while(directive[dl+1+rl] != ':') rl++;
    regs = sub_str_to_int(directive+dl+1,0,rl);
    memset(stack,0,sizeof(int)*regs);

    char* dir = malloc(dir_len+1); dir[dir_len] = 0;
    memcpy(dir, directive+dl+rl+2, dir_len);

    return (directive_info) {
        .regs = regs,
        .stack = stack,
        .directive = dir,
        .dir_len = dir_len,
    };
}

int compile_player(const char* path, directive_info* result) {
    static const value* compile_player_closure = NULL;
    if(compile_player_closure == NULL) compile_player_closure = caml_named_value("compile_player_file");
    value callback_result = caml_callback(*compile_player_closure, caml_copy_string(path));

    switch (Tag_val(callback_result)) {
        case 0: { // Ok
            const char* string_result = String_val(Field(callback_result, 0));
            *result = load_directive_to_struct(string_result);
            char* dir = result->directive;
            char* dir_dup = malloc(result->dir_len);
            memcpy(dir_dup, dir, result->dir_len);
            result->directive = dir_dup;
            return 1; 
        }
        case 1: { // Error
            printf("%s", String_val(Field(callback_result, 0)));
            return 0;
        }
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
            if (Is_some(Field(unwrapped_result, 14)) ) {
                seed = Int_val(Some_val(Field(unwrapped_result, 14)));
                srand(seed);
            }
            else {
                srand((unsigned) time(NULL));
                seed = rand();
                srand(seed);
            }

            *gr = (game_rules) {
                .actions = Int_val(Field(unwrapped_result, 0)),
                .steps = Int_val(Field(unwrapped_result, 1)),
                .mode = Int_val(Field(unwrapped_result, 2)),
                .nuke = Int_val(Field(unwrapped_result, 3)),
                .array = Int_val(Field(unwrapped_result, 4)),
                .feature_level = Int_val(Field(unwrapped_result, 8)),
                .exec_mode = Int_val(Field(unwrapped_result, 11)),
                .seed = seed,
            };

            int player_count = Int_val(Field(unwrapped_result, 6));
            int team_count = Int_val(Field(unwrapped_result, 9));
            int global_arrays_size = 3*(sizeof(int)*Int_val(Field(unwrapped_result, 4)));
            int board_x = Int_val(Field(Field(unwrapped_result, 5),0));
            int board_y = Int_val(Field(Field(unwrapped_result, 5),1));
            int resource_count = Int_val(Field(unwrapped_result, 12));
            int feed_size = 200;

            *gs = (game_state) {
                .round = 1,
                .board_x = board_x,
                .board_y = board_y,
                .player_count = player_count,
                .players = malloc(sizeof(player_state)*player_count),
                .board = empty_board(board_x, board_y),
                .feed_point = 0,
                .feed_buffer = malloc(feed_size+1),
                .global_arrays = malloc(global_arrays_size),
                .team_count = team_count,
                .team_states = malloc(sizeof(team_state) * team_count),
                .resource_registry = create_resource_registry(player_count, 10, resource_count)
            };

            for(int i = 0; i < resource_count; i++) {
                value resource = Field(Field(unwrapped_result, 13), i);
                init_resource(gs->resource_registry, String_val(Field(resource, 0)), Int_val(Field(resource, 1)), player_count);
            }


            for(int i = 0; i < team_count; i++) {
                value team_info = Field(Field(unwrapped_result, 10),i);
                gs->team_states[i].team_id = Int_val(Field(team_info, 0));
                gs->team_states[i].members_alive = Int_val(Field(team_info, 1));
            }

            for(int i = 0; i < player_count; i++) {
                value player_info = Field(Field(unwrapped_result, 7),i);
                directive_info di = load_directive_to_struct(String_val(Field(player_info, 4)));
                event_list* death_events = malloc(sizeof(event_list*));
                death_events->list = NULL;
                gs->players[i].alive = 1;
                gs->players[i].death_msg = NULL;
                gs->players[i].team = Int_val(Field(player_info, 0));
                gs->players[i].name = strdup(String_val(Field(player_info, 1)));
                gs->players[i].id = i;
                gs->players[i].stack = di.stack;
                gs->players[i].sp = di.regs;
                gs->players[i].path = strdup(String_val(Field(player_info, 3)));
                gs->players[i].directive = di.directive;
                gs->players[i].directive_len = di.dir_len;//(Field(player_info, 4))-(di.regs_len+1);
                gs->players[i].dp = 0;
                gs->players[i].x = Int_val(Field(Field(player_info, 2), 0));
                gs->players[i].y = Int_val(Field(Field(player_info, 2), 1));
                gs->players[i].death_events = death_events;
            }
            memset(gs->global_arrays, 0, global_arrays_size);
            // memset(gs->overlay, 0, sizeof(char*) * (board_x*board_y));
            memset(gs->feed_buffer, 0, feed_size+1);

            return 1;
        }
        case 1: { // Error
            printf("%s", String_val(Field(callback_result, 0)));
            return 0;
        }
    }
}