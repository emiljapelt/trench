#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <caml/callback.h>

#include "player.h"
#include "player_list.h"
#include "event_list.h"
#include "game_rules.h"
#include "game_state.h"
#include "visual.h"
#include "util.h"
#include "compiler_wrapper.h"
#include "instructions.h"
#include "entity.h"
#include "log.h"

void debug_print(player_state* ps) {
    fprintf(stderr,"\nPlayer %s(%i)\ninstr:%i\ndp:%i\nbp:%i\nsp:%i\n", ps->name, ps->id, ps->directive[ps->dp], ps->dp, ps->bp, ps->sp); wait(0.5);
    for(int i = 0; i < ps->sp; i++) {
        fprintf(stderr,"%i, ", ps->stack[i]); wait(0.1);
    }
    wait(1);
}

void try_kill_player(player_state* ps) {
    if (ps->alive && ps->death_msg != NULL) {
        update_events(entity.of_player(ps), ps->pre_death_events, (situation){ .type = NO_SITUATION});
        if (ps->alive && ps->death_msg != NULL) {
            kill_player(ps);
            update_events(entity.of_player(ps), ps->post_death_events, (situation){ .type = NO_SITUATION});
        }
    }
}

void kill_players() {
    each_player(_gs->players, &try_kill_player);
}

int is_in_view(int x, int y) {
    return 
        (x >= _gr->viewport.x) && 
        (x < _gr->viewport.x + _gr->viewport.width) && 
        (y >= _gr->viewport.y) && 
        (y < _gr->viewport.y + _gr->viewport.height);
}

void center_viewport(int x, int y) {
    _gr->viewport.x = x - (_gr->viewport.width / 2);
    _gr->viewport.y = y - (_gr->viewport.height / 2);
}

