#include "util.h"

#include <stdlib.h>
#include <string.h>

int numeric_size(char* str, int start) {
    int i = 0;
    if (str[start] == '-') i++;
    while(str[start + i] >= 48 && str[start + i] <= 57) i++;
    return i;
}

int sub_str_to_int(char* str, int start, int size) {
    char* buf = malloc(size);
    memcpy(buf, str+start, size);
    return atoi(buf);
}