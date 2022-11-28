#include "lexer.h"

#ifndef AST_HEADER_
#define AST_HEADER_ 


enum LeftOrRight{
    left = 1,
    right = 2
};

struct Variable {
char name[10];
char value[20];
char type;
};

struct Argument {
char name[10];
char type;
};

struct Function {
char name[10];
int numberArguments;
struct Argument arguments[10];
char type;
};

struct astNode* generateAST(tokenArray_t tokenArr);
void emptyAST(struct astNode* node);


struct astNode {
    struct astNode  *previous;
    struct astNode  *left, *right;
    token_t tag;
};

#endif