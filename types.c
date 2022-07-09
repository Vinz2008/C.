#include "types.h"
#include <string.h>
#include <stdio.h>

int isDigit(char c){
	return (c == '1' || c == '2' || c == '3' || c == '4' || c == '5' || c == '6' || c == '7' || c == '8' || c == '9' || c == '0');

}

int isNumber(char* str){
	int counter = 0;
	for (int i = 0; i < strlen(str); i++){
	printf("str[i] : %c\n", str[i]);
	printf("1 == 1 : %i\n",  (1 == 1));
	printf("isDigit(str[i]) : %d\n", isDigit(str[i]));
	if (isDigit(str[i]) == 1){
		counter++;
	}
	}
	printf("counter %d\n", counter);
	if (counter >= strlen(str)) {
		return 1;
	}
	return 0;
}

int isTypeWord(char* str){ 
	return (strcmp(str, "int") == 0 || strcmp(str, "char") == 0);
}