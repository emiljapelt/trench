#include "player.h"

int player_x(player_state* ps) {
    return ps->stack[0];
}
void mod_player_x(player_state* ps, int v) {
    ps->stack[0] += v;
}
void set_player_x(player_state* ps, int v) {
    ps->stack[0] = v;
}
int player_y(player_state* ps) {
    return ps->stack[1];
}
void mod_player_y(player_state* ps, int v) {
    ps->stack[1] += v;
}
void set_player_y(player_state* ps, int v) {
    ps->stack[1] = v;
}
int player_bombs(player_state* ps) {
    return ps->stack[2];
}
int player_shots(player_state* ps) {
    return ps->stack[3];
}
void mod_player_bombs(player_state* ps, int v) {
    ps->stack[2] += v;
}
void mod_player_shots(player_state* ps, int v) {
    ps->stack[3] += v;
}
void kill_player(player_state* ps) {
    ps->alive = 0;
}
