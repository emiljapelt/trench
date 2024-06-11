#ifndef GAME_RULES_H
#define GAME_RULES_H

typedef struct game_rules {
    const int actions;
    const int steps;
    const int bombs;
    const int shots;
    const int mode;
    const int nuke;
    const int array;
} game_rules;

#endif