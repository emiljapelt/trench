#include "resource_registry.h"

#include "game_state.h"
#include "game_rules.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>


resource_registry default_resource_registry;


void set_resource_entry(resource_registry* registry, resource_type resource, int amount, int max) {
    registry->resource[resource].amount = amount;
    registry->resource[resource].max = max;
}

void copy_resource_registry(const resource_registry const* old_registry, resource_registry* new_registry) {
    for(int r = 0; r < RESOURCE_COUNT; r++)
        set_resource_entry(new_registry, r, old_registry->resource[r].amount, old_registry->resource[r].max);
}

void copy_empty_resource_registry(const resource_registry const * old_registry, resource_registry* new_registry) {
    for(int r = 0; r < RESOURCE_COUNT; r++)
        set_resource_entry(new_registry, r, 0, old_registry->resource[r].max);
}

void zero_out_registry(resource_registry* registry) {
    for(int r = 0; r < RESOURCE_COUNT; r++) 
        set_resource_entry(registry, r, 0, 0);
}


char spend_resource(resource_registry* registry, resource_type r, int amount) {
    if (r < 0 || r >= RESOURCE_COUNT) return 0;

    if (registry->resource[r].amount < amount) 
        return 0;

    registry->resource[r].amount -= amount;
    return 1;
}

int peek_resource(resource_registry* registry, resource_type r) {
    if (r < 0 || r >= RESOURCE_COUNT) return 0;
    
    return registry->resource[r].amount;
}

void add_resource(resource_registry* registry, resource_type r, unsigned int amount) {
    if (r < 0 || r >= RESOURCE_COUNT) return;

    if (registry->resource[r].max == -1 || registry->resource[r].amount + amount <= registry->resource[r].max) 
        registry->resource[r].amount += amount;
    else
        registry->resource[r].amount = registry->resource[r].max;
}

char remaining_resource_space(resource_registry* registry, resource_type r) {
    int max = registry->resource[r].max;
    if (max == -1) return -1;
    int amount = registry->resource[r].amount;
    return max - amount;
}