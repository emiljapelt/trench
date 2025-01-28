#ifndef H_RESOURCE_REGISTRY
#define H_RESOURCE_REGISTRY

typedef struct resource_node
{
    const char* name;
    int amount;
    struct resource_node* next;
}
resource_node;


typedef struct resource_registry
{
    const int size;
    int count;
    const int max_count;
    resource_node** buckets;
    resource_node** index_ref;
} 
resource_registry;


resource_registry* create_resource_registry(int size, int max_count);
void init_resource(resource_registry* registry, const char* name, int init_amount);
char spend_resource(resource_registry* registry, const char* name, int amount);
int peek_resource(resource_registry* registry, const char* name);
int peek_resource_index(resource_registry* registry, const int index);
void add_resource(resource_registry* registry, const char* name, unsigned int amount);

#endif