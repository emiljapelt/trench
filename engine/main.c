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
#include "instructions.h"
#include "compiler_wrapper.h"

void debug_print(player_state* ps) {
    fprintf(stderr,"\nPlayer %s(%i)\n%i\n", ps->name, ps->id, ps->directive[ps->dp]); wait(0.5);
    for(int i = 0; i < ps->sp; i++) {
        fprintf(stderr,"%i, ", ps->stack[i]); wait(0.1);
    }
    wait(1);
}

void try_kill_player(player_state* ps) {
    if (ps->alive && ps->death_msg != NULL) {
        update_events(ps, ps->pre_death_events);
        if (ps->alive && ps->death_msg != NULL) {
            kill_player(ps);
            update_events(ps, ps->post_death_events);
        }
    }
}

void kill_players() {
    each_player(_gs->players, &try_kill_player);
}

void player_turn_async(player_state* ps) {
    while(1) {
        int change = 0;
        if (ps->dp >= ps->directive_len) { return; }
        if (!use_resource(1,&ps->remaining_steps)) return;
        //debug_print(ps);
        switch (ps->directive[ps->dp++]) {
            case Meta_PlayerX: change = meta_player_x(ps);break;
            case Meta_PlayerY: change = meta_player_y(ps);break;
            case Meta_BoardX: change = meta_board_x(ps);break;
            case Meta_BoardY: change = meta_board_y(ps);break;
            case Meta_Resource: change = meta_resource(ps);break;
            case Meta_PlayerID: change = meta_player_id(ps);break;
            case Instr_Wait: {
                if(!use_resource(1,&ps->remaining_actions)) {ps->dp--;return;}
                break;
            }
            case Instr_Pass: return;
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
            case Instr_Collect: change = instr_collect(ps); break;
            case Instr_Say: change = instr_say(ps); break;
            default: return;
        }

        if (change || _gs->feed_point) { print_board(); wait(1); }

        kill_players();
        if (_gs->feed_point) { print_board(); wait(1); }
    }
}

typedef enum player_action {
    INACTIVE,
    MOVE,
    ATTACK,
    DEFEND
} player_action;

typedef struct turn_action {
    player_action action_type;
    union {
        int (*instr)(player_state*);
        char inactive;
    } action;   
} turn_action;

turn_action inactive() {
    return (turn_action) {.action_type = INACTIVE, .action = '\0'};
}

turn_action attack_action(int (*instr)(player_state*)) {
    return (turn_action) {.action_type = ATTACK, .action = instr};
}

turn_action defend_action(int (*instr)(player_state*)) {
    return (turn_action) {.action_type = DEFEND, .action = instr};
}

turn_action move_action() {
    return (turn_action) {.action_type = MOVE, .action = '\0'};
}

