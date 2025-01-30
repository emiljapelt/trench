#ifndef GAME_RULES_H
#define GAME_RULES_H

typedef struct game_rules {
    int actions;
    int steps;
    int mode;
    int nuke;
    int exec_mode;
    int seed;
    float time_scale;
    int stack_size;
} game_rules;

#endif