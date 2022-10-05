#include "ast.h"
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>

struct astNode* firstNode = NULL;
struct astNode* lastNode;
struct astNode* nextNodeLeft;
struct astNode* nextNodeRight;
int nbNode = 0;

struct astNode createNode(struct astNode* previous, struct astNode* left, struct astNode* right, enum Token tag){
    struct astNode tempNode;
    tempNode.previous = previous;
    tempNode.left = left;
    printf("LEFT : %p\n", left);
    tempNode.right = right;
    tempNode.tag = tag; 
    return tempNode;
}

void addNode(struct astNode* node,  struct astNode* left, struct astNode* right){
    static struct astNode newNode;
    if (firstNode == NULL){
        firstNode = node;
        printf("FIRST NODE\n");
    }else {
        lastNode->left = left;
        lastNode->right = right;
    }
    lastNode = node;
}

void initAST(tokenArray_t tokenArr){
}

void printAST(struct astNode firstNodeTemp){
    printf("nbNode : %d\n", nbNode);
    //for(int i =0; i < nbNode; i++){
        printf("node : %d\n", firstNodeTemp.tag);
        printf("node left : %d\n", firstNodeTemp.left->tag);
        printf("node right : %d\n", firstNodeTemp.right->tag);
    //}
}

struct astNode* generateAST(tokenArray_t tokenArr){
    initAST(tokenArr);
    int posOther = 0;
    struct astNode* arrOther = malloc(sizeof(struct astNode) * 3);
    for (int i = 0; i < tokenArr.used; i++){
        printf("tok : %d\n", tokenArr.arr[i].type);
    }
    printf("size : %d\n", tokenArr.used);
    for (int i = 0; i < tokenArr.used /*tokenArr.arr[i].type != tok_next_line*/;i++){
         printf("tok verif : %d\n", tokenArr.arr[i].type);
        if (tokenArr.arr[i].type == tok_plus){
            struct astNode templ = createNode(NULL, NULL, NULL, tokenArr.arr[i-1].type);
            printf("tokenArr.arr[i+1].type : %d\n", tokenArr.arr[i+1].type);
            struct astNode tempr = createNode(NULL, NULL, NULL, tokenArr.arr[i+1].type);
            struct astNode tempNode = createNode(lastNode, &templ,  &tempr, tokenArr.arr[i].type);
            templ.previous = &tempNode;
            tempr.previous = &tempNode;
            addNode(&tempNode, &templ, &tempr);
            printf("TEST3 tempNode.tag %d\n", tempNode.tag);
            printf("TEST4\n");
            nbNode++;
        }
    }
    printAST(*firstNode);
    printf("firstNode pointer before : %p\n", firstNode);
    return firstNode;
}