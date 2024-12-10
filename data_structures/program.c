#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "resource_map.h"
#include "event_list.h"

typedef struct set_char_args
{
    int index;
    char c;
} set_char_args;
void set_char(char* world, void* data) {
    set_char_args* args = (set_char_args*)data;
    world[args->index] = args->c;
}

typedef struct spread_char_args
{
    int index;
} spread_char_args;
void spread_char(char* world, void* data) {
    spread_char_args* args = (spread_char_args*)data;
    char c = world[args->index];
    if (args->index - 1 > 0) world[args->index -1] = c;
    if (args->index + 1 < 10) world[args->index +1] = c;
}

int main() {

    event_list events = { .list = NULL };
    char world[10 + 1];
    memset(world, '.', 10); world[10] = 0;

    {
        set_char_args* args = malloc(sizeof(set_char_args));
        args->c = '!';
        args->index = 4;
        add_event(&events, 3, &set_char, args);
    }

    {
        spread_char_args* args = malloc(sizeof(spread_char_args));
        args->index = 4;
        add_event(&events, 6, &spread_char, args);
    }

    {
        spread_char_args* args = malloc(sizeof(spread_char_args));
        args->index = 5;
        add_event(&events, 8, &spread_char, args);
    }


    for(int i = 0; i < 10; i++) {
        update_events(world, &events);
        printf("%i: %s\n", i, world);
    }

    resource_map* map = create_resource_map(5, 5);
    init_resource(map, "mana", 100);
    init_resource(map, "food", 2);
    printf("mana: %i\nfood: %i\n", 
        peek_resource(map, "mana", 0), 
        peek_resource(map, "food", 0)
    );
    char spend = spend_resource(map, "mana", 0, 4);
    printf("%s\n", spend ? "spend" : "no spend");
    printf("mana: %i\nfood: %i\n", 
        peek_resource(map, "mana", 0), 
        peek_resource(map, "food", 0)
    );

    spend = spend_resource(map, "funk", 2, 2);
    if (!spend) printf("blocked spending\n");

    return 0;
}