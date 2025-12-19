#include "player.h"

#include "log.h"
#include "game_state.h"
#include <stdlib.h>
#include <string.h>

/**
 * Create a copy of the given player state, except:
 * - The new players pager has no messages
 * - The new player has no death events registered
 * - The resource registry is not initialized
 */
player_state* copy_player_state(const player_state* ps) {

    player_state* new_player = malloc(sizeof(player_state));

    int dir_bytes = sizeof(int) * ps->directive_len;
    int stack_bytes = sizeof(int) * _gr->stack_size;
    int* new_directive = malloc(dir_bytes);
    int* new_stack = malloc(stack_bytes);
    memcpy(new_directive, ps->directive, dir_bytes);
    memcpy(new_stack, ps->stack, stack_bytes);

    new_player->alive = 1;
    new_player->is_original_player = 0;
    new_player->death_msg = NULL;
    new_player->team = ps->team;
    new_player->name = strdup(ps->name);
    new_player->id = _gs->id_counter++;
    new_player->stack = new_stack;
    new_player->bp = ps->bp;
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

    for(int r = 0; r < RESOURCE_COUNT; r++) {
        new_player->resources.resource[r].amount = 0;
        new_player->resources.resource[r].max = 0;
    }

    new_player->remaining_actions = _gr->actions;
    new_player->remaining_steps = _gr->steps;

    return new_player;
}


void free_player(player_state* player) {

    for(int i = 0; i < player->pre_death_events->count; i++) {
        event* e = get_event(player->pre_death_events, i);
        free(e->data);
        free(e);
    }
    array_list.free(player->pre_death_events);

    for(int i = 0; i < player->post_death_events->count; i++) {
        event* e = get_event(player->post_death_events, i);
        free(e->data);
        free(e);
    }
    array_list.free(player->post_death_events);

    if (player->extra_files) {
        for(int i = 0; i < player->extra_files->count; i++) {
            free(array_list.get(player->extra_files, i));
        }
        array_list.free(player->extra_files);
    }

    array_list.free(player->pager_msgs);
    free(player->directive);
    free(player->stack);
    free(player->path);
    free(player->name);
    free(player);
}