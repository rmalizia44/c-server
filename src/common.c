#include "common.h"
#include <uv.h> // uv_err_name uv_strerror
#include <stdlib.h> // malloc free

void error_print(const char* file, unsigned line, int ec) {
    printf("[%s:%u] ", file, line);
    printf("%s\n\t%s", uv_err_name(ec), uv_strerror(ec));
    printf("\n");
}

void* heap_new(unsigned size) {
    /*if(rand() % 10 == 0) {
        return NULL;
    }*/
    return malloc(size);
}
void heap_del(void* ptr) {
    free(ptr);
}