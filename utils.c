#include "utils.h"
#include "libs/isCharContainedInStr.h"

int isFunctionCall(char* str){
    return (isCharContainedInStr('(', str) && isCharContainedInStr(')', str));
}
