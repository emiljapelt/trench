#include "linked_list.h"
#include <string.h>
#include <stdlib.h>

void list_add(linked_list* list, void* data) {
    linked_list_node* node = malloc(sizeof(linked_list_node));
    node->data = data;
    node->next = list->list;
    list->list = node;
}

void list_rev(linked_list* list) {
    linked_list reversed = { .list = NULL};
    linked_list_node* node = list->list;
    while (node) {
        linked_list_node* temp = node;
        node = node->next;
        temp->next = reversed.list;
        reversed.list = temp;
    }

    list->list = reversed.list;
}