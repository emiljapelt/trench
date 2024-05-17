#ifndef LOADER_H
#define LOADER_H


typedef enum load_key {
    BOMBS,
    SHOTS,
    ACTIONS,
    STEPS,
    MODE,
    PLAYER,
    BOARD_X,
    BOARD_Y,
    NUKE,
} load_key;

typedef struct string_chain {
    int size;
    char* str;
    struct string_chain* next;
} string_chain;

typedef struct loaded_game_file {
    char* bombs;
    char* shots;
    char* actions;
    char* steps;
    char* mode;
    char* board_x; 
    char* board_y;
    int player_count;
    string_chain* players;
    char* nuke;
} loaded_game_file;


typedef struct parsed_player_file {
    int id;
    int x, y;
    char* reg_directive;
} parsed_player_file;

typedef struct parsed_game_file {
    int bombs;
    int shots;
    int actions;
    int steps;
    int mode;
    int board_x, board_y;
    int player_count;
    int nuke;
    parsed_player_file** players;
} parsed_game_file;

int get_program_from_file(const char* file_path, const char* comp_path, char** result);
parsed_game_file* parse_game_file(const char* file_path, const char* comp_path);
void free_parsed_game_file(parsed_game_file* pgf);

#endif