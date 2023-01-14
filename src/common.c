#include "common.h"
#include <uv.h> // uv_err_name uv_strerror
#include <stdlib.h> // malloc free

#ifdef DEBUG
#include <stdatomic.h>
    atomic_int mem_counter = ATOMIC_VAR_INIT(0);
#endif

void error_print(const char* file, unsigned line, int ec) {
    printf("[%s:%u] ", file, line);
    printf("%s\n\t%s", uv_err_name(ec), uv_strerror(ec));
    printf("\n");
}

void* heap_new(unsigned size) {
    void* ptr = malloc(size);
#ifdef DEBUG
    if(ptr != NULL) {
        atomic_fetch_add(&mem_counter, 1);
        LOG("HEAP +1 (%i)", atomic_load(&mem_counter));
    } else {
        LOG("HEAP out of memory");
    }
#endif
    /*if(rand() % 10 == 0) {
        return NULL;
    }*/
    return ptr;
}
void heap_del(void* ptr) {
#ifdef DEBUG
    atomic_fetch_sub(&mem_counter, 1);
    LOG("HEAP -1 (%i)", atomic_load(&mem_counter));
#endif
    free(ptr);
}