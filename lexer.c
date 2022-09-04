#include "lexer.h"
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>


char* identifierString;
double numVal;
int currTok;
int pos = 0;

int getCharFromString(char* str){
    if (pos < strlen(str)){
        return str[pos++];
    } else {
        pos = 0;
        return 100000;
    }
}

int getTok(char* str){
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
            identifierString += lastChar;
        }
            if (identifierString == "function") return tok_function;
            if (identifierString == "extern") return tok_extern;
            return tok_identifier;
    }
    if (isdigit(lastChar) || lastChar =='.'){
        char* numString;
        do {
            strncat(numString, &lastChar, sizeof(lastChar));
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
        if (lastChar != EOF) return getTok; // recursively find other tokens
    }
    if (lastChar == EOF) return tok_eof;
    int currChar = lastChar;
    temp = getCharFromString(str);
    if (temp != 100000){
    lastChar = temp;
    }
    return currChar;
}


int getNextToken(char* str){ 
    return currTok = getTok(str);
}
