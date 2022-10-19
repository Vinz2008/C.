#include "codegen.h"
#include <stdio.h>
#include "lexer.h"
#include "ast.h"


void codegen(struct astNode* node, FILE* outfptr){
    struct astNode* currentNode = node;
    while (1){
        printf("codegen current node tag : %d\n", currentNode->tag.type);
        switch (currentNode->tag.type){
            case tok_number:
                printf("ptr : %p\n", currentNode->tag.data);
                //printf("number : %d\n", *(int*)currentNode->tag.data);
                //fprintf(outfptr, "%d", *(int*)currentNode->tag.data);
            default:
                break;
        }
        if (currentNode->left != NULL){
            currentNode = currentNode->left;
        } else {
            if (currentNode->right != NULL){
                currentNode = currentNode->right;
            } else {
                if (currentNode->previous->tag.type != tok_root_node){
                currentNode = currentNode->previous;
                } else {
                    break;
                }
            }
        }
    }

}