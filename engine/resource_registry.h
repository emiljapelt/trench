#ifndef H_RESOURCE_REGISTRY
#define H_RESOURCE_REGISTRY

typedef struct resource_node
{
    const char* name;
    int* player_registry;
    struct resource_node* next;
}
resource_node;


typedef struct resource_registry
{
    const int size;
    resource_node** buckets;
} 
resource_registry;


resource_registry* create_resource_registry(int player_count, int size);
void init_resource(resource_registry* registry, const char* name, int init_amount, int player_count);
char spend_resource(const char* name, int id, int amount);
int peek_resource(const char* name, int id);

#endif