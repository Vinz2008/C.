#include "isCharContainedInStr.h"
#include <string.h>

int isCharContainedInStr(char c, char* str){
	for (int i = 0; i < strlen(str); i++){
	if (str[i] == c){
	return 1;
	}
	}
	return 0;
}
