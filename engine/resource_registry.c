#include "resource_registry.h"

#include "game_state.h"
#include "game_rules.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>


resource_registry default_resource_registry;

void copy_resource_registry(const resource_registry const* old_registry, resource_registry* new_registry) {
    for(int r = 0; r < RESOURCE_COUNT; r++) {
        new_registry->resource[r].amount = old_registry->resource[r].amount;
        new_registry->resource[r].max = old_registry->resource[r].max;
    }
}

void copy_empty_resource_registry(const resource_registry const * old_registry, resource_registry* new_registry) {
    for(int r = 0; r < RESOURCE_COUNT; r++) {
        new_registry->resource[r].amount = 0;
        new_registry->resource[r].max = old_registry->resource[r].max;
    }
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
