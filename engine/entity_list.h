#ifndef ENTITY_LIST_H
#define ENTITY_LIST_H

#include "entity.h"
#include "array_list.h"

typedef array_list_t entity_list_t;


void add_entity(entity_list_t* list, entity_t* ps);
entity_t* get_entity(entity_list_t* players, int index);
void remove_entity(entity_list_t* list, int id);
entity_t* pop_entity(entity_list_t* list);
entity_t* peek_entity(entity_list_t* list);

#endif