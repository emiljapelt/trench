#ifndef LOG_H
#define LOG_H

typedef enum {
    INFO,
    WARN,
    ERROR,
    DEBUG,
} log_entry_type;

void _log(log_entry_type type, char* format, ...);

#endif
