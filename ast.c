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

void generateAST(char* str){
    while (getTok != tok_next_line){
        
    }
}