#include "gc_capi.h"
#include <string.h>

char* internal_get_extension(char* path){
    int pos = -1;
    for (int i = strlen(path)-1; i > 0; i--){
        if (path[i] == '.'){
            pos = i;
            break;
        }
    }
    return gc_strdup(path + pos);    
}