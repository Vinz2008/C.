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

void initTokenArray(tokenArray_t* TokenArray, size_t initalSize){
    TokenArray->arr = malloc(initalSize * sizeof(token_t));
    TokenArray->used = 0;
    TokenArray->size = initalSize;
}

void addTokenToTokenArray(tokenArray_t* TokenArray, token_t token){
    if (TokenArray->used == TokenArray->size){
        TokenArray->size *=2;
        TokenArray->arr = realloc(TokenArray->arr, TokenArray->size * sizeof(token_t));
    }
    TokenArray->arr[TokenArray->used++] = token;
}

void emptyTokenArray(tokenArray_t* TokenArray){
    free(TokenArray->arr);
    TokenArray->arr = NULL;
    TokenArray->used = TokenArray->size = 0;
}

int getCharFromString(char* str){
    if (pos < strlen(str)){
        return str[pos++];
    } else {
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
        } else {
            return tok_next_line;
        }
    }
    if (isalpha(lastChar)){ // [a-zA-Z][a-zA-Z0-9]
        append_char(lastChar, identifierString);
        while (isalnum((lastChar = getCharFromString(str)))){
            append_char(lastChar, identifierString);
        }
        data = identifierString;
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
            } else {
            return tok_next_line;
            } 
        } while (isdigit(lastChar) || lastChar == '-'); 
        *numString = strtod(numString, 0);
        int* tempInt = malloc(sizeof(int));
        *(tempInt) = atoi(numString);
        data = tempInt;
        return tok_number;
    }
    if (lastChar == '#'){
        do {
            temp = getCharFromString(str);
            if (temp != 100000){
            lastChar = temp;
            } else {
            return tok_next_line;
            }
        } while (lastChar != EOF && lastChar != '\n' && lastChar != '\r');
        if (lastChar != EOF) return getTok(str, data); // recursively find other tokens
    }
    if (lastChar == EOF) return tok_eof;
    if (lastChar == '\n') return tok_next_line;
    if (lastChar == '+') return tok_plus;
    int currChar = lastChar;
    temp = getCharFromString(str);
    if (temp != 100000){
    lastChar = temp;
    } else {
        return tok_next_line;
    }
    return currChar;
}

int getNextToken(char* str, void* data){
    return currTok = getTok(str, data);
}

tokenArray_t lexer(char* str){
	void* data = NULL;
	tokenArray_t temp_array ;
    initTokenArray(&temp_array, 1);
	enum Token temp_tok;
	int pos = 0;
    temp_tok = getTok(str, data);
	while (temp_tok != tok_next_line){
	token_t temp;
    temp.data =  data;
    temp.type = temp_tok;
    addTokenToTokenArray(&temp_array, temp);
    printf("token added %d\n", temp.type);
    printf("token inarr %d\n", temp.type);
    printf("temp_tok : %d\n", temp_array.arr[temp_array.used].type);
    printf("TEST\n");
    pos++;
    data = malloc(sizeof(*data));
    temp_tok = getTok(str, data);
	}
    printf("TEST2\n");
	return temp_array;
}
