#include "gc.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// TODO : have a preprocessor define to make no_gc work when the gc is used
/*
something like : 

void gc_init(){
#ifndef NO_STD
    GC_INIT();
#else
    panicx("using the gc with no_gc mode set in the build.toml", __FILE__, __FUNCTION__)
#endif
}

*/

void gc_init(){
    GC_INIT();
}

void* gc_malloc(size_t size){
    return GC_malloc(size);
}

void* gc_realloc(void* ptr, size_t size){
    return GC_realloc(ptr, size);
}

void gc_free(void* ptr){
    GC_free(ptr);
}

void set_pointer_int(int* ptr, int value){
    *ptr = value;
    printf("*ptr : %d\n", *ptr);
}

size_t size_of_int(){
    return sizeof(int);
}

char* gc_strdup(char* s){
    char* s2 = gc_malloc(strlen(s)+1);
    if (s2){
    strcpy(s2, s);
    }
    return s2;
}
