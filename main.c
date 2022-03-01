#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "interpret.h"
#ifdef _WIN32
#define ARGUMENT_START 1
#else
#define ARGUMENT_START 0
#endif

int main(int argc,char* argv[]){
    int i;
     if(argc==1)
    {
    printf("No Extra Command Line Argument Passed OtherThan Program Name");
    exit(0);
    }
    else
    {
    char argument[100];
    strcpy(argument, argv[1]);
    char compileArg[10];
    char debugArg[10];
    int isDebug = 0;
    char inputFilename[10];
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
    //printf("debug found\n");
    isDebug = 1;
    }
    else {
    strcpy(inputFilename,argv[i]);
    }
    }
    if (argv[1] != NULL){
    interpret(inputFilename, isDebug);
    }
    }
    return 0;
}