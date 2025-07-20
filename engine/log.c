#include "log.h"

#include "game_state.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>


#define _len 256
char _buf[_len] = {0};
int _index = 0;
char* log_path = "./trench.log";

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
    
    if (_gr && _gr->debug) {
        file = fopen(log_path, "a");
        vfprintf(file, actual_format, args);
        fclose(file);
    } 
    else {
        char temp_buffer[_len];
        vsprintf(temp_buffer, actual_format, args);
        int temp_len = strlen(temp_buffer);
        if (temp_len + _index > _len) {
            file = fopen(log_path, "a");
            fputs(_buf, file);
            fputs(temp_buffer, file);
            _index = 0;
            memset(_buf, 0, _len);
            fclose(file);
        }
        else {
            memcpy(&_buf[_index], temp_buffer, temp_len);
            _index += temp_len;
        }
    }

    va_end(args);
}

void _log_flush() {
    FILE* file;
    if (_index) {
        file = fopen(log_path, "a");
        fputs(_buf, file);
        _index = 0;
        memset(_buf, 0, _len);
        fclose(file);
    }
}