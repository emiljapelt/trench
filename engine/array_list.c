#include "array_list.h"

#include <stdlib.h>
#include <string.h>

array_list_t* create(int init_size) {
    array_list_t* new_list = malloc(sizeof(array_list_t));
    void** list = malloc(sizeof(void*) * init_size);

    new_list->count = 0;
    new_list->size = init_size;
    new_list->list = list;

    return new_list;
}

void free_array_list(array_list_t* list) {
    free(list->list);
    free(list);
}

void add(array_list_t* list, void* data) {
    if (list->count >= list->size) {
        void** new_list = malloc(sizeof(void*) * list->size * 2);
        memcpy(new_list, list->list, sizeof(void*) * list->count);
        free(list->list);
        list->list = new_list;
        list->size *= 2;
    }

    list->list[list->count++] = data;
};

void* get(array_list_t* list, int index) {
    if (index < 0 || index >= list->count)
        return NULL;

    return list->list[index];
}

void* remove(array_list_t* list, int index, int do_free) {
    if (index < 0 || index >= list->count)
        return NULL;

    if (do_free) free(list->list[index]);

    void* to_return = do_free ? NULL : list->list[index];

    memmove(list->list + index, list->list + index + 1, sizeof(void*) * (list->count - index));

    list->count--;

    if (list->size > 1 && list->count < (list->size/4)) {
        void** new_list = malloc(sizeof(void*) * (list->size / 2));
        memcpy(new_list, list->list, sizeof(void*) * list->count);
        free(list->list);
        list->list = new_list;
        list->size /= 2;
    }

    return to_return;
}

const array_list_namespace array_list = {
    .create = &create,
    .add = &add,
    .get = &get,
    .remove = &remove,
    .free = &free_array_list,
};