#include "lexer.h"
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>


char* identifierString;
double numVal;
int currTok;
int pos = 0;

void append_char(char c, char str[]){
     char arr[2] = {c , '\0'};
     strcat(str , arr);
}



int getCharFromString(char* str){
    if (pos < strlen(str)){
        return str[pos++];
    } else {
        pos = 0;
        return 100000;
    }
}

int getTok(char* str, void* data){
    identifierString = malloc(100 * sizeof(char));
    int lastChar = ' ';
    char* temp;
    while (isspace(lastChar)){
        temp = getCharFromString(str);
        if (temp != 100000){
        lastChar = temp;
        }
    }
    if (isalpha(lastChar)){ // [a-zA-Z][a-zA-Z0-9]
        identifierString = lastChar;
        while (isalnum((lastChar = getchar()))){
            append_char(lastChar, identifierString);

        }
            if (strcmp("function", identifierString) == 0) return tok_function;
            if (strcmp("extern", identifierString) == 0) return tok_extern;
            return tok_identifier;
    }
    if (isdigit(lastChar) || lastChar =='.'){
        char* numString = malloc(15 * sizeof(char));;
        do {
            append_char(lastChar, numString);
            temp = getCharFromString(str);
            if (temp != 100000){
            lastChar = temp;
            }
        } while (isdigit(lastChar) || lastChar == '-'); 
        *numString = strtod(numString, 0);
        return tok_number;
    }
    if (lastChar == '#'){
        do {
            temp = getCharFromString(str);
            if (temp != 100000){
            lastChar = temp;
            }
        } while (lastChar != EOF && lastChar != '\n' && lastChar != '\r');
        if (lastChar != EOF) return getTok(str, data); // recursively find other tokens
    }
    if (lastChar == EOF) return tok_eof;
    int currChar = lastChar;
    temp = getCharFromString(str);
    if (temp != 100000){
    lastChar = temp;
    }
    return currChar;
}

int getNextToken(char* str, void* data){
    return currTok = getTok(str, data);
}

enum Token* lexer(char* str){
	void* data;
	data = malloc(sizeof(*data));
	token_t* temp_array = malloc(100 * sizeof(token_t));
	enum Token temp_tok;
	int pos = 0;
	while ((temp_tok = getTok(str, data)) != tok_eof){
	token_t temp = { temp_tok, data };
	temp_array[pos++] = temp;
	}
	return temp_array;
}