turn_action player_turn_sync(player_state* ps) {
    while(1) {
        if (ps->dp >= ps->directive_len) return inactive();
        if (!use_resource(1,&ps->remaining_steps)) return inactive();
        // debug_print(ps);
        switch (ps->directive[ps->dp++]) {
            case Meta_PlayerX: meta_player_x(ps);break;
            case Meta_PlayerY: meta_player_y(ps);break;
            case Meta_BoardX: meta_board_x(ps);break;
            case Meta_BoardY: meta_board_y(ps);break;
            case Meta_Resource: meta_resource(ps);break;
            case Meta_PlayerID: meta_player_id(ps);break;
            case Instr_Wait: break;
            case Instr_Pass: return inactive();
            case Instr_Shoot: return attack_action(&instr_shoot);
            case Instr_Look: instr_look(ps); break;
            case Instr_Scan: instr_scan(ps); break;
            case Instr_Mine: return attack_action(&instr_mine);
            case Instr_Move: return move_action();
            case Instr_Chop: return attack_action(&instr_chop);
            case Instr_Trench: return defend_action(&instr_trench);
            case Instr_Fortify: return defend_action(&instr_fortify);
            case Instr_Random: instr_random_int(ps); break;
            case Instr_RandomSet: instr_random_range(ps); break;
            case Instr_Place: instr_place(ps); break;
            case Instr_Bomb: return attack_action(&instr_bomb);
            case Instr_Access: instr_access(ps); break; 
            case Instr_GoTo: instr_goto(ps); break;
            case Instr_GoToIf: instr_goto_if(ps); break;
            case Instr_Eq: instr_eq(ps); break;
            case Instr_Lt: instr_lt(ps); break;
            case Instr_Sub: instr_sub(ps); break;
            case Instr_Add: instr_add(ps); break;
            case Instr_Mul: instr_mul(ps); break;
            case Instr_Div: instr_div(ps); break;
            case Instr_Mod: instr_mod(ps); break;
            case Instr_Not: instr_not(ps); break;
            case Instr_Or: instr_or(ps); break;
            case Instr_And: instr_and(ps); break;
            case Instr_Assign: instr_assign(ps); break;
            case Instr_FieldProp: instr_field_prop(ps); break;
            case Instr_DecStack: instr_dec_stack(ps); break;
            case Instr_Copy: instr_copy(ps); break;
            case Instr_Swap: instr_swap(ps); break;
            case Instr_Read: instr_read(ps); break;
            case Instr_Write: instr_write(ps); break;
            case Instr_Projection: return defend_action(&instr_projection); break;
            case Instr_Freeze: return defend_action(&instr_freeze); break;
            case Instr_Fireball: return attack_action(&instr_fireball); break;
            case Instr_Meditate: return defend_action(&instr_meditate); break;
            case Instr_ManaDrain: return defend_action(&instr_mana_drain); break;
            case Instr_PagerSet: instr_pager_set(ps); break;
            case Instr_PagerRead: instr_pager_read(ps); break; 
            case Instr_PagerWrite: instr_pager_write(ps); break;
            case Instr_Wall: return defend_action(&instr_wall); break;
            case Instr_PlantTree: return defend_action(&instr_plant_tree); break;
            case Instr_Bridge: return defend_action(&instr_bridge); break;
            case Instr_Collect: instr_collect(ps); break;
            case Instr_Say: instr_say(ps); break;
            default: return inactive();
        }
    }
}

void get_new_directive(player_state* ps) {      
    while(1) {
        char* path;
        char option;
        printf("%s#%i, change directive?:\n0: No change\n1: Reload file\n2: New file\n", ps->name, ps->id);
        scanf(" %c",&option);
        switch (option) {
            case '0': return;
            case '1': {
                path = ps->path;
                break;
            };
            case '2': {
                free(ps->path);
                path = malloc(201);
                memset(path,0,201);
                puts("New path:");
                scanf(" %200s", path);
                break;
            }
            default: continue;
        }

        directive_info di;
        printf("loading %s...\n", path);
        if (compile_player(path, _gr->stack_size, &di)) {
            free(ps->directive);
            free(ps->stack);
            ps->directive = di.directive;
            ps->stack = di.stack;
            ps->sp = di.regs;
            ps->dp = 0;
            ps->directive_len = di.dir_len;
            ps->path = path;
            return;
        }
    }
}

void nuke_board() {
    for(int y = 0; y < _gs->board_y; y++)
    for(int x = 0; x < _gs->board_x; x++) 
        fields.destroy_field(x, y, "Got nuked");
}

int teams_alive() {
    int alive = 0;
    for(int i = 0; i < _gs->team_count; i++) 
        if (_gs->team_states[i].members_alive) 
            alive++;
    return alive;
}

int player_alive(player_state* ps) {
    return ps->alive;
}

char* first_team_alive() {
    player_state* ps = first_player(_gs->players, &player_alive);
    if (ps)
        return ps->team->team_name;
    printf("No player is alive\n");
    exit(1);
}

void check_win_condition() {
    switch (teams_alive(_gs)) {
        case 0:
            printf("GAME OVER: Everyone is dead...\n"); 
            printf("seed: %i\n", _gr->seed);
            exit(0);
        case 1:
            if (_gs->team_count == 1) break;
            else {
                printf("Team %s won!\n", first_team_alive()); 
                printf("seed: %i\n", _gr->seed);
                exit(0);
            }
        default: break;
    }
}


