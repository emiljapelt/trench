#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#define sleep(x) Sleep(x*1000)
#elif __unix__
#include <unistd.h>
#define sleep(x) sleep(x)
#endif

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
    int i;
    incr:
    i++;
    switch (dir) {
        case 'n': y--; break;
        case 'e': x++; break;
        case 's': y++; break;
        case 'w': x--; break;
    }
    check:
    if (!in_bounds(x,y,gs)) return 0;
    if (get_field(x,y,gs)->controller) return i;
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

void expand(char d, player_state* ps, game_state* gs) {
    int x, y;
    move_coord(player_x(ps), player_y(ps), d, &x, &y);
    if (!in_bounds(x, y, gs)) return;
    if (get_field(x,y,gs)->controller == 0) build_field(x,y,ps->id,gs);
}

void move(char d, player_state* ps, game_state* gs) {
    int x, y;
    move_coord(player_x(ps), player_y(ps), d, &x, &y);
    if (!in_bounds(x, y, gs)) return;
    if (get_field(x,y,gs)->controller == ps->id) {
        set_player_x(ps, x);
        set_player_y(ps, y);
    }
}

void bomb(int x, int y, game_state* gs) {
    field_state* fld = get_field(x,y,gs);
    if (fld->fortified) fld->fortified = 0;
    else if (fld->controller) {
        fld->controller = 0;
        fld->destroyed = 1;
        for(int p = 0; p < gs->player_count; p++) {
            if (player_x(gs->players+p) == x && player_y(gs->players+p) == y) {
                kill_player(gs->players+p);
            }
        }
    }
}

void check(char d, player_state* ps, game_state* gs) {
    int x, y;
    move_coord(player_x(ps), player_y(ps), d, &x, &y);
    if (!in_bounds(x, y, gs)) { 
        ps->stack[ps->sp++] = 0;
    }
    else if (get_field(x,y,gs)->controller == ps->id) {
        ps->stack[ps->sp++] = 1;
    }
    else {
        ps->stack[ps->sp++] = 0;
    }
}

// Returning 1 means eventful turn, 0 means uneventful
int player_turn(game_state* gs, player_state* ps, game_rules* gr) {
    char actions = gr->actions;
    while(actions) {
        if (ps->step >= ps->directive_len) return 0;
        //printf("%c\n", ps->directive[ps->step]); sleep(1);
        switch (ps->directive[ps->step++]) {
            case 'W': {
                return (actions != gr->actions);
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
                break;
            }
            case 'E': {
                expand(ps->directive[ps->step++],ps,gs);
                actions--;
                break;
            }
            case 'F': {
                if (get_field(player_x(ps),player_y(ps),gs)->controller == ps->id) fortify_field(player_x(ps),player_y(ps),gs);
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
                if (player_b(ps)) {
                    int x = ps->stack[--ps->sp];
                    int y = ps->stack[--ps->sp];
                    bomb(x,y,gs);
                    mod_player_b(ps,-1);
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
            default: return 0;
        }
    }
    return 1;
}

void get_new_directive(player_state* ps, char* comp_path) {       
    printf("Player %i, new directive:\n", ps->id);
    char* path = malloc(1001);
    fgets(path, 1000, stdin);
    if (path[0] != '\n') {
        path[strlen(path)-1] = 0;
        char* new = get_program_from_file(path, comp_path);
        int i = 0;
        while (new[i] != ':') i++;
        ps->directive = new+i+1;
        ps->step = 0;
    }
}

void nuke_board(game_state* gs) {
    for(int y = 0; y < gs->board_y; y++)
    for(int x = 0; x < gs->board_x; x++) 
        bomb(x, y, gs);
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
    int turns = 0;
    for(int i = 0; i < gs->player_count; i++) {
        if (!gs->players[i].alive) continue; 
        if (!player_turn(gs, gs->players+i, gr)) continue;
        sleep(1);
        turns++;
        print_board(gs);
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
        pgf->bombs,
        pgf->change,
        pgf->nuke
    };

    game_state gs = {
        .board_x = pgf->board_x,
        .board_y = pgf->board_y,
        .player_count = pgf->player_count,
        .board = empty_board(pgf->board_x,pgf->board_y)
    };

    // player_init players[] = {
    //     {.x = 5, .y = 5, .directive = "0,0,0:EemeEemeEemeEeFmwFEnmnFEnmnF"},
    //     {.x = 12, .y = 10, .directive = "0,0,0:EnmnFEnmnFp5p5B"},
    //     {.x = 15, .y = 15, .directive = "0,0,0,2:#3p0=?11!14!28Enmnp3p1#3-a!0Ewmw!28"},
    // };

    create_players(pgf->players, &gs, &gr);

    print_board(&gs);
    
    int round = 0;
    while(1) {
        play_round(&gs, &gr);
        round++;
        if (gr.dir_change > 0 && (round % gr.dir_change == 0)) {
            if (gr.nuke) nuke_board(&gs);
            print_board(&gs);
            check_win_condition(&gs);
            for(int i = 0; i < gs.player_count; i++) {
                if (!gs.players[i].alive) continue; 
                get_new_directive(gs.players+i, comp_path);
            }
        }
        else check_win_condition(&gs);
    }


    return 0;
}