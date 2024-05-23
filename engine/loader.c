#include "loader.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "util.h"
#include "compiler_interface.h"


void skip_whitespace(char* str, int* i) {
    while(str[*i] == ' ' || str[*i] == '\n' || str[*i] == '\t') {
        (*i)++;
    }
}


void load_file(const char* file_name, char** out_file, int* out_len) {
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
    else if(strcmp(key, "shots") == 0) ret = SHOTS;
    else if(strcmp(key, "actions") == 0) ret = ACTIONS;
    else if(strcmp(key, "steps") == 0) ret = STEPS;
    else if(strcmp(key, "mode") == 0) ret = MODE;
    else if(strcmp(key, "player") == 0) ret = PLAYER;
    else if(strcmp(key, "board_x") == 0) ret = BOARD_X;
    else if(strcmp(key, "board_y") == 0) ret = BOARD_Y;
    else if(strcmp(key, "nuke") == 0) ret = NUKE;

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

void free_parsed_game_file(parsed_game_file* pgf) {
    for(int i = 0; i < pgf->player_count; i++) 
        free(pgf->players[i]);
    free(pgf);
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
                if (lgf->bombs) { printf("Multiple entries for 'bombs'"); exit(1); }
                lgf->bombs = get_load_value(content+i, &skips);
                i += skips;
                break;
            }
            case SHOTS: {
                if (lgf->shots) { printf("Multiple entries for 'shoots'"); exit(1); }
                lgf->shots = get_load_value(content+i, &skips);
                i += skips;
                break;
            }
            case ACTIONS: {
                if (lgf->actions) { printf("Multiple entries for 'actions'"); exit(1); }
                lgf->actions = get_load_value(content+i, &skips);
                i += skips;
                break;
            }
            case STEPS: {
                if (lgf->steps) { printf("Multiple entries for 'actions'"); exit(1); }
                lgf->steps = get_load_value(content+i, &skips);
                i += skips;
                break;
            }
            case MODE: {
                if (lgf->mode) { printf("Multiple entries for 'mode'"); exit(1); }
                lgf->mode = get_load_value(content+i, &skips);
                i += skips;
                break;
            }
            case BOARD_X: {
                if (lgf->board_x) { printf("Multiple entries for 'board_x'"); exit(1); }
                lgf->board_x = get_load_value(content+i, &skips);
                i += skips;
                break;
            }
            case BOARD_Y: {
                if (lgf->board_y) { printf("Multiple entries for 'board_y'"); exit(1); }
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
            case NUKE: {
                if (lgf->board_y) { printf("Multiple entries for 'nuke'"); exit(1); }
                lgf->nuke = get_load_value(content+i, &skips);
                i += skips;
            }
        }
        skip_whitespace(content, &i);
    }

    return lgf;
}

int get_program_from_file(const char* file_path, const char* comp_path, char** result_regs, char** result_dir) {
    int fp_len = strlen(file_path);
    char* content;
    switch (file_path[fp_len-1]) {
        case 'c': {
            int content_len;
            load_file(file_path, &content, &content_len);
            if (content == NULL) { printf("Failure: No such file: %s\n", file_path); return 0;}
            break;
        }
        case 'r': {
            if(!compile_file(file_path,comp_path, &content)) return 0;
            break;
        }
        default: {
            printf("Unknown file extension");
            return 0;
        }
    }

    if (result_regs) {
        int i = 0;
        while(content[i] != ':') i++;
        char* content_regs = malloc(i+1);
        memset(content_regs,0,i+1);
        memcpy(content_regs,content,i);
        *result_regs = content_regs;
    }
    if (result_dir) {
        int i = 0;
        while(content[i] != ':') i++;
        int len = strlen(content)-i;
        char* content_dir = malloc(len+1);
        memset(content_dir,0,len+1);
        memcpy(content_dir,content+i+1,len);
        *result_dir = content_dir;
    }
    free(content);

    return 1;
}

parsed_player_file* parse_player(char* value, const char* comp_path) {
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
    while(value[end] != 0) end++;
    char* directive_file = malloc(end+1); directive_file[end] = 0;
    memcpy(directive_file, value, end);

    if(!get_program_from_file(directive_file, comp_path, &ppf->regs, &ppf->directive)) {
        printf("Player definition: '%s' did not load correctly\n", value); exit(1);
    }
    
    return ppf;
}

parsed_game_file* parse_game_file(const char* file_path, const char* comp_path) {

    char* content;
    int content_len;
    load_file(file_path, &content, &content_len);
    if (content == NULL) { printf("Failure: No such file: %s\n", file_path); exit(1);}

    loaded_game_file* lgf = load_game_file(content, content_len);
    free(content);

    parsed_game_file* pgf = malloc(sizeof(parsed_game_file));
    memset(pgf, 0, sizeof(parsed_game_file));

    pgf->bombs = (lgf->bombs == 0) ? 10 : atoi(lgf->bombs);
    pgf->shots = (lgf->shots == 0) ? 100 : atoi(lgf->shots);
    pgf->actions = (lgf->actions == 0) ? 1 : atoi(lgf->actions);
    pgf->steps = (lgf->steps == 0) ? 1000 : atoi(lgf->steps);
    pgf->mode = (lgf->mode == 0) ? 0 : atoi(lgf->mode);
    pgf->board_x = (lgf->board_x == 0) ? 20 : atoi(lgf->board_x);
    pgf->board_y = (lgf->board_y == 0) ? 20 : atoi(lgf->board_y);
    pgf->player_count = lgf->player_count;
    pgf->nuke = (lgf->nuke == 0) ? 0 : atoi(lgf->nuke);

    parsed_player_file** pps = malloc(sizeof(parsed_player_file*) * pgf->player_count);
    string_chain* player_infos = lgf->players;
    for(int i = 0; i < pgf->player_count; i++) {
        pps[i] = parse_player(player_infos->str, comp_path);
        player_infos = player_infos->next;
    }
    pgf->players = pps;

    free_loaded_game_file(lgf);

    return pgf;
}