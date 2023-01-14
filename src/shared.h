#ifndef SHARED_H
#define SHARED_H

#include "common.h"

// not thread safe

typedef struct shared_s {
    void* data;
    unsigned size;
    unsigned refs;
} shared_t;

shared_t* shared_new(unsigned size);
shared_t* shared_copy(shared_t* ptr);
void shared_del(shared_t* ptr);

#endif