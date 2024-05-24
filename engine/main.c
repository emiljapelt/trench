#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#include "player.h"
#include "game_rules.h"
#include "game_state.h"
#include "visual.h"
#include "util.h"
#include "loader.h"
#include "compiler_interface.h"


field_state* empty_board(int x, int y) {
    int size = sizeof(field_state)*x*y;
    field_state* brd = malloc(size);
    memset(brd,0,size);
    return brd;
}

int scan_dir(const direction dir, int x, int y, game_state* gs) {
    int i = 0;
    incr:
    i++;
    switch (dir) {
        case NORTH: y--; break;
        case EAST: x++; break;
        case SOUTH: y++; break;
        case WEST: x--; break;
    }
    if (!in_bounds(x,y,gs)) return 0;
    if (get_field(x,y,gs)->trenched) return i;
    goto incr;
}

void move_coord(int x, int y, direction d, int* _x, int* _y) {
    switch(d) {
        case NORTH: *_x = x; *_y = y-1; break;
        case EAST: *_x = x+1; *_y = y; break;
        case SOUTH: *_x = x; *_y = y+1; break;
        case WEST: *_x = x-1; *_y = y; break;
        case HERE: *_x = x; *_y = y; break;
    }
}

void trench(int x, int y, game_state* gs) {
    if (!in_bounds(x, y, gs)) return;
    if (!get_field(x,y,gs)->trenched) build_field(x,y,gs);
}

void move(direction d, player_state* ps, game_state* gs) {
    int x, y;
    move_coord(ps->x, ps->y, d, &x, &y);
    if (!in_bounds(x, y, gs)) return;
    if (get_field(x,y,gs)->destroyed) return;
    ps->x = x;
    ps->y = y;
}

