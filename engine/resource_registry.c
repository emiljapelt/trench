#include "resource_registry.h"

#include "game_state.h"
#include "game_rules.h"

#include <string.h>
#include <stdlib.h>

extern game_state* _gs;
extern game_rules* _gr;

resource_registry* create_resource_registry(int player_count, int size) {
    resource_registry* registry = malloc(sizeof(resource_registry));

    resource_node** buckets = malloc(sizeof(resource_node*) * size);
    
    resource_registry registry_value = {
      .size = size,
      .buckets = buckets
    };

    memcpy(registry, &registry_value, sizeof(resource_registry));
    
    return registry;
}


int get_bucket_index(const char* name, int size) {
    int bucket_index = 0;
    {
        int name_len = strlen(name);
        for(int i = 0; i < name_len; i++) bucket_index += name[i];
        bucket_index %= size;
    }
    return bucket_index;
}

void init_resource(resource_registry* registry, const char* name, int init_amount, int player_count) {
    resource_node* node = malloc(sizeof(resource_node));

    int* player_registry = malloc(sizeof(int) * player_count);
    for(int i = 0; i < _gs->player_count; i++)
        player_registry[i] = init_amount;

    int bucket_index = get_bucket_index(name, registry->size);

    resource_node node_value = {
        .name = name,
        .player_registry = player_registry,
        .next = registry->buckets[bucket_index]
    };

    memcpy(node, &node_value, sizeof(resource_node));

    registry->buckets[bucket_index] = node;
}


char spend_resource(const char* name, int id, int amount) {
    int bucket_index = get_bucket_index(name, _gs->resource_registry->size);

    resource_node* node = _gs->resource_registry->buckets[bucket_index];

    while(node && strcmp(name, node->name) != 0) 
        node = node->next;

    if (!node) return 0;
    
    if (node->player_registry[id] < amount) return 0;

    node->player_registry[id] -= amount;
    return 1;
}

int peek_resource(const char* name, int id) {
    int bucket_index = get_bucket_index(name, _gs->resource_registry->size);

    resource_node* node = _gs->resource_registry->buckets[bucket_index];

    while(node && strcmp(name, node->name) != 0) 
        node = node->next;

    if (!node) return 0;

    return node->player_registry[id];
}