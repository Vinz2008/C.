#include "ast.h"
#include <stddef.h>

struct astNode firstNode;
struct astNode* lastNode;

struct astNode createNode(struct astNode* previous, struct astNode* left, struct astNode* right, enum Token tag){
    struct astNode tempNode;
    tempNode.previous = previous;
    tempNode.left = left;
    tempNode.right = right;
    return tempNode;
}

void addNode(struct astNode* node, char* str, enum LeftOrRight leftOrRight){
    struct astNode newNode;
    newNode = createNode(lastNode, NULL, NULL, getTok(str));
    if (leftOrRight == left){
        lastNode->left = &newNode;
    } else {
        lastNode->right = &newNode;
    }
    lastNode = &newNode;
}

void initAST(char* str){
    firstNode = createNode(NULL, NULL, NULL, getTok(str));
    lastNode = &firstNode;
}

struct astNode* generateAST(enum Token* tokenArr){
    for (int i =0; tokenArr[i] != tok_next_line;i++){
        if (tokenArr[i] == tok_plus){
            printf("PLUS\n");
        }
        if (i == sizeof(tokenArr)/sizeof(tokenArr[0])){
            break;
        }
    }
    return &firstNode;
}