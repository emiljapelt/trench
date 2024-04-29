#include "loader.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "util.h"
#include "compiler_interface.h"


void skip_whitespace(char* str, int* i) {
    while(str[*i] == ' ' || str[*i] == '\n' || str[*i] == '\t') {
        (*i)++;
    }
}


void load_file(char* file_name, char** out_file, int* out_len) {
    FILE* fp;
    char* buffer;
    long numbytes;
    
    fp = fopen(file_name, "r");
    
    if (fp == NULL) { out_file = 0; return; }
    
    fseek(fp, 0L, SEEK_END);
    numbytes = ftell(fp);
    fseek(fp, 0L, SEEK_SET);	
    buffer = (char*)calloc(numbytes, sizeof(char));	
    
    if(buffer == NULL) { out_file = 0; return; }
    
    fread(buffer, sizeof(char), numbytes, fp);
    fclose(fp);

    *out_file = buffer;
    *out_len = numbytes;
}

load_key get_load_key(char* content, int* skips) {
    *skips = 0;
    int i = 0;
    skip_whitespace(content, &i);

    int key_size = 0;
    while(content[i+key_size] != ':') {
        key_size++;
    }

    char* key = malloc(key_size+1); key[key_size] = 0;
    memcpy(key, content+i, key_size);

    *skips = i+key_size+1;

    load_key ret = -1;
    if(strcmp(key, "bombs") == 0) ret = BOMBS;
    else if(strcmp(key, "actions") == 0) ret = ACTIONS;
    else if(strcmp(key, "change") == 0) ret = CHANGE;
    else if(strcmp(key, "player") == 0) ret = PLAYER;
    else if(strcmp(key, "board_x") == 0) ret = BOARD_X;
    else if(strcmp(key, "board_y") == 0) ret = BOARD_Y;

    if (ret == -1) { printf("Unknown load key: %s\n", key); exit(1); }
    free(key);

    return ret;
}

char* get_load_value(char* content, int* skips) {
    *skips = 0;
    int i = 0;
    skip_whitespace(content, &i);

    int val_size = 0;
    while(content[i+val_size] != ';') {
        val_size++;
    }

    char* val = malloc(val_size+1); val[val_size] = 0;
    memcpy(val, content+i, val_size);

    *skips = i+val_size+1;
    return val;
}

void free_loaded_game_file(loaded_game_file* lgf) {
    string_chain* chain_handle = lgf->players;
    while (chain_handle) {
        string_chain* next = chain_handle->next;
        free(chain_handle);
        chain_handle = next;
    }
    free(lgf);
}

loaded_game_file* load_game_file(char* content, const int size) {
    loaded_game_file* lgf = malloc(sizeof(loaded_game_file));
    memset(lgf, 0, sizeof(loaded_game_file));

    int i = 0;
    while(i < size) {
        int skips = 0;
        load_key lk = get_load_key(content+i, &skips);
        i += skips;
        switch (lk) {
            case BOMBS: {
                if (lgf->bombs) { printf("Multiple entries from 'bombs'"); exit(1); }
                lgf->bombs = get_load_value(content+i, &skips);
                i += skips;
                break;
            }
            case ACTIONS: {
                if (lgf->actions) { printf("Multiple entries from 'actions'"); exit(1); }
                lgf->actions = get_load_value(content+i, &skips);
                i += skips;
                break;
            }
            case CHANGE: {
                if (lgf->change) { printf("Multiple entries from 'change'"); exit(1); }
                lgf->change = get_load_value(content+i, &skips);
                i += skips;
                break;
            }
            case BOARD_X: {
                if (lgf->board_x) { printf("Multiple entries from 'board_x'"); exit(1); }
                lgf->board_x = get_load_value(content+i, &skips);
                i += skips;
                break;
            }
            case BOARD_Y: {
                if (lgf->board_y) { printf("Multiple entries from 'board_y'"); exit(1); }
                lgf->board_y = get_load_value(content+i, &skips);
                i += skips;
                break;
            }
            case PLAYER: {
                string_chain* link = malloc(sizeof(string_chain));
                char* val = get_load_value(content+i, &skips);
                link->next = lgf->players;
                link->str = val;
                link->size = skips;
                lgf->player_count++;
                lgf->players = link;
                i += skips;
                break;
            }
        }
        skip_whitespace(content, &i);
    }

    return lgf;
}

char* get_program_from_file(char* file_path, char* comp_path) {
    int fp_len = strlen(file_path);
    char* content;
    switch (file_path[fp_len-1]) {
        case 'c': {
            int content_len;
            load_file(file_path, &content, &content_len);
            if (content == NULL) { printf("Failure: No such file: %s\n", file_path); exit(1);}
            break;
        }
        case 'r': {
            content = compile_file(file_path,comp_path);
            break;
        }
        default: {
            printf("Unknown file extension");
            exit(1);
        }
    }
    return content;
}

parsed_player_file* parse_player(char* value, char* comp_path) {
    parsed_player_file* ppf = malloc(sizeof(parsed_player_file));
    memset(ppf, 0, sizeof(parsed_player_file));

    for(int n = 0; n < 3; n++) {
        int skip = 0;
        while(value[skip] != ',') skip++;
        char* num = malloc(skip+1); num[skip] = 0;
        memcpy(num, value, skip);
        if (n == 0) ppf->id = atoi(num);
        else if (n == 1) ppf->x = atoi(num);
        else if (n == 2) ppf->y = atoi(num);
        value += skip+1;
        free(num);
        if (ppf->id == 0) { printf("Player ids cannot be 0\n"); exit(1); }
    }

    int end = 0;
    while(value[end] != ';') end++;
    char* directive_file = malloc(end+1); directive_file[end] = 0;
    memcpy(directive_file, value, end);

    
    ppf->reg_directive = get_program_from_file(directive_file, comp_path);

    return ppf;
}

parsed_game_file* parse_game_file(char* file_path, char* comp_path) {

    char* content;
    int content_len;
    load_file(file_path, &content, &content_len);
    if (content == NULL) { printf("Failure: No such file: %s\n", file_path); exit(1);}

    loaded_game_file* lgf = load_game_file(content, content_len);
    free(content);

    parsed_game_file* pgf = malloc(sizeof(parsed_game_file));
    memset(pgf, 0, sizeof(parsed_game_file));

    pgf->bombs = (lgf->bombs == 0) ? 10 : atoi(lgf->bombs);
    pgf->actions = (lgf->actions == 0) ? 1 : atoi(lgf->actions);
    pgf->change = (lgf->change == 0) ? 0 : atoi(lgf->change);
    pgf->board_x = (lgf->board_x == 0) ? 20 : atoi(lgf->board_x);
    pgf->board_y = (lgf->board_y == 0) ? 20 : atoi(lgf->board_y);
    pgf->player_count = lgf->player_count;

    parsed_player_file** pps = malloc(sizeof(parsed_player_file*)*pgf->player_count);
    string_chain* player_infos = lgf->players;
    for(int i = 0; i < pgf->player_count; i++) {
        pps[i] = parse_player(player_infos->str, comp_path);
        player_infos = player_infos->next;
    }
    pgf->players = pps;

    free_loaded_game_file(lgf);

    return pgf;
}