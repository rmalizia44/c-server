#ifndef COMMON_H
#define COMMON_H

#include <uv.h> // uv_err_name uv_strerror
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

#define ERROR_SHOW(expr) {\
    const int _ec_show = (expr);\
    printf("[%s:%u] %s\n\t%s\n",\
        __FILE__,\
        __LINE__,\
        uv_err_name(_ec_show),\
        uv_strerror(_ec_show)\
    );\
}

#define ERROR_CHECK(expr) {\
    const int _ec_check = (expr);\
    if(_ec_check != 0) {\
        ERROR_SHOW(_ec_check);\
        exit(_ec_check);\
    }\
}

static inline void* heap_new(unsigned size) {
    void* ptr = malloc(size);
    if(ptr == NULL) {
        ERROR_CHECK(UV_ENOMEM);
    }
    return ptr;
}
static inline void heap_del(void* ptr) {
    free(ptr);
}

#endif