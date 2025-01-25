#ifndef GAME_RULES_H
#define GAME_RULES_H

typedef struct game_rules {
    int actions;
    int steps;
    int mode;
    int nuke;
    int feature_level;
    int exec_mode;
    int seed;
    float time_scale;
} game_rules;

#endif