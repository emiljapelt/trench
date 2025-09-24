#ifndef UTIL_H
#define UTIL_H

#include "game_rules.h"

void wait(float seconds);
int numeric_size(const char* str, const int start) ;
int sub_str_to_int(const char* str, const int start, const int size);

static inline int use_resource(int amount, int* avail) {
    if (amount > *avail) return 0;
    *avail -= amount;
}

void terminal_echo_off();
void terminal_echo_on();

void terminal_blocking_read_off();
void terminal_blocking_read_on();

void terminal_canonical_off();
void terminal_canonical_on();

#endif