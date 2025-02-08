#include "array_list.h"

#include <stdlib.h>
#include <string.h>

array_list* create_list(int init_size) {
    array_list* new_list = malloc(sizeof(array_list));
    void** list = malloc(sizeof(void*) * init_size);

    new_list->count = 0;
    new_list->size = init_size;
    new_list->list = list;

    return new_list;
}

void list_add(array_list* list, void* data) {
    if (list->count >= list->size) {
        void** new_list = malloc(list->size * 2);
        memcpy(new_list, list->list, sizeof(void*) * list->count);
        free(list->list);
        list->list = new_list;
        list->size *= 2;
    }

    list->list[list->count++] = data;
};

void* list_get(array_list* list, int index) {
    if (index < 0 || index > list->count)
        return NULL;

    return list->list[index];
}

void list_remove(array_list* list, int index, int do_free) {
    if (index < 0 || index > list->count)
        return;

    if (do_free) free(list->list[index]);

    memmove(list->list[index], list->list[index] + 1, sizeof(void*) * (list->count - index));

    list->count--;

    int quad_list_size = list->size/4;
    if (list->count < quad_list_size ) {
        void** new_list = malloc(sizeof(void*) * quad_list_size);
        memcpy(new_list, list->list, sizeof(void*) * list->count);
        free(list->list);
        list->list = new_list;
        list->size = quad_list_size;
    }
}