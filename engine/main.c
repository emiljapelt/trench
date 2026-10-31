#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <locale.h>

#include <caml/callback.h>

#include "player.h"
#include "event_list.h"
#include "game_rules.h"
#include "game_state.h"
#include "visual.h"
#include "util.h"
#include "compiler_wrapper.h"
#include "entity.h"
#include "exec_context.h"
#include "log.h"
#include "execute.h"

void garbage_collect() {
    int count_active = 0;
    for(int i = 0; i < _gs->entities->count; i++) {
        if (get_entity(_gs->entities, i)->active)
            count_active++;
    }

    if (count_active == _gs->entities->count) return; // Nothing to collect

    entity_list_t* active_entities = array_list.create(count_active);

    for(int i = _gs->entities->count - 1; i >= 0; i--) {
        entity_t* entity = get_entity(_gs->entities, i);
        
        if (entity->active) {
            array_list.add(active_entities, entity);
        } 
        else switch (entity->type) {
            case ENTITY_PLAYER: {
                free_player(entity->player);
                free(entity);
                break;
            }
            case ENTITY_VEHICLE: {
                array_list.free(entity->vehicle->entities);
                free(entity);
                break;
            }
            default:
                _log(WARN, "Failed to GC entity: %i", entity->id);        
        }
    }

    array_list.free(_gs->entities);
    _gs->entities = active_entities;
}

int is_in_view(int x, int y) {
    return 
        (x >= _gr->viewport.x) && 
        (x < _gr->viewport.x + _gr->viewport.width) && 
        (y >= _gr->viewport.y) && 
        (y < _gr->viewport.y + _gr->viewport.height);
}

void center_viewport(int x, int y) {
    const int width = _gr->viewport.width - (_gr->feed ? _gr->feed_width : 0);
    const int height = _gr->viewport.height;
    _gr->viewport.x = x - (width / 2);
    _gr->viewport.y = y - (height / 2);
}

void pan_viewport(float time, const int x, const int y) {
    const int width = _gr->viewport.width - (_gr->feed ? _gr->feed_width : 0);
    const int height = _gr->viewport.height;
    int x_diff = x - (_gr->viewport.x + (width / 2));
    int y_diff = y - (_gr->viewport.y + (height / 2));

    int steps = max(abs(x_diff), abs(y_diff));
    float time_per_step = time / steps;

    while (x_diff || y_diff) {
        int x_mod = clamp(x_diff, 1, -1);
        int y_mod = clamp(y_diff, 1, -1);;

        _gr->viewport.x += x_mod;
        _gr->viewport.y += y_mod;

        x_diff -= x_mod;
        y_diff -= y_mod;

        print_board();
        wait(time_per_step);
    }
}

// Deprecated?
void pan_viewport_player(float time, player_state* ps) {
    int x, y;
    if (location_coords(ps->entity->location, &x, &y))
        pan_viewport(time, x, y);
}

void auto_viewport(int x, int y) {
    if (_gr->viewport.automatic && !is_in_view(x,y)) 
        //center_viewport(x,y);
        pan_viewport(0.5, x, y);
}

void auto_viewport_player(entity_t* e) {
    if (e == NULL) return;
    if (_gr->viewport.automatic) {
        int x, y;
        if (location_coords(e->location, &x, &y))
            auto_viewport(x,y);
    }
}

void get_new_directive(player_state* ps) {
    while(1) {
        char* path;
        char option;
        int do_free = 0;

        if (ps->extra_files->count) {
            int index = ps->extra_files->count - 1;
            char* next = array_list.get(ps->extra_files, index);
            array_list.remove(ps->extra_files, index, 0);
            if (strcmp("_", next) == 0) {
                option = '0';
            } 
            else {
                option = '1';
                free(ps->path);
                ps->path = next;
            }
        }
        else {   
            printf("%s#%i, change directive?:\n0: No change\n1: Reload file\n2: New file\n", ps->name, ps->entity->id);

            terminal_blocking_read_on();
            scanf(" %c",&option);
            terminal_blocking_read_off();
        }

        switch (option) {
            case '0': {
                _log(INFO, "%s: no change", ps->name);
                return;
            }
            case '1': {
                path = ps->path;
                break;
            };
            case '2': {
                path = malloc(201);
                memset(path,0,201);
                puts("New path:");
                scanf(" %200s", path);
                do_free = 1;
                break;
            }
            default: continue;
        }

        directive_info di;
        if (compile_player(ps, path, _gr->stack_size, &di)) {
            if (do_free) free(ps->path);
            free(ps->exec->program);
            free(ps->exec->stack);
            ps->exec->program = di.directive;
            ps->exec->program_len = di.dir_len;
            ps->exec->stack = di.stack;
            ps->exec->sp = 0;
            ps->exec->bp = 0;
            ps->exec->dp = 0;
            ps->path = path;
            _log(INFO, "%s: change to %s", ps->name, path);
            return;
        }
    }
}

void nuke_board() {
    for(int y = 0; y < _gs->map_height; y++)
    for(int x = 0; x < _gs->map_width; x++) 
        fields.destroy_field(fields.get(x,y), "Got nuked");
}

int teams_alive() {
    int alive = 0;
    for(int i = 0; i < _gs->team_count; i++) {
        if (_gs->team_states[i].members_alive) {
            alive++; 
            continue;
        }
    }
    return alive;
}

char* first_team_alive() {
    for(int i = 0; i < _gs->entities->count; i++) {
        entity_t* entity = get_entity(_gs->entities, i);

        // entity is a player, is alive and is on a team
        if (entity->type == ENTITY_PLAYER && entity->active && entity->player->team)
            return entity->player->team->team_name;
    }

    printf("No team has live players\n");
    exit(1);
}

