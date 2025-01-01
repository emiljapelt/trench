#ifndef FIELDS_H
#define FIELDS_H

typedef enum {
    EMPTY,
    TRENCH,
} field_type;

typedef struct {
    unsigned int fortified : 1;
} trench_field;

typedef union {
    trench_field trench;
} field_data;

#endif