void check(direction d, player_state* ps, game_state* gs) {
    int x, y;
    move_coord(ps->x, ps->y, d, &x, &y);
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

void shoot(const direction d, player_state* ps, game_state* gs) {
    int x = ps->x;
    int y = ps->y;
    move_coord(x, y, d, &x, &y);
    while (in_bounds(x,y,gs)) { 
        shoot_field(x,y,d,gs);
        for(int p = 0; p < gs->player_count; p++) {
            if (gs->players[p].x == x && gs->players[p].y == y && !get_field(x,y,gs)->trenched) {
                kill_player(gs->players+p);
            }
        }
        move_coord(x, y, d, &x, &y);
    }
    print_board(gs);    
    sleep(500);

    x = ps->x;
    y = ps->y;
    while (in_bounds(x,y,gs)) {
        unshoot_field(x,y,gs);   
        move_coord(x, y, d, &x, &y);
    }
    print_board(gs);
}

void player_turn(game_state* gs, player_state* ps, game_rules* gr) {
    update_bomb_chain(ps,gs);
    gs->remaining_actions = gr->actions;
    gs->remaining_steps = gr->steps;
    while(gs->remaining_actions && gs->remaining_steps) {
        if (ps->dp >= ps->directive_len) return;
        //fprintf(stderr,"%c", ps->directive[ps->dp]); sleep(500);
        switch (ps->directive[ps->dp++]) {
            case 'W': {
                gs->remaining_actions--;
                break;
            }
            case 'P': {
                gs->remaining_actions = 0;
                break;
            }
            case 'S': {
                if (ps->shots) {
                    direction d = (direction)ps->stack[--ps->sp];
                    shoot(d,ps,gs);
                    print_board(gs);
                    ps->shots--;
                }
                gs->remaining_actions--;
                break;
            }
            case 'c': {
                direction d = (direction)ps->stack[--ps->sp];
                check(d,ps,gs);
                break;
            }
            case 's': {
                direction d = (direction)ps->stack[--ps->sp];
                sleep(100);
                ps->stack[ps->sp++] = scan_dir(d,ps->x,ps->y,gs);
                break;
            }
            case 'm': {
                direction d = (direction)ps->stack[--ps->sp];
                move(d,ps,gs);
                print_board(gs);
                sleep(250);
                break;
            }
            case 'T': {
                int x, y;
                direction d = (direction)ps->stack[--ps->sp];
                move_coord(ps->x, ps->y, d, &x, &y);
                trench(x, y, gs);
                print_board(gs);
                sleep(500);
                gs->remaining_actions--;
                break;
            }
            case 'F': {
                int x, y;
                direction d = (direction)ps->stack[--ps->sp];
                move_coord(ps->x, ps->y, d, &x, &y);
                if (get_field(x,y,gs)->trenched) 
                    fortify_field(x,y,gs);
                print_board(gs);
                sleep(500);
                gs->remaining_actions--;
                break;
            }
            case 'r': {
                ps->stack[ps->sp++] = rand();
            }
            case 'R': {
                int num_size = numeric_size(ps->directive,ps->dp);
                int num = sub_str_to_int(ps->directive,ps->dp,num_size);
                int pick = ps->stack[ps->sp - ((rand() % num)+1)];
                ps->sp -= num;
                ps->stack[ps->sp++] = pick;
                ps->dp += num_size;
                break;
            }
            case 'p': {
                int num_size = numeric_size(ps->directive,ps->dp);
                int num = sub_str_to_int(ps->directive,ps->dp,num_size);
                ps->stack[ps->sp++] = num;
                ps->dp += num_size;
                break;
            }
            case 'B': {
                int p = ps->stack[--ps->sp];
                direction d = (direction)ps->stack[--ps->sp];
                if (ps->bombs) {
                    int x = ps->x;
                    int y = ps->y;
                    for (int i = p; i > 0; i--)
                        move_coord(x,y,d,&x,&y);
                    target_field(x, y, gs);
                    add_bomb(x, y, ps, gs);
                    print_board(gs);
                    sleep(500);
                    untarget_field(x, y, gs);
                    sleep(250);
                    ps->bombs--;
                }
                gs->remaining_actions--;
                break;
            }
            case '#': {
                switch (ps->directive[ps->dp]) {
                    case 'x': {
                        ps->stack[ps->sp++] = ps->x;
                        ps->dp++;
                        break;
                    }
                    case 'y': {
                        ps->stack[ps->sp++] = ps->y;
                        ps->dp++;
                        break;
                    }
                    case 'b': {
                        ps->stack[ps->sp++] = ps->bombs;
                        ps->dp++;
                        break;
                    }
                    case 's': {
                        ps->stack[ps->sp++] = ps->shots;
                        ps->dp++;
                        break;
                    }
                    case '_': {
                        ps->stack[ps->sp++] = gs->board_x;
                        ps->dp++;
                        break;
                    }
                    case '|': {
                        ps->stack[ps->sp++] = gs->board_y;
                        ps->dp++;
                        break;
                    }
                    default: {
                        int num_size = numeric_size(ps->directive,ps->dp);
                        int num = sub_str_to_int(ps->directive,ps->dp,num_size);
                        ps->stack[ps->sp++] = ps->stack[num];
                        ps->dp += num_size;
                    }
                }
                break;
            }
            case '!': {
                int num_size = numeric_size(ps->directive,ps->dp);
                int num = sub_str_to_int(ps->directive,ps->dp,num_size);
                ps->dp = num;
                break;
            }
            case '?': {
                int v = ps->stack[--ps->sp];
                int num_size = numeric_size(ps->directive,ps->dp);
                if (v) { 
                    int num = sub_str_to_int(ps->directive,ps->dp,num_size);
                    ps->dp = num; 
                }
                else ps->dp += num_size;
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
        gs->remaining_steps--;
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

        char* directive;
        if(get_program_from_file(path, comp_path, NULL, &directive)) {
            free(ps->directive);
            ps->directive = directive;
            ps->dp = 0;
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
    if (gr->nuke > 0 && gs->round % gr->nuke == 0) nuke_board(gs);
    check_win_condition(gs);
}

// Mode: 0
// Players cannot change directive
void static_mode(game_state* gs, game_rules* gr, const char* comp_path) {
    while(1) {
        play_round(gs, gr);
        gs->round++;
    }
}

// Mode: x
// Players can change directive after 'x' rounds
void dynamic_mode(game_state* gs, game_rules* gr, const char* comp_path) {
    while(1) {
        play_round(gs, gr);
        if (gs->round % gr->mode == 0) {
            for(int i = 0; i < gs->player_count; i++) {
                if (!gs->players[i].alive) continue; 
                get_new_directive(gs->players+i, comp_path);
            }
        }
        gs->round++;
    }
}


// Mode: -x
// Players can change directive before each of their turns
void manual_mode(game_state* gs, game_rules* gr, const char* comp_path) {
    while(1) {
        for(int i = 0; i < gs->player_count; i++) {
            if (!gs->players[i].alive) continue; 
            get_new_directive(gs->players+i, comp_path);
            player_turn(gs, gs->players+i, gr);
        }
        if (gr->nuke > 0 && gs->round % gr->nuke == 0) nuke_board(gs);
        check_win_condition(gs);
        gs->round++;
    }
}


int main(int argc, char** argv) {

    srand((unsigned) time(NULL));

    if (argc < 3) {
        printf("Too few arguments given, needs: <game_file_path> <compiler_path>\n");
        exit(1);
    }

    char* comp_path = argv[2];

    parsed_game_file* pgf = parse_game_file(argv[1], comp_path);

    game_rules gr = {
        .actions = pgf->actions,
        .steps = pgf->steps,
        .bombs = pgf->bombs,
        .shots = pgf->shots,
        .mode = pgf->mode,
        .nuke = pgf->nuke
    };

    game_state gs = {
        .round = 1,
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
    else if (gr.mode > 0) dynamic_mode(&gs, &gr, comp_path);

    return 0;
}