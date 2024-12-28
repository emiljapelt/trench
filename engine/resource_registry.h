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
    int count;
    const int max_count;
    resource_node** buckets;
    resource_node** index_ref;
} 
resource_registry;


resource_registry* create_resource_registry(int player_count, int size, int max_count);
void init_resource(resource_registry* registry, const char* name, int init_amount, int player_count);
char spend_resource(const char* name, int id, int amount);
int peek_resource(const char* name, int id);
int peek_resource_index(const int index, int id);
void add_resource(const char* name, int id, unsigned int amount);

#endif