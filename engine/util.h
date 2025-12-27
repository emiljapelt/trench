#ifndef UTIL_H
#define UTIL_H

#include "game_rules.h"

extern const char* stack_overflow_msg;
extern const char* frame_break_msg;
extern const char* div_zero_msg;
extern const char* null_call_msg;
extern const char* out_of_bounds_msg;


void wait(float seconds);
int numeric_size(const char* str, const int start) ;
int sub_str_to_int(const char* str, const int start, const int size);
int clamp(int a, int hi, int lo);
int max(int a, int b);

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