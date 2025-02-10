#ifndef ARRAY_LIST_H
#define ARRAY_LIST_H

typedef struct array_list_t {
    void** list;
    int size;
    int count;
} array_list_t;

typedef struct array_list_namespace {
    array_list_t* (*const create)(const int init_size);
    void (*const add)(array_list_t* list, void* data);
    void* (*const get)(array_list_t* list, int index);
    void (*const remove)(array_list_t* list, int index, int do_free);
} array_list_namespace;

extern const array_list_namespace array_list;

#endif