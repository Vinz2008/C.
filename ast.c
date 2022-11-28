#include "ast.h"
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <stdbool.h>

struct astNode* firstNode = NULL;
struct astNode* lastNode;
struct astNode* nextNodeLeft;
struct astNode* nextNodeRight;
int nbNode = 0;

struct astNode* createNode(struct astNode* previous, struct astNode* left, struct astNode* right, enum Token tag, void* data){
    struct astNode* tempNode = malloc(sizeof(struct astNode));
    tempNode->previous = previous;
    tempNode->left = left;
    printf("LEFT : %p\n", left);
    tempNode->right = right;
    tempNode->tag.type = tag; 
    tempNode->tag.data = data;
    return tempNode;
}

void addNode(struct astNode* node, bool toRight, struct astNode* left, struct astNode* right){
    static struct astNode newNode;
    if (firstNode == NULL){
        firstNode = node;
        printf("FIRST NODE\n");
    }else {
        if (toRight){
            lastNode->right = node;
            lastNode = node;
            lastNode->right = right;
            lastNode->left = left;


        } else {
            lastNode->left = node;
            lastNode = node;
            lastNode->right = right;
            lastNode->left = left;
        }
        return;
    }
    lastNode = node;
}

void initAST(tokenArray_t tokenArr){
    struct astNode* rootNode = createNode(NULL, NULL, NULL, tok_root_node, NULL);
    firstNode = rootNode;
    lastNode = rootNode;
}

void emptyAST(struct astNode* node){
    free(node);
    firstNode = NULL;
    lastNode = NULL;
}

void printAST(struct astNode* firstNodeTemp){
    struct astNode tempNode = *firstNodeTemp;
    printf("nbNode : %d\n", nbNode);
    //for(int i =0; i < nbNode; i++){
        if (firstNodeTemp != NULL){
        if (tempNode.left != NULL){
        printf("node : %d\n", tempNode.left->tag.type);
        printf("pointer left : %p\n", tempNode.left->left);
        printf("pointer left tag : %p\n", &tempNode.left->left->tag.type);
        printf("node left : %d\n", tempNode.left->left->tag.type);
        printf("node right : %d\n", tempNode.left->right->tag.type);
        }
        }
    //}
}

struct astNode* generateAST(tokenArray_t tokenArr){
    initAST(tokenArr);
    int posOther = 0;
    for (int i = 0; i < tokenArr.used; i++){
        printf("tok : %d\n", tokenArr.arr[i].type);
    }
    printf("size : %d\n", tokenArr.used);
    for (int i = 0; i < tokenArr.used /*tokenArr.arr[i].type != tok_next_line*/;i++){
         printf("tok verif : %d\n", tokenArr.arr[i].type);
        if (tokenArr.arr[i].type == tok_plus){
            struct astNode* templ = createNode(NULL, NULL, NULL, tokenArr.arr[i-1].type, tokenArr.arr[i-1].data);
            printf("tokenArr.arr[i+1].type : %d\n", tokenArr.arr[i+1].type);
            struct astNode* tempr = createNode(NULL, NULL, NULL, tokenArr.arr[i+1].type, tokenArr.arr[i+1].data);
            struct astNode* tempNode = createNode(lastNode, templ,  tempr, tokenArr.arr[i].type, tokenArr.arr[i].data);
            templ->previous = tempNode;
            tempr->previous = tempNode;
            addNode(tempNode, 0, templ, tempr);
            printf("TEST3 tempNode.tag %d\n", tempNode->tag.type);
            printf("TEST4\n");
            nbNode++;
        }
        else if (tokenArr.arr[i].type == tok_import){
            printf("IMPORT\n");
            struct astNode* tempNode = createNode(lastNode, NULL,  NULL, tokenArr.arr[i].type, tokenArr.arr[i].data);
            struct astNode* tempr = createNode(tempNode, NULL, NULL, tokenArr.arr[i-1].type, tokenArr.arr[i+1].data);
            tempr->right = tempr;
            addNode(tempNode, 0, NULL, tempr);
            nbNode++;
        }
    }
    printAST(firstNode);
    printf("firstNode pointer before : %p\n", firstNode);
    return firstNode;
}