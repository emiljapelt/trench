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
#include "game_state.h"
#include "visual.h"
#include "util.h"

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

int player_turn(game_state* gs, player_state* ps) {
    char cont = 1;
    while(cont) {
        if (ps->step >= ps->directive_len) return 0;
        //printf("%c\n", ps->directive[ps->step]); sleep(1);
        switch (ps->directive[ps->step++]) {
            case 'W': {
                cont = 0;
                break;
            }
            case 'c': {
                switch(ps->directive[ps->step++]) {
                    case 'n': if (get_field(player_x(ps),player_y(ps)-1,gs)->controller == ps->id) ps->stack[ps->sp++] = 1; else ps->stack[ps->sp++] = 0; break;
                    case 'e': if (get_field(player_x(ps)+1,player_y(ps),gs)->controller == ps->id) ps->stack[ps->sp++] = 1; else ps->stack[ps->sp++] = 0; break;
                    case 's': if (get_field(player_x(ps),player_y(ps)+1,gs)->controller == ps->id) ps->stack[ps->sp++] = 1; else ps->stack[ps->sp++] = 0; break;
                    case 'w': if (get_field(player_x(ps)-1,player_y(ps),gs)->controller == ps->id) ps->stack[ps->sp++] = 1; else ps->stack[ps->sp++] = 0; break;
                }
                break;
            }
            case 's': {
                char dir = ps->directive[ps->step++];
                ps->stack[ps->sp++] = scan_dir(dir,player_x(ps),player_y(ps),gs);
                break;
            }
            case 'm': {
                switch(ps->directive[ps->step++]) {
                    case 'n': if (get_field(player_x(ps),player_y(ps)-1,gs)->controller == ps->id) mod_player_y(ps,-1); break;
                    case 'e': if (get_field(player_x(ps)+1,player_y(ps),gs)->controller == ps->id) mod_player_x(ps,+1); break;
                    case 's': if (get_field(player_x(ps),player_y(ps)+1,gs)->controller == ps->id) mod_player_y(ps,+1); break;
                    case 'w': if (get_field(player_x(ps)-1,player_y(ps),gs)->controller == ps->id) mod_player_x(ps,-1); break;
                }
                break;
            }
            case 'E': {
                switch(ps->directive[ps->step++]) {
                    case 'n': if (get_field(player_x(ps),player_y(ps)-1,gs)->controller == 0) build_field(player_x(ps),player_y(ps)-1,ps->id,gs); break;
                    case 'e': if (get_field(player_x(ps)+1,player_y(ps),gs)->controller == 0) build_field(player_x(ps)+1,player_y(ps),ps->id,gs); break;
                    case 's': if (get_field(player_x(ps),player_y(ps)+1,gs)->controller == 0) build_field(player_x(ps),player_y(ps)+1,ps->id,gs); break;
                    case 'w': if (get_field(player_x(ps)-1,player_y(ps),gs)->controller == 0) build_field(player_x(ps)-1,player_y(ps),ps->id,gs); break;
                }
                cont = 0;
                break;
            }
            case 'F': {
                if (get_field(player_x(ps),player_y(ps),gs)->controller == ps->id) fortify_field(player_x(ps),player_y(ps),gs);
                cont = 0;
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
                    mod_player_b(ps,-1);
                }
                cont = 0;
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
            default: cont = 0;
        }
    }
    return 1;
}

void play_round(game_state* gs) {
    int turns = 0;
    for(int i = 0; i < gs->player_count; i++) {
        if (!gs->players[i].alive) continue; 
        if (!player_turn(gs, gs->players+i)) continue;
        sleep(1);
        turns++;
        print_board(gs);
    }
}

int main() {

    game_state gs = {
        .board_x = 20,
        .board_y = 20,
        .board = empty_board(20,20)
    };

    player_init players[] = {
        {.x = 5, .y = 5, .directive = "0,0,0:EemeEemeEemeEeFmwFEnmnFEnmnF"},
        {.x = 12, .y = 10, .directive = "0,0,0:EnmnFEnmnFp5p5B"},
        {.x = 15, .y = 15, .directive = "0,0,0,2:#3p0=?11!14!28Enmnp3p1#3-a!0Ewmw!28"},
    };

    create_players(3, players, &gs);

    print_board(&gs);
    while(1) {
        play_round(&gs);
    }


    return 0;
}