#include "gc.h"
#include <stdio.h>

void gc_init(){
    GC_INIT();
}

void* gc_malloc(size_t size){
    return GC_malloc(size);
}

void set_pointer_int(int* ptr, int value){
    *ptr = value;
    printf("*ptr : %d\n", *ptr);
}

size_t size_of_int(){
    return sizeof(int);
}