#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#include <caml/callback.h>

#include "player.h"
#include "game_rules.h"
#include "game_state.h"
#include "visual.h"
#include "util.h"
#include "instructions.h"

//"compiler_wrapper.h"
extern int compile_game(const char* path, game_rules* gr, game_state* gs);
extern int compile_player(const char* path, directive_info* result);

void debug_print(player_state* ps) {
    for(int i = 0; i < ps->sp; i++) {
        fprintf(stderr,"%i, ", ps->stack[i]); sleep(100);
    }
    fprintf(stderr,"\n%c\n", ps->directive[ps->dp]); sleep(500);
}

void player_turn_async(player_state* ps) {
    _gs->remaining_actions = _gr->actions;
    _gs->remaining_steps = _gr->steps;
    
    while(1) {
        int change = 0;
        if (ps->dp >= ps->directive_len) { return; }
        if (!use_resource(1,&_gs->remaining_steps)) return;
        //debug_print(ps);
        switch (ps->directive[ps->dp++]) {
            case 'W': { // Wait 
                if(!use_resource(1,&_gs->remaining_actions)) {ps->dp--;return;}
                break;
            }
            case 'P': return;
            case 'S': { // Shot in direction
                if(!use_resource(1,&_gs->remaining_actions)) {ps->dp--;return;}
                instr_shoot(ps);
                change = 1;
                break;
            }
            case 'l': instr_look(ps); break;
            case 's': instr_scan(ps); break;
            case 'M': { // Place mine
                if(!use_resource(1,&_gs->remaining_actions)) {ps->dp--;return;}
                instr_mine(ps);
                change = 1;
                break;
            }
            case 'm': { // Move
                instr_move(ps);
                change = 1;
                break;
            }
            case 'A': { // Melee attack
                if(!use_resource(1,&_gs->remaining_actions)) {ps->dp--; return;}
                instr_melee(ps);
                break;
            }
            case 'T': { // Trench
                if(!use_resource(1,&_gs->remaining_actions)) {ps->dp--; return;}
                instr_trench(ps);
                change = 1;
                break;
            }
            case 'F': { // Fortify
                if(!use_resource(1,&_gs->remaining_actions)) {ps->dp--; return;}
                instr_fortify(ps);
                change = 1;
                break;
            }
            case 'r': instr_random_int(ps); break; 
            case 'R': instr_random_range(ps); break;
            case 'p': instr_place(ps); break;
            case 'B': { 
                if(!use_resource(1,&_gs->remaining_actions)) {ps->dp--; return;}
                instr_bomb(ps);
                change = 1;
                break;
            }
            case '@': instr_global_access(ps); break;
            case '#': instr_access(ps); break;
            case '!': instr_goto(ps); break;
            case '?': instr_goto_if(ps); break;
            case '=': instr_eq(ps); break;
            case '<': instr_lt(ps); break;
            case '-': instr_sub(ps); break;
            case '+': instr_add(ps); break;
            case '*': instr_mul(ps); break;
            case '/': instr_div(ps); break;
            case '%': instr_mod(ps); break;
            case '~': instr_not(ps); break;
            case '|': instr_or(ps); break;
            case '&': instr_and(ps); break;
            case 'a': instr_assign(ps); break;
            case '\'': instr_flag_access(ps); break;
            case 'd': instr_dec_stack(ps); break;
            case 'c': instr_clone(ps); break;
            case '^': instr_swap(ps); break;
            default: return;
        }

        if (change || _gs->feed_point) { print_board(); sleep(1000); }

        for(int i = 0; i < _gs->player_count; i++)
            if (_gs->players[i].death_msg != NULL)
                kill_player(_gs->players+i);
        if (_gs->feed_point) { print_board(); sleep(1000); }

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
        //debug_print(ps);
        switch (ps->directive[ps->dp++]) {
            case 'W': break;
            case 'P': return inactive();
            case 'S': return attack_action(&instr_shoot);
            case 'l': instr_look(ps); break;
            case 's': instr_scan(ps); break;
            case 'M': return attack_action(&instr_mine);
            case 'm': return move_action();
            case 'A': return attack_action(&instr_melee);
            case 'T': return defend_action(&instr_trench);
            case 'F': return defend_action(&instr_fortify);
            case 'r': instr_random_int(ps); break;
            case 'R': instr_random_range(ps); break;
            case 'p': instr_place(ps); break;
            case 'B': return attack_action(&instr_bomb);
            case '@': instr_global_access(ps); break;
            case '#': instr_access(ps); break; 
            case '!': instr_goto(ps); break;
            case '?': instr_goto_if(ps); break;
            case '=': instr_eq(ps); break;
            case '<': instr_lt(ps); break;
            case '-': instr_sub(ps); break;
            case '+': instr_add(ps); break;
            case '*': instr_mul(ps); break;
            case '/': instr_div(ps); break;
            case '%': instr_mod(ps); break;
            case '~': instr_not(ps); break;
            case '|': instr_or(ps); break;
            case '&': instr_and(ps); break;
            case 'a': instr_assign(ps); break;
            case '\'': instr_flag_access(ps); break;
            case 'd': instr_dec_stack(ps); break;
            case 'c': instr_clone(ps); break;
            case '^': instr_swap(ps); break;
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
        if (compile_player(ps->path, &di)) {
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
        explode_field(x, y);
}

int teams_alive() {
    int alive = 0;
    for(int i = 0; i < _gs->team_count; i++) if (_gs->team_states[i].members_alive) alive++;
    return alive;
}

int first_team_alive() {
    for(int i = 0; i < _gs->player_count; i++) if (_gs->players[i].alive) return _gs->players[i].team;
    printf("No player is alive\n");
    exit(1);
}

void check_win_condition() {
    switch (teams_alive(_gs)) {
        case 0:
            printf("GAME OVER: Everyone is dead...\n"); exit(0);
        case 1:
            if (_gs->team_count == 1) break;
            else printf("Team %i won!\n", first_team_alive()); exit(0);
        default: break;
    }
}


void play_round_sync() {
    // Bomb chain update
        //for(int i = 0; i < _gs->player_count; i++) 
        //    update_bomb_chain(_gs->players+i);

        //for(int i = 0; i < _gs->player_count; i++)
        //    if (_gs->players[i].death_msg != NULL)
        //        kill_player(_gs->players+i);
    
        int change;
    // Step phase
        turn_action* acts = malloc(sizeof(turn_action)*_gs->player_count);
        for(int i = 0; i < _gs->player_count; i++) 
            if (_gs->players[i].alive) acts[i] = player_turn_sync(_gs->players+i);

    // Defend phase
        change = 0;
        for(int i = 0; i < _gs->player_count; i++) 
            if (_gs->players[i].alive && acts[i].action_type == DEFEND) {
                acts[i].action.instr(_gs->players+i);
                change++;
            }
        if (change || _gs->feed_point) { print_board(); sleep(1000); }

    // Attack phase
        char* ks = malloc(_gs->player_count);
        change = 0;
        for(int i = 0; i < _gs->player_count; i++) 
            if (_gs->players[i].alive && acts->action_type == ATTACK) {
                acts[i].action.instr(_gs->players+i);
                change++;
            }
        if (change || _gs->feed_point) { print_board(); sleep(1000); }

        for(int i = 0; i < _gs->player_count; i++)
            if (_gs->players[i].death_msg != NULL)
                kill_player(_gs->players+i);
        if (_gs->feed_point) { print_board(); sleep(1000); }

    // Move phase
        change = 0;
        for(int i = 0; i < _gs->player_count; i++) 
            if (_gs->players[i].alive && acts[i].action_type == MOVE) {
                instr_move(_gs->players+i);
                change++;
            }
        if (change || _gs->feed_point) { print_board(); sleep(1000); }

        if (_gr->nuke > 0 && _gs->round % _gr->nuke == 0) { 
            nuke_board();
            print_board();
            sleep(1000);
        }

        for(int i = 0; i < _gs->player_count; i++)
            if (_gs->players[i].death_msg != NULL)
                kill_player(_gs->players+i);
        if (_gs->feed_point) { print_board(); sleep(1000); }

    // Check win condition
        check_win_condition();
        free(acts);
}

/*
    Players complete their turn seperately one at a time. 
    Once each player has taken a turn, a round has passed.
*/
void play_round_async() {
    for(int i = 0; i < _gs->player_count; i++) if (_gs->players[i].alive) {
        //update_bomb_chain(_gs->players+i);
        //check_win_condition();
        player_turn_async(_gs->players+i);
        check_win_condition();
    }
    if (_gr->nuke > 0 && _gs->round % _gr->nuke == 0) nuke_board();
}

void play_round() {
    play_round_async();
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
            for(int i = 0; i < _gs->player_count; i++) {
                if (!_gs->players[i].alive) continue; 
                get_new_directive(_gs->players+i);
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
        for(int i = 0; i < _gs->player_count; i++) {
            if (!_gs->players[i].alive) continue; 
            get_new_directive(_gs->players+i);
            print_board();
            player_turn_async(_gs->players+i);
            check_win_condition();
        }
        if (_gr->nuke > 0 && _gs->round % _gr->nuke == 0) nuke_board();
        _gs->round++;
    }
}


int main(int argc, char** argv) {

    srand((unsigned) time(NULL));

    if (argc < 2) {
        printf("Too few arguments given, needs: <game_file_path>\n");
        exit(1);
    }

    caml_startup(argv);
    _gr = malloc(sizeof(game_rules));
    _gs = malloc(sizeof(game_state));
    if(!compile_game(argv[1], _gr, _gs)) return 1;

    print_board();
    sleep(1000);

    if (_gr->mode == 0) static_mode();
    else if (_gr->mode < 0) manual_mode();
    else if (_gr->mode > 0) dynamic_mode();

    return 0;
}