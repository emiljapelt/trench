#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include "player.h"
#include "game_rules.h"
#include "game_state.h"
#include "visual.h"
#include "util.h"
#include "loader.h"
#include "compiler_interface.h"

typedef enum direction {
    NORTH,
    EAST,
    SOUTH,
    WEST
} direction;


field_state* empty_board(int x, int y) {
    int size = sizeof(field_state)*x*y;
    field_state* brd = malloc(size);
    memset(brd,0,size);
    return brd;
}

int scan_dir(char dir, int x, int y, game_state* gs) {
    int i = 0;
    incr:
    i++;
    switch (dir) {
        case 'n': y--; break;
        case 'e': x++; break;
        case 's': y++; break;
        case 'w': x--; break;
    }
    if (!in_bounds(x,y,gs)) return 0;
    if (get_field(x,y,gs)->trenched) return i;
    goto incr;
}

void move_coord(int x, int y, char d, int* _x, int* _y) {
    switch(d) {
        case 'n': *_x = x; *_y = y-1; break;
        case 'e': *_x = x+1; *_y = y; break;
        case 's': *_x = x; *_y = y+1; break;
        case 'w': *_x = x-1; *_y = y; break;
    }
}

void trench(player_state* ps, game_state* gs) {
    int x = player_x(ps);
    int y = player_y(ps);
    if (!get_field(x,y,gs)->trenched) build_field(x,y,ps->id,gs);
}

void expand(char d, player_state* ps, game_state* gs) {
    int x, y;
    move_coord(player_x(ps), player_y(ps), d, &x, &y);
    if (!in_bounds(x, y, gs)) return;
    if (!get_field(x,y,gs)->trenched) build_field(x,y,ps->id,gs);
}

void move(char d, player_state* ps, game_state* gs) {
    int x, y;
    move_coord(player_x(ps), player_y(ps), d, &x, &y);
    if (!in_bounds(x, y, gs)) return;
    if (get_field(x,y,gs)->destroyed) return;
    set_player_x(ps, x);
    set_player_y(ps, y);
}

void check(char d, player_state* ps, game_state* gs) {
    int x, y;
    move_coord(player_x(ps), player_y(ps), d, &x, &y);
    if (!in_bounds(x, y, gs)) { 
        ps->stack[ps->sp++] = 0;
    }
    else if (get_field(x,y,gs)->trenched) {
        ps->stack[ps->sp++] = 1;
    }
    else {
        ps->stack[ps->sp++] = 0;
    }
}

void shoot(char d, player_state* ps, game_state* gs) {
    int x = player_x(ps);
    int y = player_y(ps);
    move_coord(x, y, d, &x, &y);
    while (in_bounds(x,y,gs)) { 
        shoot_field(x,y,d,gs);
        for(int p = 0; p < gs->player_count; p++) {
            if (player_x(gs->players+p) == x && player_y(gs->players+p) == y && !get_field(x,y,gs)->trenched) {
                kill_player(gs->players+p);
            }
        }
        move_coord(x, y, d, &x, &y);
    }
    print_board(gs);    
    sleep(500);

    x = player_x(ps);
    y = player_y(ps);
    while (in_bounds(x,y,gs)) {
        unshoot_field(x,y,gs);   
        move_coord(x, y, d, &x, &y);
    }
    print_board(gs);
}

