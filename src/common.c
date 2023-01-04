#include "common.h"
#include <uv.h> // uv_err_name uv_strerror
#include <stdlib.h> // malloc free

void error_print(const char* file, unsigned line, int ec) {
    LOG("%s\n\t%s", uv_err_name(ec), uv_strerror(ec));
}
void error_check(const char* file, unsigned line, int ec) { 
    if(ec != 0) {
        ERROR_SHOW(ec);
        exit(ec);
    }
}

void* heap_new(unsigned size) {
    void* ptr = malloc(size);
    if(ptr == NULL) {
        ERROR_CHECK(UV_ENOMEM);
    }
    return ptr;
}
void heap_del(void* ptr) {
    free(ptr);
}