#ifndef ARRAY_LIST_H
#define ARRAY_LIST_H

typedef struct array_list {
    void** list;
    int size;
    int count;
} array_list;


array_list* create_list(int init_size);
void list_add(array_list* list, void* data);
void* list_get(array_list* list, int index);
void list_remove(array_list* list, int index, int do_free);

#endif