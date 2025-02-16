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
    _gs->remaining_actions = _gr->actions;
    _gs->remaining_steps = _gr->steps;
    
    while(1) {
        int change = 0;
        if (ps->dp >= ps->directive_len) { return; }
        if (!use_resource(1,&_gs->remaining_steps)) return;
        // debug_print(ps);
        switch (ps->directive[ps->dp++]) {
            case Meta_PlayerX: meta_player_x(ps);break;
            case Meta_PlayerY: meta_player_y(ps);break;
            case Meta_BoardX: meta_board_x(ps);break;
            case Meta_BoardY: meta_board_y(ps);break;
            case Meta_Resource: meta_resource(ps);break;
            case Meta_PlayerID: meta_player_id(ps);break;
            case Instr_Wait: {
                if(!use_resource(1,&_gs->remaining_actions)) {ps->dp--;return;}
                break;
            }
            case Instr_Pass: return;
            case Instr_Shoot: { // Shot in direction
                if(!use_resource(1,&_gs->remaining_actions)) {ps->dp--;return;}
                instr_shoot(ps);
                change = 1;
                break;
            }
            case Instr_Look: instr_look(ps); break;
            case Instr_Scan: instr_scan(ps); break;
            case Instr_Mine: { // Place mine
                if(!use_resource(1,&_gs->remaining_actions)) {ps->dp--;return;}
                instr_mine(ps);
                change = 1;
                break;
            }
            case Instr_Move: { // Move
                instr_move(ps);
                change = 1;
                break;
            }
            case Instr_Melee: { // Melee attack
                if(!use_resource(1,&_gs->remaining_actions)) {ps->dp--; return;}
                instr_melee(ps);
                break;
            }
            case Instr_Trench: { // Trench
                if(!use_resource(1,&_gs->remaining_actions)) {ps->dp--; return;}
                instr_trench(ps);
                change = 1;
                break;
            }
            case Instr_Fortify: { // Fortify
                if(!use_resource(1,&_gs->remaining_actions)) {ps->dp--; return;}
                instr_fortify(ps);
                change = 1;
                break;
            }
            case Instr_Random: instr_random_int(ps); break; 
            case Instr_RandomSet: instr_random_range(ps); break;
            case Instr_Place: instr_place(ps); break;
            case Instr_Bomb: { 
                if(!use_resource(1,&_gs->remaining_actions)) {ps->dp--; return;}
                instr_bomb(ps);
                change = 1;
                break;
            }
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
            case Instr_Projection: {
                if(!use_resource(1,&_gs->remaining_actions)) {ps->dp--; return;}
                instr_projection(ps); 
                change = 1;
                break;
            }
            case Instr_Freeze: {
                if(!use_resource(1,&_gs->remaining_actions)) {ps->dp--; return;}
                instr_freeze(ps); 
                change = 1;
                break;
            }
            case Instr_Fireball: {
                if(!use_resource(1,&_gs->remaining_actions)) {ps->dp--; return;}
                instr_fireball(ps); 
                change = 1;
                break;
            }
            case Instr_Meditate: {
                if(!use_resource(1,&_gs->remaining_actions)) {ps->dp--; return;}
                instr_meditate(ps); 
                change = 1;
                break;
            }
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
        void (*instr)(player_state*);
        char inactive;
    } action;   
} turn_action;

turn_action inactive() {
    return (turn_action) {.action_type = INACTIVE, .action = '\0'};
}

turn_action attack_action(void (*instr)(player_state*)) {
    return (turn_action) {.action_type = ATTACK, .action = instr};
}

turn_action defend_action(void (*instr)(player_state*)) {
    return (turn_action) {.action_type = DEFEND, .action = instr};
}

turn_action move_action() {
    return (turn_action) {.action_type = MOVE, .action = '\0'};
}

turn_action player_turn_sync(player_state* ps) {
    _gs->remaining_steps = _gr->steps;
    
    while(1) {
        if (ps->dp >= ps->directive_len) return inactive();
        if (!use_resource(1,&_gs->remaining_steps)) return inactive();
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
            case Instr_Melee: return attack_action(&instr_melee);
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
            case Instr_Projection: defend_action(&instr_projection); break;
            case Instr_Freeze: defend_action(&instr_freeze); break;
            case Instr_Fireball: attack_action(&instr_fireball); break;
            case Instr_Meditate: defend_action(&instr_meditate); break;
            default: return inactive();
        }
    }
}

void get_new_directive(player_state* ps) {      
    while(1) {
        char* path;
        char option;
        //print_board();
        printf("Player %s, change directive?:\n0: No change\n1: Reload file\n2: New file\n", ps->name);
        scanf(" %c",&option);
        switch (option) {
            case '0': return;
            case '1': break;
            case '2': {
                free(ps->path);
                path = malloc(1001);
                memset(path,0,1001);
                puts("New path:");
                scanf(" %1000s", path);
                ps->path = path;
                break;
            }
            default: continue;
        }

        directive_info di;
        if (compile_player(ps->path, _gr->stack_size, &di)) {
            free(ps->directive);
            free(ps->stack);
            ps->directive = di.directive;
            ps->stack = di.stack;
            ps->sp = di.regs;
            ps->dp = 0;
            ps->directive_len = di.dir_len;
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
            if (player->alive) 
                acts[i] = player_turn_sync(player);
        }

    // Defend phase
        change = 0;
        for(i = 0; i < player_count; i++) {
            player_state* player = get_player(_gs->players, i);
            if (player->alive && acts[i].action_type == DEFEND) {
                acts[i].action.instr(player);
                change++;
            }
        }
        if (change || _gs->feed_point) { print_board(); wait(1); }

    // Attack phase
        change = 0;
        for(i = 0; i < player_count; i++) {
            player_state* player = get_player(_gs->players, i);
            if (player->alive && acts[i].action_type == ATTACK) {
                acts[i].action.instr(player);
                change++;
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