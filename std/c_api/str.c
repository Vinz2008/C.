#ifndef __WASM__
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "str.h"

struct struct_str_c create_string(char* s){
    struct struct_str_c str;
    str.factor = DEFAULT_FACTOR;
    str.maxlength = strlen(s) * str.factor;
    str.ptr = malloc(str.maxlength * sizeof(char));
    strcpy(str.ptr, s);
    str.length = strlen(s);
    return str;
}

double append_char(struct struct_str_c* str, char c){
    if (str->length >= str->maxlength){
        str->ptr = realloc(str->ptr, (str->length + 1) * sizeof(char));
        str->ptr[str->length++] = c;
    } else {
        str->ptr[str->length++] = c;
    }
    return 0;
}


double print_str(struct struct_str_c s){
    puts(s.ptr);
    return s.length;
}
#endif