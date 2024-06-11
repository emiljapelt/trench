#ifndef UTIL_H
#define UTIL_H

#ifdef _WIN32
#include <windows.h>
#define sleep(x) Sleep(x)
#elif __unix__
#include <unistd.h>
#define sleep(x) usleep(x*1000)
#endif


int numeric_size(const char* str, const int start) ;
int sub_str_to_int(const char* str, const int start, const int size);

#endif