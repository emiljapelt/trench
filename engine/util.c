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
