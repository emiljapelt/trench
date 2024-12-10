#include "resource_map.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

resource_map* create_resource_map(int player_count, int size) {
    resource_map* map = malloc(sizeof(resource_map));

    resource_node** buckets = malloc(sizeof(resource_node*) * size);
    
    resource_map map_value = {
      .size = size,
      .player_count = player_count,
      .buckets = buckets
    };

    memcpy(map, &map_value, sizeof(resource_map));
    
    return map;
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

void init_resource(resource_map* map, const char* name, int init_amount) {
    resource_node* node = malloc(sizeof(resource_node));

    int* registry = malloc(sizeof(int) * map->player_count);
    for(int i = 0; i < map->player_count; i++)
        registry[i] = init_amount;

    int bucket_index = get_bucket_index(name, map->size);

    printf("Setting %s at %i\n", name, bucket_index);

    resource_node node_value = {
        .name = name,
        .registry = registry,
        .next = map->buckets[bucket_index]
    };

    memcpy(node, &node_value, sizeof(resource_node));

    map->buckets[bucket_index] = node;
}


char spend_resource(resource_map* map, const char* name, int id, int amount) {
    int bucket_index = get_bucket_index(name, map->size);

    resource_node* node = map->buckets[bucket_index];

    while(node && strcmp(name, node->name) != 0) 
        node = node->next;

    if (!node) return 0;
    
    if (node->registry[id] < amount) return 0;

    node->registry[id] -= amount;
    return 1;
}

int peek_resource(resource_map* map, const char* name, int id) {
    int bucket_index = get_bucket_index(name, map->size);

    resource_node* node = map->buckets[bucket_index];

    while(node && strcmp(name, node->name) != 0) 
        node = node->next;

    if (!node) return 0;
    
    printf("Peeking at %s\n", node->name);

    return node->registry[id];
}