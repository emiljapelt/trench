#include "resource_registry.h"

#include "game_state.h"
#include "game_rules.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>


resource_registry* create_resource_registry(int size, int max_count) {
    resource_registry* registry = malloc(sizeof(resource_registry));

    resource_node** buckets = malloc(sizeof(resource_node*) * size);
    resource_node** index_ref = malloc(sizeof(resource_node*) * max_count);
    
    resource_registry registry_value = {
      .size = size,
      .max_count = max_count,
      .buckets = buckets,
      .count = 0,
      .index_ref = index_ref,
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

void init_resource(resource_registry* registry, const char* name, int max_amount) {
    if (registry->count >= registry->max_count) return;

    resource_node* node = malloc(sizeof(resource_node));
    int bucket_index = get_bucket_index(name, registry->size);

    resource_node node_value = {
        .name = name,
        .amount = 0,
        .max = max_amount,
        .next = registry->buckets[bucket_index]
    };

    memcpy(node, &node_value, sizeof(resource_node));

    registry->index_ref[registry->count++] = node;
    registry->buckets[bucket_index] = node;
}

resource_registry* copy_resource_registry(resource_registry* old_registry) {
    resource_registry* new_registry = malloc(sizeof(resource_registry));

    resource_node** buckets = malloc(sizeof(resource_node*) * old_registry->size);
    resource_node** index_ref = malloc(sizeof(resource_node*) * old_registry->max_count);

    resource_registry new_registry_value = {
      .size = old_registry->size,
      .max_count = old_registry->max_count,
      .buckets = buckets,
      .count = 0,
      .index_ref = index_ref,
    };

    memcpy(new_registry, &new_registry_value, sizeof(resource_registry));

    for(int i = 0; i < old_registry->count; i++) {
        init_resource(new_registry, old_registry->index_ref[i]->name, old_registry->index_ref[i]->max);
        add_resource(new_registry, old_registry->index_ref[i]->name, old_registry->index_ref[i]->amount);
    }

    return new_registry;
}


char spend_resource(resource_registry* registry, const char* name, int amount) {
    int bucket_index = get_bucket_index(name, registry->size);

    resource_node* node = registry->buckets[bucket_index];

    while(node && strcmp(name, node->name) != 0) 
        node = node->next;

    if (!node) return 0;
    
    if (node->amount < amount) return 0;

    node->amount -= amount;
    return 1;
}

char spend_resource_index(resource_registry* registry, const int index, const int amount) {
    if (index >= registry->count || index < 0) return 0;

    resource_node* node = registry->index_ref[index];
    
    if (node->amount < amount) return 0;

    node->amount -= amount;
    return 1;
}

int peek_resource(resource_registry* registry, const char* name) {
    int bucket_index = get_bucket_index(name, registry->size);

    resource_node* node = registry->buckets[bucket_index];

    while(node && strcmp(name, node->name) != 0) 
        node = node->next;

    if (!node) return 0;

    return node->amount;
}

int peek_resource_index(resource_registry* registry, const int index) {
    if (index >= registry->count || index < 0) return 0;

    resource_node* node = registry->index_ref[index];

    if (!node) return 0;

    return node->amount;
}

void add_resource(resource_registry* registry, const char* name, unsigned int amount) {
    int bucket_index = get_bucket_index(name, registry->size);

    resource_node* node = registry->buckets[bucket_index];

    while(node && strcmp(name, node->name) != 0) 
        node = node->next;

    if (!node) return;
    
    if (node->max == -1 || node->amount + amount <= node->max)
        node->amount += amount;
    else
        node->amount = node->max;
}



resource_registry* empty_resource_registy;

resource_registry* get_empty_resource_registy() {
    return copy_resource_registry(empty_resource_registy);
}

void set_empty_resource_registy(resource_registry* reg) {
    empty_resource_registy = reg;
}