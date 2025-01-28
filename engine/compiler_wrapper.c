#include <string.h>
#include <time.h>

#include <caml/mlvalues.h>
#include <caml/callback.h>
#include <caml/alloc.h>

#include "util.h"
#include "game_state.h"
#include "visual.h"
#include "resource_registry.h"
#include "player_list.h"

field_state* empty_board(const int x, const int y) {
    int size = sizeof(field_state)*x*y;
    field_state* brd = malloc(size);

    for (int _x = 0; _x < x; _x++) 
    for (int _y = 0; _y < y; _y++) {
        linked_list* enter_events = malloc(sizeof(linked_list*));
        linked_list* exit_events = malloc(sizeof(linked_list*));
        enter_events->list = NULL;
        exit_events->list = NULL;
        brd[(_y * x) + _x] = (field_state) {
            .foreground_color_overlay = NULL,
            .background_color_overlay = NULL,
            .mod_overlay = 0,
            .symbol_overlay = 0,
            .type = EMPTY,
            .player_data = 0,
            .enter_events = enter_events,
            .exit_events = exit_events,
        };
    }

    //memset(brd,0,size);
    return brd;
}

directive_info load_directive_to_struct(value comp) {
    int dir_len = Int_val(Field(comp, 0));
    int regs = Int_val(Field(comp, 1));
    int* stack = malloc(sizeof(int) * (regs + 1000));
    memset(stack, 0, sizeof(int)*regs);

    int* dir = malloc(sizeof(int) * dir_len);
    for(int i = 0; i < dir_len; i++)
        dir[i] = Int_val(Field(comp, i+2));

    return (directive_info) {
        .regs = regs,
        .stack = stack,
        .directive = dir,
        .dir_len = dir_len
    };
}

int compile_player(const char* path, directive_info* result) {
    static const value* compile_player_closure = NULL;
    if(compile_player_closure == NULL) compile_player_closure = caml_named_value("compile_player_file");
    value callback_result = caml_callback(*compile_player_closure, caml_copy_string(path));

    switch (Tag_val(callback_result)) {
        case 0: { // Ok
            value comp = Field(callback_result, 0);
            *result = load_directive_to_struct(comp);
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
            if (Is_some(Field(unwrapped_result, 12)) ) {
                seed = Int_val(Some_val(Field(unwrapped_result, 12)));
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
                .exec_mode = Int_val(Field(unwrapped_result, 9)),
                .seed = seed,
                .time_scale = (float)Double_val(Field(unwrapped_result, 13)),
            };

            int player_count = Int_val(Field(unwrapped_result, 5));
            int team_count = Int_val(Field(unwrapped_result, 7));
            int board_x = Int_val(Field(Field(unwrapped_result, 4),0));
            int board_y = Int_val(Field(Field(unwrapped_result, 4),1));
            int resource_count = Int_val(Field(unwrapped_result, 10));
            int feed_size = 200;

            *gs = (game_state) {
                .round = 1,
                .board_x = board_x,
                .board_y = board_y,
                .player_count = player_count,
                .players = malloc(sizeof(linked_list)),
                .board = empty_board(board_x, board_y),
                .feed_point = 0,
                .feed_buffer = malloc(feed_size+1),
                .team_count = team_count,
                .team_states = malloc(sizeof(team_state) * team_count),
                .events = malloc(sizeof(linked_list*))
            };
            gs->events->list = NULL;
            gs->players->list = NULL;

            for(int i = 0; i < team_count; i++) {
                value team_info = Field(Field(unwrapped_result, 8),i);
                gs->team_states[i].team_name = strdup(String_val(Field(team_info, 0)));
                gs->team_states[i].color = malloc(sizeof(color));
                *gs->team_states[i].color = (color) { 
                    .r = (Int_val(Field(Field(team_info, 1), 0))),
                    .g = (Int_val(Field(Field(team_info, 1), 1))),
                    .b = (Int_val(Field(Field(team_info, 1), 2))),
                    .predef = 1
                };
                gs->team_states[i].members_alive = Int_val(Field(team_info, 2));
            }

            for(int i = 0; i < player_count; i++) {
                value player_info = Field(Field(unwrapped_result, 6),i);
                directive_info di = load_directive_to_struct(Field(player_info, 4));
                linked_list* death_events = malloc(sizeof(linked_list*));
                death_events->list = NULL;
                player_state* player = malloc(sizeof(player_state));
                player->alive = 1;
                player->death_msg = NULL;
                player->team = Int_val(Field(player_info, 0));
                player->name = strdup(String_val(Field(player_info, 1)));
                player->id = i;
                player->stack = di.stack;
                player->sp = di.regs;
                player->path = strdup(String_val(Field(player_info, 3)));
                player->directive = di.directive;
                player->directive_len = di.dir_len;//(Field(player_info, 4))-(di.regs_len+1);
                player->dp = 0;
                player->x = Int_val(Field(Field(player_info, 2), 0));
                player->y = Int_val(Field(Field(player_info, 2), 1));
                player->death_events = death_events;
                player->resources = create_resource_registry(10, resource_count);
                for(int r = 0; r < resource_count; r++) {
                    value resource = Field(Field(unwrapped_result, 11), r);
                    init_resource(player->resources, strdup(String_val(Field(resource, 0))), Int_val(Field(resource, 1)));
                }
                add_player(gs->players, player);
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