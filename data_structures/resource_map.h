#ifndef H_RESOURCE_MAP
#define H_RESOURCE_MAP

typedef struct resource_node
{
    const char* name;
    int* registry;
    struct resource_node* next;
}
resource_node;


typedef struct resource_map
{
    const int size;
    const int player_count;
    resource_node** buckets;
} 
resource_map;


resource_map* create_resource_map(int player_count, int size);
void init_resource(resource_map* map, const char* name, int init_amount);
char spend_resource(resource_map* map, const char* name, int id, int amount);
int peek_resource(resource_map* map, const char* name, int id);

#endif