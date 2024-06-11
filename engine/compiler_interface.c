#include "compiler_interface.h"
#include "util.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

const int max_result_size = 1000000;

int compile_file(const char* file_path, const char* comp_path, char** result) {

    // Prepare command string
    int fp_len = strlen(file_path);
    int cp_len = strlen(comp_path);
    int command_len = fp_len + cp_len + 4;
    char* command = malloc(command_len); 
    memset(command, 0, command_len); 
    memcpy(command, comp_path, cp_len);
    command[cp_len] = ' ';
    command[cp_len+1] = '"';
    memcpy(command+cp_len+1, file_path, fp_len);
    command[command_len-1] = '"';

    // Run command and load result to buffer
    FILE* fp;
    fp = popen(command, "r");
    if (fp == NULL) {
        printf("Compilation failed: Unknown error\n");
        return 0;
    }

    int len = fgetc(fp) | fgetc(fp) << 8 | fgetc(fp) << 16 | fgetc(fp) << 24; 
    fgetc(fp);

    // Prepare result buffer
    char* buffer = malloc(len + 1);
    memset(buffer, 0, len+1);

    for(int i = 0; i < len; i++) {
        buffer[i] = (char)fgetc(fp);
        //putchar(buffer[i]);
    }

    if(pclose(fp) != 0) {
        printf("Compilation failed: %s\n", buffer);
        return 0;
    }

    int result_len = strlen(buffer);
    *result = buffer;

    free(command);
    return result_len;
}