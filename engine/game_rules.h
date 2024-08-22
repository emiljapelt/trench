#ifndef GAME_RULES_H
#define GAME_RULES_H

typedef struct game_rules {
    int actions;
    int steps;
    int bombs;
    int shots;
    int mode;
    int nuke;
    int array;
    int feature_level;
} game_rules;

#endif