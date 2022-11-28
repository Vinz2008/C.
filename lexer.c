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
    strncat(str , &c, 1);
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

char getCharFromString(char* str){
    if (pos < strlen(str)){
        return str[pos++];
    } else {
        return '\0';
    }
}

int getTok(struct TokReturn* tokreturn){
    identifierString = malloc(100 * sizeof(char));
    int lastChar = ' ';
    char temp;
    while (isspace(lastChar)){
        temp = getCharFromString(tokreturn->str);
        if (temp != '\0'){
        lastChar = temp;
        } else {
            return tok_next_line;
        }
    }
    if (isalpha(lastChar)){ // [a-zA-Z][a-zA-Z0-9]
        append_char(lastChar, identifierString);
        while (isalnum((lastChar = getCharFromString(tokreturn->str)))){
            append_char(lastChar, identifierString);
        }
        tokreturn->data = identifierString;
        if (strcmp("function", identifierString) == 0) return tok_function;
        if (strcmp("extern", identifierString) == 0) return tok_extern;
        if (strcmp("import", identifierString) == 0) return tok_import;
        return tok_identifier;
    }
    if (isdigit(lastChar) || lastChar =='.'){
        printf("lastchar digit : %c\n", lastChar);
        char* numString = malloc(15 * sizeof(char));
        memset(numString, 0, sizeof(numString));
        printf("numstring before appending : %s\n", numString);
        do {
            append_char(lastChar, numString);
            printf("CHAR APPENDED %c\n", lastChar);
            temp = getCharFromString(tokreturn->str);
            printf("temp digit : %c\n", temp);
            if (temp != '\0'){
            lastChar = temp;
            } else {
            return tok_next_line;
            } 
        } while (isdigit(lastChar) || lastChar == '-'); 
        //char ptr;
        //strtol(numString, &ptr, 10);
        //*numString = strtod(numString, 0);
        int* tempInt = malloc(sizeof(int));
        *(tempInt) = atoi(numString);
        tokreturn->data = tempInt;
        printf("numString : %s\n", numString);
        printf("num : %d\n", atoi(numString));
        printf("data : %d\n", *(int*)tokreturn->data);
        return tok_number;
    }
    if (lastChar == '#'){
        do {
            temp = getCharFromString(tokreturn->str);
            if (temp != '\0'){
            lastChar = temp;
            } else {
            return tok_next_line;
            }
        } while (lastChar != EOF && lastChar != '\n' && lastChar != '\r');
        if (lastChar != EOF) return getTok(tokreturn); // recursively find other tokens
    }
    if (lastChar == EOF) return tok_eof;
    if (lastChar == '\n') return tok_next_line;
    if (lastChar == '+') return tok_plus;
    int currChar = lastChar;
    temp = getCharFromString(tokreturn->str);
    if (temp != '\0'){
    lastChar = temp;
    } else {
        return tok_next_line;
    }
    return currChar;
}

int getNextToken(struct TokReturn* tokreturn){
    return currTok = getTok(tokreturn);
}

tokenArray_t lexer(char* str){
	tokenArray_t temp_array ;
    initTokenArray(&temp_array, 1);
	enum Token temp_tok;
	int pos = 0;
    struct TokReturn* tokreturn = malloc(sizeof(struct TokReturn));
    tokreturn->str = str;
    temp_tok = getTok(tokreturn);
    if (temp_tok == tok_number){
        printf("data after function : %p\n", tokreturn->data);
        printf("data value after function : %d\n", *(int*)tokreturn->data);
    }
	while (temp_tok != tok_next_line){
	token_t temp;
    temp.data =  tokreturn->data;
    temp.type = temp_tok;
    addTokenToTokenArray(&temp_array, temp);
    printf("token added %d\n", temp.type);
    printf("token inarr %d\n", temp.type);
    printf("temp_tok : %d\n", temp_array.arr[temp_array.used].type);
    printf("temp_data ptr : %p\n", temp.data);
    printf("TEST\n");
    pos++;
    tokreturn->data = NULL;
    //data = malloc(sizeof(*data));
    temp_tok = getTok(tokreturn);
	}
    printf("TEST2\n");
	return temp_array;
}
