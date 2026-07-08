#ifndef UTIL_H
#define UTIL_H

extern const char* stack_overflow_msg;
extern const char* frame_break_msg;
extern const char* div_zero_msg;
extern const char* null_call_msg;
extern const char* out_of_bounds_msg;

void set_wait_time_scaler(float*);
void wait(float seconds);
int clamp(int a, int hi, int lo);
int max(int a, int b);

static inline int use_resource(int amount, int* avail) {
    if (amount > *avail) return 0;
    *avail -= amount;
}

/* params: int mod */
#define _TERM_PRINT_MOD "\033[%im\0"
/* params: 3|4 target, int r, int g, int b */
#define _TERM_PRINT_COLOR "\033[%c8;2;%i;%i;%im\0"
#define _TERM_PRINT_RESET "\033[m\0"

void clear_screen(void);
void reset_cursor(void);

void terminal_echo_off();
void terminal_echo_on();

void terminal_blocking_read_off();
void terminal_blocking_read_on();

void terminal_canonical_off();
void terminal_canonical_on();

#endif