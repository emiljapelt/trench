#include "player.h"

#include "game_state.h"
#include <stdlib.h>
#include <string.h>

/**
 * Create a copy of the given player state, except:
 * - The new players pager has no messages
 * - The new player has no death events registered
 */
player_state* copy_player_state(const player_state* ps) {

    player_state* new_player = malloc(sizeof(player_state));

    int dir_bytes = sizeof(int) * ps->directive_len;
    int stack_bytes = sizeof(int) * ps->stack_len;
    int* new_directive = malloc(dir_bytes);
    int* new_stack = malloc(stack_bytes);
    memcpy(new_directive, ps->directive, dir_bytes);
    memcpy(new_stack, ps->stack, stack_bytes);

    new_player->alive = 1;
    new_player->death_msg = NULL;
    new_player->team = ps->team;
    new_player->name = strdup(ps->name);
    new_player->id = _gs->players->count;
    new_player->stack = new_stack;
    new_player->stack_len = ps->stack_len;
    new_player->sp = ps->sp;
    new_player->path = strdup(ps->path);
    new_player->directive = new_directive;
    new_player->directive_len = ps->directive_len;
    new_player->dp = ps->dp;
    new_player->location = (location) { .type = VOID_LOCATION };
    new_player->pager_channel = ps->pager_channel;
    new_player->pager_msgs = array_list.create(10);
    new_player->pre_death_events = array_list.create(10);
    new_player->post_death_events = array_list.create(10);
    new_player->resources = copy_resource_registry(ps->resources);

    return new_player;
}