void pan_viewport(float time, const int x, const int y) {
    int x_diff = x - (_gr->viewport.x + (_gr->viewport.width / 2));
    int y_diff = y - (_gr->viewport.y + (_gr->viewport.height / 2));

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

void auto_viewport(int x, int y) {
    if (_gr->viewport.automatic && !is_in_view(x,y)) 
        //center_viewport(x,y);
        pan_viewport(0.5, x, y);
}

void auto_viewport_player(player_state* ps) {
    if (_gr->viewport.automatic) {
        int x, y;
        location_coords(ps->location, &x, &y);
        auto_viewport(x,y);
    }
}


void player_turn_default(player_state* ps) {
    while(1) {
        int change = 0;
        if (ps->dp >= ps->directive_len) { return; }
        if (!use_resource(1,&ps->remaining_steps)) return;
        // debug_print(ps);
        _log(DEBUG, "%s executes %i", ps->name, ps->directive[ps->dp]);
        switch (ps->directive[ps->dp++]) {
            case Meta_PlayerX: change = meta_player_x(ps);break;
            case Meta_PlayerY: change = meta_player_y(ps);break;
            case Meta_BoardX: change = meta_board_x(ps);break;
            case Meta_BoardY: change = meta_board_y(ps);break;
            case Meta_Resource: change = meta_resource(ps);break;
            case Meta_PlayerID: change = meta_player_id(ps);break;
            case Instr_Wait: {
                if(!use_resource(1,&ps->remaining_actions)) {ps->dp--;return;}
                ps->stack[ps->sp++] = 1;
                break;
            }
            case Instr_Pass: {
                ps->stack[ps->sp++] = 1;
                return;
            }
            case Instr_Shoot: { 
                if(!use_resource(1,&ps->remaining_actions)) {ps->dp--;return;}
                change = instr_shoot(ps);
                break;
            }
            case Instr_Look: instr_look(ps); break;
            case Instr_Scan: instr_scan(ps); break;
            case Instr_Mine: {
                if(!use_resource(1,&ps->remaining_actions)) {ps->dp--;return;}
                change = instr_mine(ps);
                break;
            }
            case Instr_Move: {
                if(!use_resource(1,&ps->remaining_actions)) {ps->dp--;return;}
                change = instr_move(ps);
                break;
            }
            case Instr_Chop: {
                if(!use_resource(1,&ps->remaining_actions)) {ps->dp--; return;}
                change = instr_chop(ps);
                break;
            }
            case Instr_Trench: { 
                if(!use_resource(1,&ps->remaining_actions)) {ps->dp--; return;}
                change = instr_trench(ps);
                break;
            }
            case Instr_Fortify: {
                if(!use_resource(1,&ps->remaining_actions)) {ps->dp--; return;}
                change = instr_fortify(ps);
                break;
            }
            case Instr_PlantTree: {
                if(!use_resource(1,&ps->remaining_actions)) {ps->dp--; return;}
                change = instr_plant_tree(ps);
                break;
            }
            case Instr_Random: instr_random_int(ps); break; 
            case Instr_RandomSet: instr_random_range(ps); break;
            case Instr_Place: instr_place(ps); break;
            case Instr_Bomb: { 
                if(!use_resource(1,&ps->remaining_actions)) {ps->dp--; return;}
                change = instr_bomb(ps);
                break;
            }
            case Instr_Access: change = instr_access(ps); break;
            case Instr_GoTo: change = instr_goto(ps); break;
            case Instr_GoToIf: change = instr_goto_if(ps); break;
            case Instr_Eq: change = instr_eq(ps); break;
            case Instr_Lt: change = instr_lt(ps); break;
            case Instr_Sub: change = instr_sub(ps); break;
            case Instr_Add: change = instr_add(ps); break;
            case Instr_Mul: change = instr_mul(ps); break;
            case Instr_Div: change = instr_div(ps); break;
            case Instr_Mod: change = instr_mod(ps); break;
            case Instr_Not: change = instr_not(ps); break;
            case Instr_Or: change = instr_or(ps); break;
            case Instr_And: change = instr_and(ps); break;
            case Instr_Assign: change = instr_assign(ps); break;
            case Instr_FieldProp: change = instr_field_prop(ps); break;
            case Instr_DecStack: change = instr_dec_stack(ps); break;
            case Instr_Copy: change = instr_copy(ps); break;
            case Instr_Swap: change = instr_swap(ps); break;
            case Instr_Read: change = instr_read(ps); break;
            case Instr_Write: change = instr_write(ps); break;
            case Instr_Projection: {
                if(!use_resource(1,&ps->remaining_actions)) {ps->dp--; return;}
                change = instr_projection(ps); 
                break;
            }
            case Instr_Freeze: {
                if(!use_resource(1,&ps->remaining_actions)) {ps->dp--; return;}
                change = instr_freeze(ps); 
                break;
            }
            case Instr_Fireball: {
                if(!use_resource(1,&ps->remaining_actions)) {ps->dp--; return;}
                change = instr_fireball(ps); 
                break;
            }
            case Instr_Meditate: {
                if(!use_resource(1,&ps->remaining_actions)) {ps->dp--; return;}
                change = instr_meditate(ps); 
                break;
            }
            case Instr_Disarm: {
                if(!use_resource(1,&ps->remaining_actions)) {ps->dp--; return;}
                change = instr_disarm(ps); 
                break;
            }
            case Instr_Dispel: {
                if(!use_resource(1,&ps->remaining_actions)) {ps->dp--; return;}
                change = instr_dispel(ps); 
                break;
            }
            case Instr_ManaDrain: {
                if(!use_resource(1,&ps->remaining_actions)) {ps->dp--; return;}
                change = instr_mana_drain(ps); 
                break;
            }
            case Instr_PagerSet: change = instr_pager_set(ps); break;
            case Instr_PagerRead: change = instr_pager_read(ps); break; 
            case Instr_PagerWrite: change = instr_pager_write(ps); break;
            case Instr_Wall: {
                if(!use_resource(1,&ps->remaining_actions)) {ps->dp--; return;}
                change = instr_wall(ps);
                break;
            }
            case Instr_Bridge: {
                if(!use_resource(1,&ps->remaining_actions)) {ps->dp--; return;}
                change = instr_bridge(ps);
                break;
            }
            case Instr_Collect: {
                if(!use_resource(1,&ps->remaining_actions)) {ps->dp--; return;}
                change = instr_collect(ps); 
                break;
            }
            case Instr_Say: change = instr_say(ps); break;
            case Instr_Mount: {
                if(!use_resource(1,&ps->remaining_actions)) {ps->dp--; return;}
                change = instr_mount(ps); 
                break;
            }
            case Instr_Dismount: {
                if(!use_resource(1,&ps->remaining_actions)) {ps->dp--; return;}
                change = instr_dismount(ps); 
                break;
            }
            case Instr_Boat: {
                if(!use_resource(1,&ps->remaining_actions)) {ps->dp--; return;}
                change = instr_boat(ps); 
                break;
            } 
            case Instr_BearTrap: {
                if(!use_resource(1,&ps->remaining_actions)) {ps->dp--; return;}
                change = instr_bear_trap(ps); 
                break;
            }
            case Instr_Call: change = instr_call(ps); break;
            case Instr_Return: change = instr_return(ps); break;
            case Instr_Declare: change = instr_declare(ps); break;
            case Instr_GlobalAccess: change = instr_global_access(ps); break;
            case Instr_GlobalAssign: change = instr_global_assign(ps); break;
            case Instr_Index: change = instr_index(ps); break;
            case Instr_ThrowClay: {
                if(!use_resource(1,&ps->remaining_actions)) {ps->dp--; return;}
                change = instr_throw_clay(ps); 
                break;
            }
            case Instr_ClayGolem: {
                if(!use_resource(1,&ps->remaining_actions)) {ps->dp--; return;}
                change = instr_clay_golem(ps); 
                break;
            }
            case Instr_Drop: change = instr_drop(ps); break;
            case Instr_Take: change = instr_take(ps); break;
            case Instr_MineShaft: {
                if(!use_resource(1,&ps->remaining_actions)) {ps->dp--; return;}
                change = instr_mine_shaft(ps); 
                break;
            }
            case Instr_Craft: {
                if(!use_resource(1,&ps->remaining_actions)) {ps->dp--; return;}
                change = instr_craft(ps); 
                break;
            }
            default: return;
        }

        if (change || _gs->feed_point) { 
            auto_viewport_player(ps);
            print_board(); 
            wait(1); 
        }

        kill_players();
        if (_gs->feed_point) { print_board(); wait(1); }
        if (!ps->alive) return;
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
            printf("%s#%i, change directive?:\n0: No change\n1: Reload file\n2: New file\n", ps->name, ps->id);

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
        if (compile_player(path, _gr->stack_size, _gr->program_size_limit, &di)) {
            if (do_free) free(ps->path);
            free(ps->directive);
            free(ps->stack);
            ps->directive = di.directive;
            ps->stack = di.stack;
            ps->sp = 0;
            ps->bp = 0;
            ps->dp = 0;
            ps->directive_len = di.dir_len;
            ps->path = path;
            _log(INFO, "%s: change to %s", ps->name, path);
            return;
        }
    }
}

void nuke_board() {
    for(int y = 0; y < _gs->board_y; y++)
    for(int x = 0; x < _gs->board_x; x++) 
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

int player_alive(player_state* ps) {
    return ps->alive;
}

char* first_team_alive() {
    player_state* ps = first_player(_gs->players, &player_alive);
    if (ps && ps->team)
        return ps->team->team_name;
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
    int pause = 0;
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
            }
            break;
            case ' ': 
                pause = !pause;
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
    const int player_count = _gs->players->count;
    for(int i = 0; i < player_count; i++) {
        handle_input();
        player_state* player = get_player(_gs->players, i);
        int finished_events = update_events(entity.of_player(player), _gs->events, (situation){ .type = NO_SITUATION});
        if (finished_events) { print_board(); wait(1); }
        kill_players();
        if (_gs->feed_point) { print_board(); wait(1); }
        if (player->alive) {
            _log(DEBUG, "Turn: %s (#%i)", player->name, player->id);
            player_turn_default(player);
            set_player_steps_and_actions(player);
        }
        check_win_condition();
    }
    if (_gr->nuke > 0 && _gs->round % _gr->nuke == 0) nuke_board();
}

void play_round() {
    switch (_gr->exec_mode) {
        case 0:
            play_round_default();
            break;
    }
    _log_flush();
    clear_screen();
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
            const int player_count = _gs->players->count;
            for (int i = 0; i < player_count; i++) {
                player_state* player = get_player(_gs->players, i);
                if (!player->alive) continue; 
                if (player->is_original_player)
                    get_new_directive(player);
                clear_screen();
                print_board();
            }
        }
        _gs->round++;
    }
}


// Mode: -x
// Players can change directive before each of their turns
void manual_mode() {
    while(1) {
        const int player_count = _gs->players->count;
        for(int i = 0; i < player_count; i++) {
            player_state* player = get_player(_gs->players, i);
            if (!player->alive) continue;
            if (player->is_original_player)
                get_new_directive(player);
            clear_screen();
            print_board();
            player_turn_default(player);
            check_win_condition();
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

    terminal_echo_off();
    terminal_blocking_read_off();
    terminal_canonical_off();

    clear_screen();
    print_board();

    wait(1);

    _log(INFO, "--- RUNNING ---");
    _log_flush();

    if (_gr->mode == 0) static_mode();
    else if (_gr->mode < 0) manual_mode();
    else if (_gr->mode > 0) dynamic_mode();

    return 0;
}