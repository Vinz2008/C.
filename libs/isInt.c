#include <string.h>
#include <ctype.h>

int isInt(char* str){
    int i;
    int counter = 0;
    for (i = 0; i < strlen(str); i++){
        if (isdigit(str[i])){
            counter++;
        } 
    }
    if (counter >= strlen(str)){ return 1; }
    return 0;
}