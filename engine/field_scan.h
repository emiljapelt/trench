#ifndef FIELD_SCAN_H
#define FIELD_SCAN_H

#include "game_state.h"

typedef struct {
    unsigned int obstruction : 1;
    unsigned int trench : 1;
    unsigned int player : 1;
    unsigned int trapped : 1;
    unsigned int _ : 28;
} field_scan;

field_scan scan_field(const int x, const int y);

#endif