void player_turn(game_state* gs, player_state* ps, game_rules* gr) {
    update_bomb_chain(ps,gs);
    char actions = gr->actions;
    char steps = gr->steps;
    while(actions && steps) {
        if (ps->step >= ps->directive_len) return;
        //fprintf(stderr,"%c", ps->directive[ps->step]); sleep(20);
        switch (ps->directive[ps->step++]) {
            case 'W': {
                actions--;
                break;
            }
            case 'P': {
                actions = 0;
                break;
            }
            case 'S': {
                if (player_shots(ps)) {
                    shoot(ps->directive[ps->step++],ps,gs);
                    print_board(gs);
                    mod_player_shots(ps,-1);
                }
                actions--;
                break;
            }
            case 'c': {
                check(ps->directive[ps->step++],ps,gs);
                break;
            }
            case 's': {
                char dir = ps->directive[ps->step++];
                ps->stack[ps->sp++] = scan_dir(dir,player_x(ps),player_y(ps),gs);
                break;
            }
            case 'm': {
                move(ps->directive[ps->step++],ps,gs);
                print_board(gs);
                sleep(500);
                break;
            }
            case 'E': {
                expand(ps->directive[ps->step++],ps,gs);
                print_board(gs);
                sleep(500);
                actions--;
                break;
            }
            case 'T': {
                if (!get_field(player_x(ps),player_y(ps),gs)->trenched)
                    trench(ps, gs);
                print_board(gs);
                sleep(500);
                actions--;
                break;
            }
            case 'F': {
                if (get_field(player_x(ps),player_y(ps),gs)->trenched) 
                    fortify_field(player_x(ps),player_y(ps),gs);
                print_board(gs);
                sleep(500);
                actions--;
                break;
            }
            case 'p': {
                int num_size = numeric_size(ps->directive,ps->step);
                int num = sub_str_to_int(ps->directive,ps->step,num_size);
                ps->stack[ps->sp++] = num;
                ps->step += num_size;
                break;
            }
            case 'B': {
                char d = ps->directive[ps->step++];
                if (player_bombs(ps)) {
                    int x = player_x(ps);
                    int y = player_y(ps);
                    for (int i = ps->stack[--ps->sp]; i > 0; i--)
                        move_coord(x,y,d,&x,&y);
                    target_field(x, y, gs);
                    add_bomb(x, y, ps, gs);
                    print_board(gs);
                    sleep(500);
                    untarget_field(x, y, gs);
                    sleep(250);
                    mod_player_bombs(ps,-1);
                }
                actions--;
                break;
            }
            case '#': {
                int num_size = numeric_size(ps->directive,ps->step);
                int num = sub_str_to_int(ps->directive,ps->step,num_size);
                ps->stack[ps->sp++] = ps->stack[num];
                ps->step += num_size;
                break;
            }
            case '!': {
                int num_size = numeric_size(ps->directive,ps->step);
                int num = sub_str_to_int(ps->directive,ps->step,num_size);
                ps->step = num;
                break;
            }
            case '?': {
                int v = ps->stack[--ps->sp];
                int num_size = numeric_size(ps->directive,ps->step);
                if (v) { 
                    int num = sub_str_to_int(ps->directive,ps->step,num_size);
                    ps->step = num; 
                }
                else ps->step += num_size;
                break;
            }
            case '=': {
                int v0 = ps->stack[--ps->sp];
                int v1 = ps->stack[--ps->sp];
                ps->stack[ps->sp++] = v0 == v1;
                break;
            }
            case '<': {
                int v0 = ps->stack[--ps->sp];
                int v1 = ps->stack[--ps->sp];
                ps->stack[ps->sp++] = v0 < v1;
                break;
            }
            case '-': {
                int v0 = ps->stack[--ps->sp];
                int v1 = ps->stack[--ps->sp];
                ps->stack[ps->sp++] = v0 - v1;
                break;
            }
            case '+': {
                int v0 = ps->stack[--ps->sp];
                int v1 = ps->stack[--ps->sp];
                ps->stack[ps->sp++] = v0 + v1;
                break;
            }
            case '*': {
                int v0 = ps->stack[--ps->sp];
                int v1 = ps->stack[--ps->sp];
                ps->stack[ps->sp++] = v0 * v1;
                break;
            }
            case '/': {
                int v0 = ps->stack[--ps->sp];
                int v1 = ps->stack[--ps->sp];
                ps->stack[ps->sp++] = v0 / v1;
                break;
            }
            case '%': {
                int v0 = ps->stack[--ps->sp];
                int v1 = ps->stack[--ps->sp];
                ps->stack[ps->sp++] = v0 % v1;
                break;
            }
            case '~': {
                int v = ps->stack[--ps->sp];
                ps->stack[ps->sp++] = !v;
                break;
            }
            case '|': {
                int v0 = ps->stack[--ps->sp];
                int v1 = ps->stack[--ps->sp];
                ps->stack[ps->sp++] = v0 || v1;
                break;
            }
            case '&': {
                int v0 = ps->stack[--ps->sp];
                int v1 = ps->stack[--ps->sp];
                ps->stack[ps->sp++] = v0 && v1;
                break;
            }
            case 'a': {
                int v = ps->stack[--ps->sp];
                int target = ps->stack[--ps->sp];
                ps->stack[target] = v;
                break;
            }
            default: return;;
        }
        steps--;
    }
}

