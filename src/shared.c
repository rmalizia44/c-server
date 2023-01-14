#include "shared.h"

shared_t* shared_new(unsigned size) {
    shared_t* ptr;
    
    void* data = heap_new(size);
    if(data == NULL) {
        return NULL;
    }
    ptr = (shared_t*)heap_new(sizeof(shared_t));
    if(ptr == NULL) {
        heap_del(data);
        return NULL;
    }
    ptr->data = data;
    ptr->size = size;
    ptr->refs = 1;
    
    return ptr;
}
shared_t* shared_copy(shared_t* ptr) {
    ptr->refs += 1;
    return ptr;
}
void shared_del(shared_t* ptr) {
    ptr->refs -= 1;
    if(ptr->refs == 0) {
        heap_del(ptr->data);
        heap_del(ptr);
    }
}