#include "log.h"

#include "game_state.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>


void _log(log_entry_type type, char* format, ...) {
    FILE* file;
    va_list args;
    
    int min_len = strlen(format) + 10;
    char actual_format[min_len];
    switch (type) {
        case INFO:
            sprintf(actual_format, "INFO: %s\n", format);
            break;
        case WARN:
            sprintf(actual_format, "WARN: %s\n", format);
            break;
        case ERROR:
            sprintf(actual_format, "ERROR: %s\n", format);
            break;
        case DEBUG:
            if (!_gr->debug) return;
            sprintf(actual_format, "DEBUG: %s\n", format);
            break;
        default:
            return;
    }
    
    va_start(args, format);
    file = fopen("./trench.log", "a");

    vfprintf(file, actual_format, args);

    fclose(file);
    va_end(args);
}
