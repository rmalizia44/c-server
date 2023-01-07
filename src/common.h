#ifndef COMMON_H
#define COMMON_H

#include <stdio.h> // printf
#include <stdlib.h> // exit

#define LOG(...) {\
    printf("[%s:%u] ",\
        __FILE__,\
        __LINE__\
    );\
    printf(__VA_ARGS__);\
    printf("\n");\
}

#define ERROR_SHOW(expr) { error_print(__FILE__, __LINE__, expr); }

void error_print(const char* file, unsigned line, int ec);

void* heap_new(unsigned size);
void heap_del(void* ptr);

#endif