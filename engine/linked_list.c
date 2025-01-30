#include "linked_list.h"
#include <string.h>
#include <stdlib.h>

void list_add(linked_list_node** list, void* data) {
    linked_list_node* node = malloc(sizeof(linked_list_node));
    node->data = data;
    node->next = *list;
    *list = node;
}

void list_rev(linked_list_node** list) {
    linked_list reversed = { .list = NULL};
    while (*list) {
        linked_list_node* temp = *list;
        *list = (*list)->next;
        temp->next = reversed.list;
        reversed.list = temp;
    }

    *list = reversed.list;
}