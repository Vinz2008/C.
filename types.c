#include "types.h"
#include <string.h>

int isDigit(char c){
	return (c == '1' || c == '2' || c == '3' || c == '4' || c == '5' || c == '6' || c == '7' || c == '8' || c == '9');

}

int isNumber(char* str){
	int counter = 0;
	for (int i = 0; i < strlen(str); i++){
	if (isDigit(str[i]))
		counter++;
	}
	if (counter > strlen(str)) {
		return 1;
	}
	return 0;
}