void check_win_condition() {

    switch (teams_alive()) {
        case 0:
            _log(INFO, "--- GAME END: Everyone is dead ---");
            _log_flush();
            printf("GAME OVER: Everyone is dead...\n"); 
            terminal_echo_on();
            terminal_blocking_read_on();
            terminal_canonical_on();
            exit(0);
        case 1:
            if (_gs->team_count == 1) break;
            else {
                _log(INFO, "--- GAME END: %s won! ---", first_team_alive());
                _log_flush();
                printf("Team %s won!\n", first_team_alive()); 
                terminal_echo_on();
                terminal_blocking_read_on();
                terminal_canonical_on();
                exit(0);
            }
        default: break;
    }
}

void handle_input() {
    char buf[1];
    int pause = !_gr->started;
    while (1) {
        int read_status = read(STDIN_FILENO, &buf, 1);

        if (read_status == -1 || read_status == 0) {
            if (pause) continue;
            else return;
        }

        switch (buf[0]) {
            case '+':
                _gr->time_scale += 0.1;
                break;
            case '-': 
                _gr->time_scale -= 0.1;
                if (_gr->time_scale < 0.1) _gr->time_scale = 0.1;
                break;
            case '0':
                _gr->time_scale = 0;
                break;
            case '1':
                _gr->time_scale = 1;
                break;
            case 'a':
                _gr->viewport.automatic = !_gr->viewport.automatic;
                break;
            case '\033': {
                if (_gr->viewport.automatic) break;
                char special_buf[2];
                read(STDIN_FILENO, &special_buf, 2);
                switch (special_buf[1]) {
                    case 'A': // UP
                        _gr->viewport.y--;
                        break;
                    case 'B': // DOWN
                        _gr->viewport.y++;
                        break;
                    case 'C': // RIGHT
                        _gr->viewport.x++;
                        break;
                    case 'D': // LEFT
                        _gr->viewport.x--;
                        break;
                }
                print_board();
                break;
            }
            case ' ': 
                pause = !pause;
                _gr->started = 1;
                break;
            case 'f': 
                _gr->feed ^= 1;
                print_board();
                break;
            case 'q': 
                terminal_echo_on();
                terminal_blocking_read_on();
                terminal_canonical_on();
                exit(0);
                break;
        }
    }
}

/*
    Players complete their turn seperately one at a time. 
    Once each player has taken a turn, a round has passed.
*/
void play_round_default() {
    const int entity_count = _gs->entities->count;
    for(int i = 0; i < entity_count; i++) {
        handle_input();
        entity_t* entity = get_entity(_gs->entities, i);
        int finished_events = update_events(entity, _gs->events);
        
        if (finished_events || _gs->feed_change) { print_board(); wait(1); }

        switch (entity->type) {
            case ENTITY_PLAYER:
                if (entity->active) {
                    _log(DEBUG, "Turn: %s (#%i)", entity->player->name, entity->id);
                    exec(entity->player->exec);
                    set_player_steps_and_actions(entity->player);
                }
                break;
            default:
                break;
        }

        check_win_condition();
    }
    if (_gs->round - _gs->latest_print >= 1000) print_board();
    if (_gr->nuke > 0 && _gs->round % _gr->nuke == 0) nuke_board();
    garbage_collect();
}

void play_round() {
    switch (_gr->exec_mode) {
        case 0:
            play_round_default();
            break;
    }
    _log_flush();
}

// Mode: 0
// Players cannot change directive
void static_mode() {
    while(1) {
        play_round();
        _gs->round++;
    }
}

// Mode: x
// Players can change directive after 'x' rounds
void dynamic_mode() {
    while(1) {
        play_round();
        if (_gs->round % _gr->mode == 0) {
            const int entity_count = _gs->entities->count;
            for (int i = 0; i < entity_count; i++) {
                entity_t* entity = get_entity(_gs->entities, i);

                if (entity->type == ENTITY_PLAYER) {
                    if (!entity->active) continue; 
                    if (entity->player->is_original_player)
                        get_new_directive(entity->player);
                    print_board();
                }
            }
        }
        _gs->round++;
    }
}


// Mode: -x
// Players can change directive before each of their turns
void manual_mode() {
    while(1) {
        const int entity_count = _gs->entities->count;
        for(int i = 0; i < entity_count; i++) {
            entity_t* entity = get_entity(_gs->entities, i);

            if (entity->type == ENTITY_PLAYER) {
                if (!entity->active) continue;
                if (entity->player->is_original_player)
                    get_new_directive(entity->player);
                print_board();
                exec(entity->player->exec);
                check_win_condition();
            }
        }

        if (_gr->nuke > 0 && _gs->round % _gr->nuke == 0) nuke_board();
        _gs->round++;
    }
}

int main(int argc, char** argv) {

    if (argc < 2) {
        printf("Too few arguments given, needs: <game_file_path>\n");
        exit(1);
    }
    caml_startup(argv);
    _gr = malloc(sizeof(game_rules));
    _gs = malloc(sizeof(game_state));

    _log(INFO, "--- COMPILING ----"); 

    if(!compile_game(argv[1], _gr, _gs)) return 1;

    _log(INFO,  "DONE!");

    terminal_echo_off();
    terminal_blocking_read_off();
    terminal_canonical_off();

    setlocale(LC_ALL, "");

    set_wait_time_scaler(&_gr->time_scale);

    print_board();

    handle_input();

    _log(INFO, "--- RUNNING ---");
    _log_flush();

    if (_gr->mode == 0) static_mode();
    else if (_gr->mode < 0) manual_mode();
    else if (_gr->mode > 0) dynamic_mode();

    return 0;
}