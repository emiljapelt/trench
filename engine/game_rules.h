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
} game_rules;

#endif