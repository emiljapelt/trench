#ifndef LOADER_H
#define LOADER_H


typedef enum load_key {
    BOMBS,
    ACTIONS,
    CHANGE,
    PLAYER,
    BOARD_X,
    BOARD_Y,
} load_key;

typedef struct string_chain {
    int size;
    char* str;
    struct string_chain* next;
} string_chain;

typedef struct loaded_game_file {
    char* bombs;
    char* actions;
    char* change;
    char* board_x; 
    char* board_y;
    int player_count;
    string_chain* players;
} loaded_game_file;


typedef struct parsed_player_file {
    int id;
    int x, y;
    char* reg_directive;
} parsed_player_file;

typedef struct parsed_game_file {
    int bombs;
    int actions;
    int change;
    int board_x, board_y;
    int player_count;
    parsed_player_file** players;
} parsed_game_file;

char* get_program_from_file(char* file_path, char* comp_path);
parsed_game_file* parse_game_file(char* file_path, char* comp_path);

#endif