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
    int program_size_limit;
    int debug: 1;

    struct {
        int x;
        int y;
        int width;
        int height;
    } viewport;

    struct settings {
        struct {
            int range;
            int cost;
        } fireball;

        struct {
            int range;
        } shoot;

        struct {
            int range;
        } bomb;
        
        struct {
            int amount;
        } meditate;

        struct {
            int cost;
        } dispel;

        struct {
            int cost;
        } mana_drain;

        struct {
            int cost;
        } wall;

        struct {
            int delay;
        } plant_tree;

        struct {
            int cost;
        } bridge;

        struct {
            int wood_gain;
            int sapling_chance;
        } chop;

        struct {
            int cost;
        } fortify;

        struct {
            int cost;
            int upkeep;
        } projection;

        struct {
            int cost;
            int duration;
            int range;
            int refreeze;
        } freeze;

        struct {
            int range;
        } look;

        struct {
            int range;
        } scan;

        struct {
            int capacity;
            int cost;
        } boat;

        struct {
            int cost;
            int range;
        } throw_clay;

        struct {
            int spread_limit;
            int contain_limit;
            int collect_max;
        } clay_pit;

        struct {
            int cost;
        } clay_golem;

        struct {
            int cost;
        } mine_shaft;

        struct {
            int ammo_per_metal;
            int beartraps_per_metal;
        } craft;
    } settings;
} game_rules;

#endif