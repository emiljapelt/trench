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

    int dir_bytes = sizeof(int) * ps->exec->program_len;
    int stack_bytes = sizeof(int) * _gr->stack_size;
    int* new_program = malloc(dir_bytes);
    int* new_stack = malloc(stack_bytes);
    memcpy(new_program, ps->exec->program, dir_bytes);
    memcpy(new_stack, ps->exec->stack, stack_bytes);

    exec_context* new_exec = malloc(sizeof(exec_context));

    new_exec->stack = new_stack;
    new_exec->bp = ps->exec->bp;
    new_exec->sp = ps->exec->sp;
    new_exec->dp = ps->exec->dp;
    new_exec->program = new_program;
    new_exec->program_len = ps->exec->program_len;
    new_exec->steps = _gr->steps;
    new_exec->actions = _gr->actions;


    new_player->is_original_player = 0;
    new_player->team = ps->team;
    new_player->name = strdup(ps->name);
    new_player->exec = new_exec; 
    new_player->path = strdup(ps->path);
    new_player->pager_channel = ps->pager_channel;
    new_player->pager_msgs = array_list.create(10);
    new_player->pre_death_events = array_list.create(10);
    new_player->post_death_events = array_list.create(10);
    new_player->extra_files = NULL;

    for(int r = 0; r < RESOURCE_COUNT; r++) {
        new_player->resources.resource[r].amount = 0;
        new_player->resources.resource[r].max = 0;
    }

    return new_player;
}


void free_player(player_state* player) {

    for(int i = 0; i < player->pre_death_events->count; i++) {
        event* e = get_event(player->pre_death_events, i);
        free_event(e);
    }
    array_list.free(player->pre_death_events);

    for(int i = 0; i < player->post_death_events->count; i++) {
        event* e = get_event(player->post_death_events, i);
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
    free(player->exec->program);
    free(player->exec->stack);
    free(player->exec);
    free(player->path);
    free(player->name);
    free(player);
}