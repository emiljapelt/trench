#include "util.h"

#include <stdlib.h>
#include <string.h>

#include "game_state.h"

void wait(float seconds) {
    #ifdef _WIN32
    #include <windows.h>
    Sleep((seconds * _gr->time_scale) * 1000);
    #elif __unix__
    #include <unistd.h>
    usleep((seconds * _gr->time_scale) * 1000000);
    #endif
}

int numeric_size(const char* str, const int start) {
    int i = 0;
    if (str[start] == '-') i++;
    while(str[start + i] >= 48 && str[start + i] <= 57) i++;
    return i;
}

int sub_str_to_int(const char* str, const int start, const int size) {
    char* buf = malloc(size+1); buf[size] = 0;
    memcpy(buf, str+start, size);
    return atoi(buf);
}