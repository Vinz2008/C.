#include "codegen.h"
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <sys/types.h>
#ifdef __linux__
#include <linux/limits.h>
#endif
#include "lexer.h"
#include "ast.h"


void codegen(struct astNode* node, FILE* outfptr){
    struct astNode* currentNode = node;
    int left_used = 0;
    int right_used = 0;
    if (currentNode == NULL){ return; }
    printf("codegen current node tag : %d\n", currentNode->tag.type);
    switch (currentNode->tag.type){
    case tok_number:
	    printf("currentNode->tag.data ptr : %p\n", currentNode->tag.data);
	    fprintf(outfptr, "%d ", *(int*)currentNode->tag.data);
	    break;
	case tok_plus:
        fprintf(outfptr, "%d ", *(int*)currentNode->left->tag.data);
        left_used = 1;
        fprintf(outfptr, "plus ");
        fprintf(outfptr, "%d ", *(int*)currentNode->right->tag.data);
        right_used = 1;
	    break;
    case tok_import:
        char* pathStd = "./std";
        char tempStr[100];
        char* libraryName = (char*)currentNode->right->tag.data;
        snprintf(tempStr, PATH_MAX, "%s/%s", pathStd, libraryName);
        fprintf(outfptr, "; %s\n", libraryName);
        FILE* fptrTemp;
	    char* line = NULL;
	    ssize_t charNb;
	    size_t len;
        if ((fptrTemp = fopen(tempStr, "r")) == NULL){
	        printf("The file doesn't exist or cannot be opened\n");
	        exit(1);
	    }
            while ((charNb = getline(&line, &len, fptrTemp)) != -1){
	        fprintf(outfptr,"%s",line);
	        }
            fprintf(outfptr,"\n");
            memset(line, 0, strlen(line));
	default:
	    break;
    }
    if (left_used == 0){
    codegen(currentNode->left, outfptr);
    }
    if (right_used == 0){
    codegen(currentNode->right, outfptr);
    }
    /*
    while (currentNode->left != NULL){
        currentNode = currentNode->left;
    }
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
                printf("tag type : %d\n", currentNode->tag.type);
                if (currentNode->tag.type != tok_root_node){
                printf("BACK ONE NODE\n");
                currentNode = currentNode->previous->right;
                } else {
                    break;
                }
            }
        }
    }
    */
}