void play_round_sync() {
        const int player_count = _gs->players->count;
        int change = 0;
        int i = 0;

    // Pre phase
        change = 0;
        for(i = 0; i < player_count; i++) {
            player_state* player = get_player(_gs->players, i);
            int finished_events = update_events(player, _gs->events);
            if (finished_events) change++;
        }
        if (change || _gs->feed_point) { print_board(); wait(1); }

        kill_players();
        if (_gs->feed_point) { print_board(); wait(1); }

    // Step phase
        turn_action* acts = malloc(sizeof(turn_action) * player_count);
        change = 0;
        for(i = 0; i < player_count; i++) {
            player_state* player = get_player(_gs->players, i);
            if (player->alive) {
                acts[i] = player_turn_sync(player);
                set_player_steps_and_actions(player);
            }
        }

    // Defend phase
        change = 0;
        for(i = 0; i < player_count; i++) {
            player_state* player = get_player(_gs->players, i);
            if (player->alive && acts[i].action_type == DEFEND) {
                change += acts[i].action.instr(player);
            }
        }
        if (change || _gs->feed_point) { print_board(); wait(1); }

    // Attack phase
        change = 0;
        for(i = 0; i < player_count; i++) {
            player_state* player = get_player(_gs->players, i);
            if (player->alive && acts[i].action_type == ATTACK) {
                change += acts[i].action.instr(player);
            }
        }
        if (change || _gs->feed_point) { print_board(); wait(1); }

        kill_players();
        if (_gs->feed_point) { print_board(); wait(1); }

    // Move phase
        change = 0;
        for(i = 0; i < player_count; i++) {
            player_state* player = get_player(_gs->players, i);
            if (player->alive && acts[i].action_type == MOVE) {
                instr_move(player);
                change++;
            }
        }
        if (change || _gs->feed_point) { print_board(); wait(1); }

        if (_gr->nuke > 0 && _gs->round % _gr->nuke == 0) { 
            nuke_board();
            print_board();
            wait(1);
        }

        kill_players();
        if (_gs->feed_point) { print_board(); wait(1); }

    // Check win condition
        check_win_condition();
        free(acts);
}

/*
    Players complete their turn seperately one at a time. 
    Once each player has taken a turn, a round has passed.
*/
void play_round_async() {
    const int player_count = _gs->players->count;
    for(int i = 0; i < player_count; i++) {
        player_state* player = get_player(_gs->players, i);
        int finished_events = update_events(player, _gs->events);
        if (finished_events) { print_board(); wait(1); }
        kill_players();
        if (_gs->feed_point) { print_board(); wait(1); }
        if (player->alive) {
            player_turn_async(player);
            set_player_steps_and_actions(player);
            check_win_condition();
        }
    }
    if (_gr->nuke > 0 && _gs->round % _gr->nuke == 0) nuke_board();
}

void play_round() {
    switch (_gr->exec_mode) {
        case 0:
            play_round_async();
            break;
        case 1:
            play_round_sync();
            break;
    }
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
        play_round(_gr);
        if (_gs->round % _gr->mode == 0) {
            const int player_count = _gs->players->count;
            for (int i = 0; i < player_count; i++) {
                player_state* player = get_player(_gs->players, i);
                if (!player->alive) continue; 
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
            get_new_directive(player);
            clear_screen();
            print_board();
            player_turn_async(player);
            check_win_condition();
        }

        if (_gr->nuke > 0 && _gs->round % _gr->nuke == 0) nuke_board();
        _gs->round++;
    }
}


int main(int argc, char** argv) {

    // srand((unsigned) time(NULL));

    if (argc < 2) {
        printf("Too few arguments given, needs: <game_file_path>\n");
        exit(1);
    }
    caml_startup(argv);
    _gr = malloc(sizeof(game_rules));
    _gs = malloc(sizeof(game_state));

    if(!compile_game(argv[1], _gr, _gs)) return 1;

    clear_screen();
    print_board();
    wait(1);

    if (_gr->mode == 0) static_mode();
    else if (_gr->mode < 0) manual_mode();
    else if (_gr->mode > 0) dynamic_mode();

    return 0;
}