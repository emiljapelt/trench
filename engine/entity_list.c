#include "entity_list.h"

#include <stdlib.h>
#include <stdio.h>

#include "util.h"
#include "entity.h"

void add_entity(entity_list_t* list, entity_t* e) {
    array_list.add(list, e);
}

entity_t* get_entity(entity_list_t* list, int index) {
    return (entity_t*)array_list.get(list, index);
}

entity_t* remove_entity(entity_list_t* list, int id) {
    for(int i = 0; i < list->count; i++) {
        if (entity.get_id(get_entity(list, i)) == id) {
            return array_list.remove(list, i, 0);
        }
    }
    return NULL;
}

entity_t* get_entity_from_id(entity_list_t* list, int id) {
    for(int i = 0; i < list->count; i++) {
        entity_t* e = get_entity(list, i); 
        if (entity.get_id(e) == id) {
            return e;
        }
    }
    return NULL;
}

entity_t* pop_entity(entity_list_t* list) {
    entity_t* result = (entity_t*) array_list.get(list, list->count-1);
    array_list.remove(list, list->count-1, 0);
    return result;
}

entity_t* peek_entity(entity_list_t* list) {
    return (entity_t*) array_list.get(list, list->count-1);
}