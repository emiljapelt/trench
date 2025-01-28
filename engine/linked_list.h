#ifndef LINKED_LIST_H
#define LINKED_LIST_H

typedef struct linked_list_node {
    void* data;
    struct linked_list_node* next;
}
linked_list_node;

typedef struct linked_list {
    linked_list_node* list;
} linked_list;

void list_add(linked_list* list, void* data);
void* list_get(linked_list* list, int index);
void list_rev(linked_list* list);

#endif