#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "lexer.h"
#include "ast.h"
#include "codegen.h"
#include "interpret.h"
#define ARGUMENT_START 1


int main(int argc,char* argv[]){
    int i;
     if(argc==1)
    {
    printf("No Extra Command Line Argument Passed Other Than Program Name\n");
    exit(0);
    }
    else
    {
    char argument[100];
    strcpy(argument, argv[1]);
    char compileArg[10];
    char debugArg[10];
    int isDebug = 0;
    int llvmMode = 1;
    int devASTMode = 0;
    int IsCompile = 0;
    char inputFilename[15];
    char tempFileName[15];
    char fileCompileOutput[11];
    for (i=ARGUMENT_START;i<argc;i++) 
    {
    //printf("argv[%i] : %s\n",i, argv[i]);
    /*if(strcmp(argv[i], "-c") == 0)
    {
    //printf("compiling found\n");
    //strcpy(compileArg, argv[i+1]);
    //printf("compileArg : %s\n", compileArg);
    i++;
    }*/
    if(strcmp(argv[i], "-d") == 0)
    {
    printf("debug found\n");
    isDebug = 1;
    }
    else if (strcmp(argv[i], "-c") == 0) {
        IsCompile = 1;

    }
    else if (strcmp(argv[i], "-i") == 0){
    	IsCompile = 0;
    }
    else if (strcmp(argv[i], "--llvm") == 0) {
        llvmMode = 1;
    }
    else if (strcmp(argv[i], "--dev-ast") == 0) {
        devASTMode = 1;
    }
    else {
    memset(inputFilename,0,sizeof(inputFilename));
    strcpy(inputFilename,argv[i]);
    //printf("filename input %s\n", inputFilename);
    }
    }
    //printf("filename opening 1 : %s\n", inputFilename);
    strcpy(tempFileName,inputFilename);
    if (IsCompile == 1) {
        strcpy(fileCompileOutput, "out.c"); 
    } else if (llvmMode == 1) {
        strcpy(fileCompileOutput, "out.ll"); 
    } else {
        strcpy(fileCompileOutput, ""); 
    }
    if (argv[1] != NULL){
    if (devASTMode == 1){
        char line[100]; 
        FILE* file = fopen(inputFilename, "r");
        fgets(line, sizeof (line), file);
        tokenArray_t lexerArr = lexer(line);
        struct astNode* firstNode = generateAST(lexerArr);
        printf("AFTER AST\n");
        printf("firstNode pointer after : %p\n", firstNode);
        printf("firstNode = %d\n", firstNode->tag.type);
        printf("firstNode left = %d\n", firstNode->left->tag.type);
        printf("firstNode left right = %d\n", firstNode->left->right->tag.type);
        printf("firstNode left left = %d\n", firstNode->left->left->tag.type);
        printf("START CODEGEN\n");
        FILE* outfptr = fopen(fileCompileOutput, "w");
        codegen(firstNode, outfptr);
        fclose(outfptr);
        fclose(file);
        return 0;
    }
    //printf("filename opening 2 : %s\n", inputFilename);
    strcpy(inputFilename, tempFileName);
    //printf("filename opening 3 : %s\n", inputFilename);
    interpret(inputFilename, fileCompileOutput, isDebug, IsCompile, llvmMode);
    }
    }
    return 0;
}
