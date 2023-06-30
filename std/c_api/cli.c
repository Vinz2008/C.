#include <stdio.h>
#include "gc_capi.h"

char* internal_readline(){
    size_t size = 100;
    size_t used = 0;
    char* buf = gc_malloc(sizeof(char) * size);
    char c = '\0';
    while (c != '\n'){
        c = getchar();
        if (used >= size){
            size += 10;
            buf = gc_realloc(buf, sizeof(char) * size);
        }
        buf[used] = c;
        used++;
    }
    return buf;
}