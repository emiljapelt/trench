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

//"compiler_wrapper.h"
extern int compile_game(const char* path, game_rules* gr, game_state* gs);
extern int compile_player(const char* path, directive_info* result);


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
            kill_player(_gs->players+i, "Lost a fist fight");
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
        kill_player(ps, "Stepped on a mine");
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
                kill_player(_gs->players+p, "Was gunned down");
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

static inline int use_resource(int amount, int* avail) {
    if (amount > *avail) return 0;
    *avail -= amount;
}

void debug_print(player_state* ps) {
    for(int i = 0; i < ps->sp; i++) {
        fprintf(stderr,"%i, ", ps->stack[i]); sleep(100);
    }
    fprintf(stderr,"\n%c\n", ps->directive[ps->dp]); sleep(500);
}

void player_turn(player_state* ps) {
    update_bomb_chain(ps);
    _gs->remaining_actions = _gr->actions;
    _gs->remaining_steps = _gr->steps;
    
    while(1) {
        if (ps->dp >= ps->directive_len) { return; }
        if (!use_resource(1,&_gs->remaining_steps)) return;
        //debug_print(ps);
        switch (ps->directive[ps->dp++]) {
            case 'W': { // Wait 
                if(!use_resource(1,&_gs->remaining_actions)) {ps->dp--;return;}
                break;
            }
            case 'P': { // Pass turn
                return;
            }
            case 'S': { // Shot in direction
                if(!use_resource(1,&_gs->remaining_actions)) {ps->dp--;return;}
                if (ps->shots) {
                    direction d = (direction)ps->stack[--ps->sp];
                    shoot(d,ps);
                    print_board();
                    ps->shots--;
                }
                break;
            }
            case 'l': { // Look in direction
                direction d = (direction)ps->stack[--ps->sp];
                ps->stack[ps->sp++] = look(d,ps->x,ps->y);
                break;
            }
            case 's': { // Scan field
                direction p = (direction)ps->stack[--ps->sp];
                direction d = (direction)ps->stack[--ps->sp];
                ps->stack[ps->sp++] = scan(d,ps->x,ps->y,p);
                break;
            }
            case 'M': { // Place mine
                if(!use_resource(1,&_gs->remaining_actions)) {ps->dp--;return;}
                if(!use_resource(1,&ps->bombs)) break;
            
                ps->bombs--;
                int x, y;
                direction d = (direction)ps->stack[--ps->sp];
                move_coord(ps->x, ps->y, d, &x, &y);
                char kill = 0;
                for(int i = 0; i < _gs->player_count; i++) 
                    if (_gs->players[i].x == x && _gs->players[i].y == y) { kill_player(_gs->players+i, "Hit by a thrown mine"); kill = 1; }
                if (!kill) mine(x,y);
                break;
            }
            case 'm': { // Move
                direction d = (direction)ps->stack[--ps->sp];
                move(d,ps);
                print_board();
                sleep(250);
                break;
            }
            case 'A': { // Melee attack
                if(!use_resource(1,&_gs->remaining_actions)) {ps->dp--; return;}
                int x, y;
                direction d = (direction)ps->stack[--ps->sp];
                move_coord(ps->x, ps->y, d, &x, &y);
                for(int i = 0; i < _gs->player_count; i++)
                    if (_gs->players[i].x == x && _gs->players[i].y == y) kill_player(_gs->players+i, "Lost a fist fight");
                move(d,ps);
                break;
            }
            case 'T': { // Trench
                if(!use_resource(1,&_gs->remaining_actions)) {ps->dp--; return;}
                int x, y;
                direction d = (direction)ps->stack[--ps->sp];
                move_coord(ps->x, ps->y, d, &x, &y);
                trench(x, y);
                print_board();
                sleep(500);
                break;
            }
            case 'F': { // Fortify
                if(!use_resource(1,&_gs->remaining_actions)) {ps->dp--; return;}
                int x, y;
                direction d = (direction)ps->stack[--ps->sp];
                move_coord(ps->x, ps->y, d, &x, &y);
                if (get_field(x,y)->trenched) 
                    fortify_field(x,y);
                print_board();
                sleep(500);
                break;
            }
            case 'r': { // Place random int
                ps->stack[ps->sp++] = rand();
                break;
            }
            case 'R': { // Select random from stack
                int num = *(int*)((ps->directive)+(ps->dp));
                int pick = ps->stack[ps->sp - ((rand() % num)+1)];
                ps->sp -= num;
                ps->stack[ps->sp++] = pick;
                ps->dp += 4;
                break;
            }
            case 'p': { // Place value
                int num = *(int*)((ps->directive)+(ps->dp));
                ps->stack[ps->sp++] = num;
                ps->dp += 4;
                break;
            }
            case 'B': { // Throw bomb
                if(!use_resource(1,&_gs->remaining_actions)) {ps->dp--; return;}
                int p = ps->stack[--ps->sp];
                direction d = (direction)ps->stack[--ps->sp];
                if(!use_resource(1,&ps->bombs)) break;
            
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
                break;
            }
            case '@': { // Global array access
                int i = ps->stack[--ps->sp];
                int t = ps->stack[--ps->sp];
                if (i > 0 && i < _gr->array) {
                    ps->stack[ps->sp++] = _gs->global_arrays[(t*_gr->array)+i];
                } else ps->stack[ps->sp++] = 0;
                break;
            }
            case '#': { // Access
                switch (ps->directive[ps->dp]) {
                    case 'x': { // Player x position
                        ps->stack[ps->sp++] = ps->x;
                        ps->dp++;
                        break;
                    }
                    case 'y': { // Player y position
                        ps->stack[ps->sp++] = ps->y;
                        ps->dp++;
                        break;
                    }
                    case 'b': { // Player remaining bombs
                        ps->stack[ps->sp++] = ps->bombs;
                        ps->dp++;
                        break;
                    }
                    case 's': { // Player remaining shots
                        ps->stack[ps->sp++] = ps->shots;
                        ps->dp++;
                        break;
                    }
                    case '_': { // Board x size
                        ps->stack[ps->sp++] = _gs->board_x;
                        ps->dp++;
                        break;
                    }
                    case '|': { // Board y size
                        ps->stack[ps->sp++] = _gs->board_y;
                        ps->dp++;
                        break;
                    }
                    case 'g': { // Global array size
                        ps->stack[ps->sp++] = _gr->array;
                        ps->dp++;
                        break;
                    }
                    case 'v': { // Variable access
                        ps->dp++;
                        int num = ps->stack[ps->sp--];
                        ps->stack[ps->sp++] = ps->stack[num];
                        break;
                    }
                }
                break;
            }
            case '!': { // Unconditional go-to
                int num = *(int*)((ps->directive)+(ps->dp));
                ps->dp = num;
                break;
            }
            case '?': { // Conditonal go-to
                int v = ps->stack[--ps->sp];
                if (v) { 
                    int num = *(int*)((ps->directive)+(ps->dp));
                    ps->dp = num; 
                }
                else ps->dp += 4;
                break;
            }
            case '=': { // Equal
                int v0 = ps->stack[--ps->sp];
                int v1 = ps->stack[--ps->sp];
                ps->stack[ps->sp++] = v0 == v1;
                break;
            }
            case '<': { // Less-than
                int v0 = ps->stack[--ps->sp];
                int v1 = ps->stack[--ps->sp];
                ps->stack[ps->sp++] = v0 < v1;
                break;
            }
            case '-': { // Subtract
                int v0 = ps->stack[--ps->sp];
                int v1 = ps->stack[--ps->sp];
                ps->stack[ps->sp++] = v1 - v0;
                break;
            }
            case '+': { // Addition
                int v0 = ps->stack[--ps->sp];
                int v1 = ps->stack[--ps->sp];
                ps->stack[ps->sp++] = v0 + v1;
                break;
            }
            case '*': { // Multiply
                int v0 = ps->stack[--ps->sp];
                int v1 = ps->stack[--ps->sp];
                ps->stack[ps->sp++] = v0 * v1;
                break;
            }
            case '/': { // Divide
                int v0 = ps->stack[--ps->sp];
                int v1 = ps->stack[--ps->sp];
                if (v1 == 0) { kill_player(ps, "Mental break down (div by 0)"); return; }
                ps->stack[ps->sp++] = v0 / v1;
                break;
            }
            case '%': { // Modulo
                int v0 = ps->stack[--ps->sp];
                int v1 = ps->stack[--ps->sp];
                if (v1 == 0) { kill_player(ps, "Mental break down (div by 0)"); return; }
                ps->stack[ps->sp++] = ((v0%v1) + v1)%v1;
                break;
            }
            case '~': { // Not
                int v = ps->stack[--ps->sp];
                ps->stack[ps->sp++] = !v;
                break;
            }
            case '|': { // Logical OR
                int v0 = ps->stack[--ps->sp];
                int v1 = ps->stack[--ps->sp];
                ps->stack[ps->sp++] = v0 || v1;
                break;
            }
            case '&': { // Logical AND
                int v0 = ps->stack[--ps->sp];
                int v1 = ps->stack[--ps->sp];
                ps->stack[ps->sp++] = v0 && v1;
                break;
            }
            case 'a': { // Assignment
                switch (ps->directive[ps->dp++]) {
                    case '_': { // Local
                        int v = ps->stack[--ps->sp];
                        int target = ps->stack[--ps->sp];
                        ps->stack[target] = v;
                        break;
                    }
                    case '@': { // Global
                        int v = ps->stack[--ps->sp];
                        int i = ps->stack[--ps->sp];
                        int t = ps->stack[--ps->sp];
                        if (i > 0 && i < _gr->array) _gs->global_arrays[(t*_gr->array)+i] = v;
                        break;
                    }
                }
                break;
            }
            case '\'': {  // Flag access
                int v = ps->stack[--ps->sp];
                int num = *(int*)((ps->directive)+(ps->dp));
                //ps->stack[ps->sp++] = num;
                ps->dp += 4;
                ps->stack[ps->sp++] = v & num;
                break;
            }
            case 'd': { // Decrease stack
                ps->sp--;
            }
            case 'c': { // Clone top of stack
                ps->stack[ps->sp] = ps->stack[ps->sp - 1];
                ps->sp++;
            }
            case '^': { // Swap
                int v0 = ps->stack[--ps->sp];
                int v1 = ps->stack[--ps->sp];
                ps->stack[ps->sp++] = v0;
                ps->stack[ps->sp++] = v1;
            }
            default: return;;
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

void play_round() {
    for(int i = 0; i < _gs->player_count; i++) {
        if (!_gs->players[i].alive) continue; 
        player_turn(_gs->players+i);
        check_win_condition();
    }
    if (_gr->nuke > 0 && _gs->round % _gr->nuke == 0) nuke_board();
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
            player_turn(_gs->players+i);
            check_win_condition();
        }
        if (_gr->nuke > 0 && _gs->round % _gr->nuke == 0) nuke_board();
        _gs->round++;
    }
}


int main(int argc, char** argv) {

    srand((unsigned) time(NULL));

    if (argc < 2) {
        printf("Too few arguments given, needs: <game_file_path> <compiler_path>\n");
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