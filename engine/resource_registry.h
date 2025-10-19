#ifndef H_RESOURCE_REGISTRY
#define H_RESOURCE_REGISTRY

typedef struct resource_node
{
    const char* name;
    int amount;
    int max;
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
resource_registry* copy_resource_registry(resource_registry* old_registry);
void init_resource(resource_registry* registry, const char* name, int max_amount);
char spend_resource(resource_registry* registry, const char* name, int amount);
int peek_resource(resource_registry* registry, const char* name);
int peek_resource_index(resource_registry* registry, const int index);
void add_resource(resource_registry* registry, const char* name, unsigned int amount);


resource_registry* get_empty_resource_registy();
void set_empty_resource_registy(resource_registry* reg);

#endif