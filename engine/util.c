#include "util.h"

#include <stdlib.h>
#include <string.h>
#include <termios.h>

#include "game_state.h"

int max(int a, int b) {
    if (a > b) return a;
    return b;
}

int clamp(int a, int hi, int lo) {
    if (a < lo) return lo;
    if (a > hi) return hi;
    return a;
}

void wait(float seconds) {
    #ifdef _WIN32
    #include <windows.h>
    Sleep((seconds * _gr->time_scale) * 1000);
    #elif __unix__
    #include <unistd.h>
    usleep((seconds * _gr->time_scale) * 1000000);
    #endif
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