#include "compiler_interface.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

const int max_result_size = 10000;

int compile_file(const char* file_path, const char* comp_path, char** result) {

    // Prepare command string
    int fp_len = strlen(file_path);
    int cp_len = strlen(comp_path);
    int command_len = fp_len + cp_len + 4;
    char* command = malloc(command_len); command[command_len] = 0;
    memcpy(command, comp_path, cp_len);
    command[cp_len] = ' ';
    command[cp_len+1] = '"';
    memcpy(command+cp_len+1, file_path, fp_len);
    command[command_len-1] = '"';

    // Prepare temporary result buffer
    char* buffer = malloc(max_result_size + 1);
    memset(buffer, 0, max_result_size+1);

    // Run command and load result to buffer
    FILE* fp;
    fp = popen(command, "r");
    if (fp == NULL) {
        printf("Compilation failed\n" );
        return 0;
    }
    fgets(buffer, max_result_size, fp);    
    
    // Copy significant result from buffer
    int result_len = strlen(buffer);
    *result = malloc(result_len + 1); result[result_len] = 0;
    memcpy(*result, buffer, result_len);

    pclose(fp);
    free(command);
    free(buffer);
    return 1;
}