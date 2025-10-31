#ifndef H_RESOURCE_REGISTRY
#define H_RESOURCE_REGISTRY

#define RESOURCE_COUNT 6

typedef enum {
    R_Explosive = 0,
    R_Ammo = 1,
    R_Mana = 2,
    R_Sapling = 3,
    R_Clay = 4,
    R_Wood = 5,
} resource_type;

typedef struct resource {
    int amount;
    int max;
} resource;

typedef struct resource_registry {
    resource resource[RESOURCE_COUNT];
} resource_registry;

extern resource_registry default_resource_registry;

void copy_resource_registry(const resource_registry const * old_registry, resource_registry* new_registry);
void copy_empty_resource_registry(const resource_registry const * old_registry, resource_registry* new_registry);
char spend_resource(resource_registry* registry, resource_type r, int amount);
int peek_resource(resource_registry* registry, resource_type r);
void add_resource(resource_registry* registry, resource_type r, unsigned int amount);

#endif