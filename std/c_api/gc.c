#include "gc.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

extern void panicx(char* string, const char* filename, const char* function) __attribute__ ((noreturn));;

#define NO_GC_PANIC() panicx("using the gc with no_gc mode set in the build.toml", __FILE__, __FUNCTION__);

void gc_init(){
#ifndef NO_STD
    GC_INIT();
#else
    NO_GC_PANIC();
#endif
}

void* gc_malloc(size_t size){
#ifndef NO_STD
    return GC_malloc(size);
#else
    NO_GC_PANIC();
    return NULL;
#endif
}

void* gc_realloc(void* ptr, size_t size){
#ifndef NO_STD
    return GC_realloc(ptr, size);
#else
    NO_GC_PANIC();
    return NULL;
#endif
}

void gc_free(void* ptr){
#ifndef NO_STD
    GC_free(ptr);
#else
    NO_GC_PANIC();
#endif
}

void set_pointer_int(int* ptr, int value){
    *ptr = value;
    printf("*ptr : %d\n", *ptr);
}

char* gc_strdup(char* s){
    char* s2 = gc_malloc(strlen(s)+1);
    if (s2){
        strcpy(s2, s);
    }
    return s2;
}
