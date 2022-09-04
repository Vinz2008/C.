#include "lexer.h"

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

void generateAST();



typedef struct ast {
    enum Token tag;
} AST;