void get_new_directive(player_state* ps, const char* comp_path) {      
    while(1) {
        printf("Player %i, change directive?:\n", ps->id);
        char* path = malloc(1001);
        fgets(path, 1000, stdin);
        if (path[0] == '\n') break;
        
        for(int i = 0; i < 1000; i++) 
            if (path[i] == '\n') { path[i] = 0; break; }

        char* new;
        if(get_program_from_file(path, comp_path, &new)) {
            int i = 0;
            while (new[i] != ':') i++;
            free(ps->directive);
            ps->directive = new+i+1;
            ps->step = 0;
            free(path);
            break;
        }
        free(path);
        continue;
        
    }
}

void nuke_board(game_state* gs) {
    for(int y = 0; y < gs->board_y; y++)
    for(int x = 0; x < gs->board_x; x++) 
        explode_field(x, y, gs);
}

int players_alive(game_state* gs) {
    int alive = 0;
    for(int i = 0; i < gs->player_count; i++) if (gs->players[i].alive) alive++;
    return alive;
}

int first_player_alive(game_state* gs) {
    for(int i = 0; i < gs->player_count; i++) if (gs->players[i].alive) return gs->players[i].id;
    printf("No player is alive\n");
    exit(1);
}

void check_win_condition(game_state* gs) {
    switch (players_alive(gs)) {
        case 0:
            printf("GAME OVER: Everyone is dead...\n"); exit(0);
        case 1:
            if (gs->player_count == 1) break;
            else printf("Player %i won!\n", first_player_alive(gs)); exit(0);
        default: break;
    }
}

void play_round(game_state* gs, game_rules* gr) {
    for(int i = 0; i < gs->player_count; i++) {
        if (!gs->players[i].alive) continue; 
        player_turn(gs, gs->players+i, gr);
    }
}


// Players cannot change directive
void static_mode(game_state* gs, game_rules* gr, const char* comp_path) {
    int round = 1;
    while(1) {
        play_round(gs, gr);
        check_win_condition(gs);
        round++;
    }
}

// Players can change directive after 'change' rounds
void dynamic_mode(game_state* gs, game_rules* gr, const char* comp_path, const int change) {
    int round = 1;
    while(1) {
        play_round(gs, gr);
        if (round % change == 0) {
            if (gr->nuke) nuke_board(gs);
            print_board(gs);
            check_win_condition(gs);
            for(int i = 0; i < gs->player_count; i++) {
                if (!gs->players[i].alive) continue; 
                get_new_directive(gs->players+i, comp_path);
            }
        }
        else check_win_condition(gs);
        round++;
    }
}

// Players can change directive before each of their turns
void manual_mode(game_state* gs, game_rules* gr, const char* comp_path) {
    int round = 1;
    while(1) {
        for(int i = 0; i < gs->player_count; i++) {
            if (!gs->players[i].alive) continue; 
            get_new_directive(gs->players+i, comp_path);
            player_turn(gs, gs->players+i, gr);
        }
        check_win_condition(gs);
        round++;
    }
}


int main(int argc, char** argv) {

    if (argc < 3) {
        printf("Too few arguments given, needs: <game_file_path> <compiler_path>\n");
        exit(1);
    }

    char* comp_path = argv[2];
    parsed_game_file* pgf = parse_game_file(argv[1], comp_path);
  
    game_rules gr = {
        pgf->actions,
        pgf->steps,
        pgf->bombs,
        pgf->shots,
        pgf->mode,
        pgf->nuke
    };

    game_state gs = {
        .board_x = pgf->board_x,
        .board_y = pgf->board_y,
        .player_count = pgf->player_count,
        .board = empty_board(pgf->board_x,pgf->board_y)
    };

    create_players(pgf->players, &gs, &gr);
    free_parsed_game_file(pgf);

    print_board(&gs);
    sleep(1000);

    if (gr.mode == 0) static_mode(&gs, &gr, comp_path);
    else if (gr.mode < 0) manual_mode(&gs, &gr, comp_path);
    else if (gr.mode > 0) dynamic_mode(&gs, &gr, comp_path, gr.mode);

    return 0;
}