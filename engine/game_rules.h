#ifndef GAME_RULES_H
#define GAME_RULES_H

typedef struct game_rules {
    const int actions;
    const int bombs;
    const int dir_change;
    const unsigned char nuke : 1;
} game_rules;

#endif