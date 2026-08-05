#include "util.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>

#include "game_state.h"

const char* stack_overflow_msg = "STACK_OVERFLOW";
const char* frame_break_msg = "FRAME_BREAK";
const char* div_zero_msg = "DIV_BY_ZERO";
const char* null_call_msg = "NULL_CALL";
const char* out_of_bounds_msg = "OUT_OF_BOUNDS";

int print_utf8(unsigned int cp, unsigned char out[4]) {
    if (cp <= 0x7F) {
        out[0] = cp;
        return 1;
    }

    if (cp <= 0x7FF) {
        out[0] = 0xC0 | (cp >> 6);
        out[1] = 0x80 | (cp & 0x3F);
        return 2;
    }

    if (cp >= 0xD800 && cp <= 0xDFFF)
        return 0;

    if (cp <= 0xFFFF) {
        out[0] = 0xE0 | (cp >> 12);
        out[1] = 0x80 | ((cp >> 6) & 0x3F);
        out[2] = 0x80 | (cp & 0x3F);
        return 3;
    }

    if (cp <= 0x10FFFF) {
        out[0] = 0xF0 | (cp >> 18);
        out[1] = 0x80 | ((cp >> 12) & 0x3F);
        out[2] = 0x80 | ((cp >> 6) & 0x3F);
        out[3] = 0x80 | (cp & 0x3F);
        return 4;
    }

    return 0;
}

int max(int a, int b) {
    if (a > b) return a;
    return b;
}

int clamp(int a, int hi, int lo) {
    if (a < lo) return lo;
    if (a > hi) return hi;
    return a;
}

float clampf(float a, float hi, float lo) {
    if (a < lo) return lo;
    if (a > hi) return hi;
    return a;
}

float* wait_time_scaler = NULL;

void set_wait_time_scaler(float* s) {
    wait_time_scaler = s;
}

void wait(float seconds) {
    #ifdef _WIN32
    #include <windows.h>
    Sleep((seconds * (*wait_time_scaler)) * 1000);
    #elif __unix__
    #include <unistd.h>
    usleep((seconds * (*wait_time_scaler)) * 1000000);
    #endif
}

// https://stackoverflow.com/questions/26423537/how-to-position-the-input-text-cursor-in-c
void clear_screen(void) {
    printf("\033[H\033[J");
}

void reset_cursor(void) {
    printf("\033[0;0H");
}

void terminal_echo_off() {
    struct termios config;

    tcgetattr(STDIN_FILENO, &config);

    config.c_lflag &= ~(ECHO);

    tcsetattr(STDIN_FILENO, TCSANOW, &config);
}

void terminal_echo_on() {
    struct termios config;

    tcgetattr(STDIN_FILENO, &config);

    config.c_lflag |= ECHO;

    tcsetattr(STDIN_FILENO, TCSANOW, &config);
}

cc_t prev_VMIN;
cc_t prev_VTIME;
void terminal_blocking_read_off() {
    struct termios config;

    tcgetattr(STDIN_FILENO, &config);

    prev_VMIN = config.c_cc[VMIN];
    prev_VTIME = config.c_cc[VTIME];

    config.c_cc[VMIN] = 0;
    config.c_cc[VTIME] = 0;

    tcsetattr(STDIN_FILENO, TCSANOW, &config);
}

void terminal_blocking_read_on() {
    struct termios config;

    tcgetattr(STDIN_FILENO, &config);

    config.c_cc[VMIN] = prev_VMIN;
    config.c_cc[VTIME] = prev_VTIME;

    tcsetattr(STDIN_FILENO, TCSANOW, &config);
}


void terminal_canonical_off() {
    struct termios config;

    tcgetattr(STDIN_FILENO, &config);

    config.c_lflag &= ~(ICANON);

    tcsetattr(STDIN_FILENO, TCSANOW, &config);
}

void terminal_canonical_on() {
    struct termios config;

    tcgetattr(STDIN_FILENO, &config);

    config.c_lflag |= ICANON;

    tcsetattr(STDIN_FILENO, TCSANOW, &config);
}