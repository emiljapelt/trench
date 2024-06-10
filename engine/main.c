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
#include "compiler_interface.h"


field_state* empty_board(int x, int y) {
    int size = sizeof(field_state)*x*y;
    field_state* brd = malloc(size);
    memset(brd,0,size);
    return brd;
}

int scan(const direction dir, int x, int y, const int power) {
    int result = 0;
    switch (dir) {
        case NORTH: y -= power; break;
        case EAST: x += power; break;
        case SOUTH: y += power; break;
        case WEST: x -= power; break;
    }
    if (!in_bounds(x, y)) goto end;
    if (get_field(x,y)->trenched) result |= TRENCH_FLAG;
    if (get_field(x,y)->destroyed) result |= DESTORYED_FLAG;
    if (get_field(x,y)->mine) result |= MINE_FLAG;
    for(int i = 0; i < _gs->player_count; i++) 
        if (_gs->players[i].x == x && _gs->players[i].y == y) result |= PLAYER_FLAG;

    end:
    return result;
}

int look(const direction dir, int x, int y) {
    int i = 0;
    incr:
    i++;
    switch (dir) {
        case NORTH: y--; break;
        case EAST: x++; break;
        case SOUTH: y++; break;
        case WEST: x--; break;
    }
    if (!in_bounds(x,y)) return 0;
    if (get_field(x,y)->trenched) return i;
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

void trench(int x, int y) {
    if (!in_bounds(x, y)) return;
    if (!get_field(x,y)->trenched) build_field(x,y);
}

void charge(int x, int y, player_state* ps) {
    if (!in_bounds(x,y)) return;
    if (get_field(x,y)->destroyed) return;
    for(int i = 0; i < _gs->player_count; i++) {
        if (!_gs->players[i].alive) continue; 
        if (_gs->players[i].x == x && _gs->players[i].y == y)
            kill_player(_gs->players+i);
    }
    ps->x = x;
    ps->y = y;
}

void mine(int x, int y) {
    if (!in_bounds(x,y)) return;
    if (get_field(x,y)->destroyed) return;
    set_visual(x,y,MINE);
    get_field(x,y)->mine = 1;
    print_board();
    sleep(500);
    unset_visual(x,y);
}

void move(direction d, player_state* ps) {
    int x, y;
    move_coord(ps->x, ps->y, d, &x, &y);
    if (!in_bounds(x, y)) return;
    if (get_field(x,y)->destroyed) return;
    if (get_field(x,y)->mine) {
        kill_player(ps);
        get_field(x,y)->mine = 0;
        _gs->remaining_actions = 0;
        set_visual(x,y,EXPLOSION);
        print_board();
        sleep(250);
        unset_visual(x,y);
        return;
    }
    ps->x = x;
    ps->y = y;
}

void shoot(const direction d, player_state* ps) {
    int x = ps->x;
    int y = ps->y;
    const char* visual; switch (d) {
        case NORTH:
        case SOUTH: 
            visual = BULLETS_NS;
            break;
        case EAST:
        case WEST:
            visual = BULLETS_EW;
            break;
    }
    move_coord(x, y, d, &x, &y);
    while (in_bounds(x,y)) { 
        set_visual(x,y,visual);
        for(int p = 0; p < _gs->player_count; p++) {
            if (_gs->players[p].x == x && _gs->players[p].y == y && !get_field(x,y)->trenched) {
                kill_player(_gs->players+p);
            }
        }
        move_coord(x, y, d, &x, &y);
    }
    print_board();    
    sleep(500);

    x = ps->x;
    y = ps->y;
    while (in_bounds(x,y)) {
        unset_visual(x,y);   
        move_coord(x, y, d, &x, &y);
    }
    print_board();
}

void player_turn(player_state* ps) {
    update_bomb_chain(ps);
    _gs->remaining_actions = _gr->actions;
    _gs->remaining_steps = _gr->steps;
    while(_gs->remaining_actions && _gs->remaining_steps) {
        if (ps->dp >= ps->directive_len) return;
        //fprintf(stderr,"%c", ps->directive[ps->dp]); sleep(500);
        switch (ps->directive[ps->dp++]) {
            case 'W': {
                _gs->remaining_actions--;
                break;
            }
            case 'P': {
                _gs->remaining_actions = 0;
                break;
            }
            case 'S': {
                if (ps->shots) {
                    direction d = (direction)ps->stack[--ps->sp];
                    shoot(d,ps);
                    print_board();
                    ps->shots--;
                }
                _gs->remaining_actions--;
                break;
            }
            case 'l': {
                direction d = (direction)ps->stack[--ps->sp];
                ps->stack[ps->sp++] = look(d,ps->x,ps->y);
                break;
            }
            case 's': {
                direction p = (direction)ps->stack[--ps->sp];
                direction d = (direction)ps->stack[--ps->sp];
                ps->stack[ps->sp++] = scan(d,ps->x,ps->y,p);
                break;
            }
            case 'M': {
                if (ps->bombs) {
                    ps->bombs--;
                    int x, y;
                    direction d = (direction)ps->stack[--ps->sp];
                    move_coord(ps->x, ps->y, d, &x, &y);
                    char kill = 0;
                    for(int i = 0; i < _gs->player_count; i++) 
                        if (_gs->players[i].x == x && _gs->players[i].y == y) { kill_player(_gs->players+i); kill = 1; }
                    if (!kill) mine(x,y);
                    _gs->remaining_steps--;
                }
                break;
            }
            case 'm': {
                direction d = (direction)ps->stack[--ps->sp];
                move(d,ps);
                print_board();
                sleep(250);
                break;
            }
            case 'A': {
                int x, y;
                direction d = (direction)ps->stack[--ps->sp];
                move_coord(ps->x, ps->y, d, &x, &y);
                for(int i = 0; i < _gs->player_count; i++)
                    if (_gs->players[i].x == x && _gs->players[i].y == y) kill_player(_gs->players+i);
                move(d,ps);
                _gs->remaining_actions--;
                break;
            }
            case 'T': {
                int x, y;
                direction d = (direction)ps->stack[--ps->sp];
                move_coord(ps->x, ps->y, d, &x, &y);
                trench(x, y);
                print_board();
                sleep(500);
                _gs->remaining_actions--;
                break;
            }
            case 'F': {
                int x, y;
                direction d = (direction)ps->stack[--ps->sp];
                move_coord(ps->x, ps->y, d, &x, &y);
                if (get_field(x,y)->trenched) 
                    fortify_field(x,y);
                print_board();
                sleep(500);
                _gs->remaining_actions--;
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
                    set_visual(x,y,TARGET);
                    add_bomb(x, y, ps);
                    print_board();
                    sleep(500);
                    unset_visual(x,y);
                    sleep(250);
                    ps->bombs--;
                }
                _gs->remaining_actions--;
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
                        ps->stack[ps->sp++] = _gs->board_x;
                        ps->dp++;
                        break;
                    }
                    case '|': {
                        ps->stack[ps->sp++] = _gs->board_y;
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
            case '\'': {
                int v = ps->stack[--ps->sp];
                int num_size = numeric_size(ps->directive,ps->dp);
                int num = sub_str_to_int(ps->directive,ps->dp,num_size);
                ps->stack[ps->sp++] = num;
                ps->dp += num_size;
                ps->stack[ps->sp++] = v & num;
                break;
            }
            default: return;;
        }
        _gs->remaining_steps--;
    }
}

void get_new_directive(player_state* ps, const char* comp_path) {      
    while(1) {
        printf("Player %i, change directive?:\n", ps->id);
        char* path = malloc(1001);
        memset(path,0,1001);
        fgets(path, 1000, stdin);
        if (path[0] == '\n') break;

        for(int i = 0; i < 1000; i++) 
            if (path[i] == '\n') { path[i] = 0; break; }

        char* directive;
        if(compile_file(path, comp_path, &directive)) {
            int i = 0;
            while(directive[i] != ':') i++; i++;
            memmove(directive, directive+i, strlen(directive+i));
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

void nuke_board() {
    for(int y = 0; y < _gs->board_y; y++)
    for(int x = 0; x < _gs->board_x; x++) 
        explode_field(x, y);
}

int players_alive() {
    int alive = 0;
    for(int i = 0; i < _gs->player_count; i++) if (_gs->players[i].alive) alive++;
    return alive;
}

int first_player_alive() {
    for(int i = 0; i < _gs->player_count; i++) if (_gs->players[i].alive) return _gs->players[i].id;
    printf("No player is alive\n");
    exit(1);
}

void check_win_condition() {
    switch (players_alive(_gs)) {
        case 0:
            printf("GAME OVER: Everyone is dead...\n"); exit(0);
        case 1:
            if (_gs->player_count == 1) break;
            else printf("Player %i won!\n", first_player_alive()); exit(0);
        default: break;
    }
}

void play_round() {
    for(int i = 0; i < _gs->player_count; i++) {
        if (!_gs->players[i].alive) continue; 
        player_turn(_gs->players+i);
    }
    if (_gr->nuke > 0 && _gs->round % _gr->nuke == 0) nuke_board();
    check_win_condition();
}

// Mode: 0
// Players cannot change directive
void static_mode(const char* comp_path) {
    while(1) {
        play_round();
        _gs->round++;
    }
}

// Mode: x
// Players can change directive after 'x' rounds
void dynamic_mode(const char* comp_path) {
    while(1) {
        play_round(_gr);
        if (_gs->round % _gr->mode == 0) {
            for(int i = 0; i < _gs->player_count; i++) {
                if (!_gs->players[i].alive) continue; 
                get_new_directive(_gs->players+i, comp_path);
            }
        }
        _gs->round++;
    }
}


// Mode: -x
// Players can change directive before each of their turns
void manual_mode(const char* comp_path) {
    while(1) {
        for(int i = 0; i < _gs->player_count; i++) {
            if (!_gs->players[i].alive) continue; 
            get_new_directive(_gs->players+i, comp_path);
            player_turn(_gs->players+i);
        }
        if (_gr->nuke > 0 && _gs->round % _gr->nuke == 0) nuke_board();
        check_win_condition();
        _gs->round++;
    }
}


int main(int argc, char** argv) {

    srand((unsigned) time(NULL));

    if (argc < 3) {
        printf("Too few arguments given, needs: <game_file_path> <compiler_path>\n");
        exit(1);
    }

    
    char* game_info;
    compile_file(argv[1], argv[2], &game_info);

    game_rules gr = {
        .bombs = ((int*)game_info)[0],
        .shots = ((int*)game_info)[1],
        .actions = ((int*)game_info)[2],
        .steps = ((int*)game_info)[3],
        .mode = ((int*)game_info)[4],
        .nuke = ((int*)game_info)[5],
    };
    _gr = &gr;

    game_state gs = {
        .round = 1,
        .board_x = ((int*)game_info)[6],
        .board_y = ((int*)game_info)[7],
        .player_count = ((int*)game_info)[8],
        .board = empty_board(((int*)game_info)[6],((int*)game_info)[7])
    };
    _gs = &gs;

    create_players(game_info+(9*sizeof(int)));
    free(game_info);

    print_board();
    sleep(1000);

    if (gr.mode == 0) static_mode(argv[2]);
    else if (gr.mode < 0) manual_mode(argv[2]);
    else if (gr.mode > 0) dynamic_mode(argv[2]);

    return 0;
}