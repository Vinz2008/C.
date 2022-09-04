#include "interpret.h"
#include <unistd.h>
#include <limits.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef __linux__
#include <linux/limits.h>
#endif
#include "parser.h"
#include "ast.h"
#include "types.h"
#include "utils.h"
#include "libs/removeCharFromString.h"
#include "libs/isCharContainedInStr.h"
#include "libs/startswith.h"
#include "libs/color.h"



int interpret(char filename[], char filecompileOutput[],int debugMode, int compileMode, int llvmMode) {
    int i;
    int i2;
    int posSlash;
    char pathTemp[PATH_MAX];
    char pathStd[PATH_MAX];
    memset(pathTemp,0,sizeof(pathTemp));
    if (readlink("/proc/self/exe", pathTemp, PATH_MAX) == -1) {
    printf("ERROR : the compiler can't find itself and so can't find std");
    } else {
    printf("%s\n", pathTemp);
    for (i = strlen(pathTemp) - 1; i > 0; i--){
    if (pathTemp[i] == '/'){
    posSlash = i;
    if (debugMode == 1){
    printf("posSlash : %i\n", posSlash);
    }
    break;
    }
    }
    for (i = 0; i <= posSlash; i++){
    pathStd[i] = pathTemp[i];
    }
    strncat(pathStd, "std", 4);
    if (debugMode == 1){
    printf("pathStd : %s\n", pathStd);
    }
    }
    FILE *fptr;
    FILE* declarationTempFile;
    char declarationTempFileName[PATH_MAX];
    snprintf(declarationTempFileName, PATH_MAX, "%s.declaration.temp", filecompileOutput);
    FILE* functionTempFile;
    char functionTempFileName[PATH_MAX];
    snprintf(functionTempFileName, PATH_MAX, "%s.function.temp", filecompileOutput);
    if (compileMode == 1) {
    FILE *fptrtemp;
    fptrtemp = fopen(filecompileOutput, "w");
    fclose(fptrtemp);
    }
    if (llvmMode == 1){
    FILE *fptrtemp2;
    fptrtemp2 = fopen(declarationTempFileName, "w");
    fclose(fptrtemp2);
    FILE* fptrtemp3;
    fptrtemp3 = fopen(functionTempFileName, "w");
    fclose(fptrtemp3);
    functionTempFile = fopen(functionTempFileName, "w");
    }
    FILE *fptrOutput; //Only used if compiling
    fptrOutput = fopen(filecompileOutput, "w");
    char line[40];
    //printf("filename opening 3 : %s\n", filename);
    fptr = fopen(filename, "r");
    //struct Variable varArray[20];
    struct Variable* varArray = malloc(20*sizeof(struct Variable));
    //struct Function FuncArray[20];
    struct Function* funcArray = malloc(20*sizeof(struct Function));
    int nbVariable = 0;
    int nbVariableMax = 20;
    if (fptr == NULL)
    {
        printf("Error! The file is empty\n");
        exit(1);
    }
    if (llvmMode == 1){
    char* filenameBase = basename(filename);
    fprintf(fptrOutput, "source_filename = \"%s\"\n", filenameBase);
    }
    while (fgets(line,40, fptr)) {
    removeCharFromString('\t',line);
    removeCharFromString('\n',line);
    if (debugMode == 1) {
    printf("line : %s\n", line);
    }
    char line2[40];
	char lineTemp[40];
	i2 = 0;
	for (i=0;i<strlen(line);i++){
	if (line[i] != '\n'){
	lineTemp[i2]= line[i];
	i2++;
	}
	}
	strcpy(line, lineTemp);
	memset(lineTemp,0,sizeof(lineTemp));
    strcpy(line2, line);
    int c = 0;
    int posLastQuote = -1;
    int posFirstQuote;
    int posFirstParenthesis;
    int posLastParenthesis;
    struct ParsedText parsedText;
    parsedText.sizeLineList = 0;
    int isFunctionInt = 0;
    //char lineList[10][10];
    parsedText.lineList =  malloc(10 * sizeof(char*));
    memset(parsedText.lineList, 0, sizeof(parsedText.lineList));
    char tempStr[PATH_MAX];
    char* libraryName;
    if (debugMode == 1) {
    printf("BEFORE PARSER\n");
    }
    parser(line, &parsedText, posFirstQuote, posLastQuote, posFirstParenthesis, posLastParenthesis, debugMode);
    if (debugMode == 1) {
    printf("AFTER PARSER\n");
    printf("sizeLineList after parsing  : %d\n", parsedText.sizeLineList);
    printf("lineList[1] after parsing : %s\n", parsedText.lineList[1]);
    }
    for (i = 0; i < parsedText.sizeLineList; i++) {
        if (debugMode == 1) {
        printf("lineList[i]: %s\n", parsedText.lineList[i]);
        }
        //if instructions
        if (startswith("import", parsedText.lineList[i])){
            libraryName = parsedText.lineList[i+1];
            printf("path std: %s\n", pathStd);
            snprintf(tempStr, PATH_MAX, "%s/%s", pathStd, libraryName);
            /*strcpy(tempStr, pathStd);
            strncat(tempStr, "/", 2);
            strncat(tempStr, libraryName, strlen(libraryName));*/
            if (debugMode == 1) {
            printf("File %s in %s imported\n", libraryName, tempStr);
            }
            fprintf(fptrOutput, "; %s\n", libraryName);
            FILE* fptrTemp;
	        char* line = NULL;
	        ssize_t charNb;
	        size_t len;
            if ((fptrTemp = fopen(tempStr, "r")) == NULL){
	        printf("The file doesn't exist or cannot be opened\n");
	        exit(1);
	        }
            while ((charNb = getline(&line, &len, fptrTemp)) != -1){
	        fprintf(fptrOutput,"%s",line);
	        }
            fprintf(fptrOutput,"\n");
            memset(line, 0, strlen(line));
            //memset(tempStr, 0, strlen(tempStr));  
        } else if(startswith("}", parsedText.lineList[i])){
            if (debugMode == 1) {
                printf("} detected\n");
            }
            if (llvmMode == 1){
                fprintf(functionTempFile, "}");
            }
        }
        else if (startswith("print", parsedText.lineList[i])){
            int i3 = 0;
            char stringToPrint[20];
            if (debugMode == 1) {
            printf("print detected\n");
            }
            for (i2 = posFirstQuote + 1; i2 < posLastQuote; i2++) {
                if (debugMode == 1) {
                printf("lineList[i][i2]: %c\n",parsedText.lineList[i][i2]);
                }
                stringToPrint[i3] = parsedText.lineList[i][i2];
                i3++;
            }
            if (compileMode == 1) {
                //printf("compile print\n");
                fprintf(fptrOutput, "printf(\"");
		        fprintf(fptrOutput, "%s",stringToPrint);
		        fprintf(fptrOutput, "\");\n");
            } else if (llvmMode == 1) {
                fprintf(functionTempFile, "call i32 @printf(");
            }
            else {
            if (debugMode == 1) {
            printf("stringToPrint: %s\n", stringToPrint);
            printf("---REAL OUTPUT---\n");
            }
            printf("%s\n", stringToPrint);
            if (debugMode == 1) {
            printf("-----------------\n");
            }
            }
        }
	    else if (isFunctionCall(parsedText.lineList[i])){
	        char functionName[100];
	        for (int i3 = 0; parsedText.lineList[i][i3] != '('; i3++){
                functionName[i3] = parsedText.lineList[i][i3];
	        }
	        if (debugMode == 1){
	    	printf("calling function %s detected\n", functionName);
	    	}
	    	if (llvmMode == 1){
	    	fprintf(functionTempFile, "call ");
        	fprintf(functionTempFile, "@%s()", functionName);
	    	}
	    }
        else if (startswith("return", parsedText.lineList[i])){
            if (debugMode == 1) {
                printf("return detected\n");
            }
            char returnedThing[10];
            strcpy(returnedThing, parsedText.lineList[i+1]);
             /*if (isFunctionInt == 1) {
            	printf("function is Int detected \n");
                int returnedThingInt;
                returnedThingInt = (int)returnedThing;
                memset( returnedThing, 0, strlen(returnedThing) );
                int returnedThing = returnedThingInt;
                }*/
            if (debugMode == 1) {
                printf("returnedThing: %s\n", returnedThing);
            }
		    if (compileMode == 1){
		    fprintf(fptrOutput, "return ");
		    fprintf(fptrOutput, "%s;\n", returnedThing);
		    } else if (llvmMode == 1){
                fprintf(functionTempFile, "ret ");
                if (isNumber(returnedThing) == 1){
                if (debugMode == 1) {
            		printf("the returned value %s is an int\n", returnedThing);
                }
                fprintf(functionTempFile, "i32 %d", atoi(returnedThing));
            	} else {
                    fprintf(functionTempFile, "%s",  returnedThing);
                }
            }

        }
        else if (startswith("if", parsedText.lineList[i])){
            if (debugMode == 1) {
                printf("if detected\n");
		        printf("%c\n", parsedText.lineList[i+1][0]);
            }
		    if (parsedText.lineList[i+1][0] != '('){
		        printf(BRED "ERROR : no parenthesis in if\n" reset);
		        printf("%s\n", line2);
		        exit(0);
		    }

        }
        else if (startswith("func", parsedText.lineList[i])){
            isFunctionInt = 0;
            if (debugMode == 1) {
                printf("function detected\n");
            }
            if (llvmMode == 1){
                fprintf(functionTempFile, "define ");
            }
            if (startswith("int", parsedText.lineList[i + 1])){
                if (debugMode == 1) {
                printf("function %s is an int\n", parsedText.lineList[i+2]);
                }
                char* functionName = parsedText.lineList[i+2];
		        removeCharFromString('(', functionName);
		        removeCharFromString(')',functionName); 
                //isFunctionInt = 1;
		        if (compileMode == 1){
		        	fprintf(fptrOutput, "int ");
		        }
                else if(llvmMode == 1){
                    fprintf(functionTempFile, "i32 ");
                    fprintf(functionTempFile, "@%s", functionName);
                    fprintf(functionTempFile, "(){\n");
                }
            }
	        if (compileMode == 1){
		    fprintf(fptrOutput, "\n");
		    }
        }
        if (isTypeWord(parsedText.lineList[i]) == 1){
            if (debugMode == 1) {
                printf("int detected\n");
            }
            strcpy(varArray[nbVariable].name, parsedText.lineList[i + 1]);
	        strcpy(varArray[nbVariable].value, parsedText.lineList[i + 2]);
	        if (startswith("int",parsedText.lineList[i])){
		    varArray[nbVariable].type = 'i';
	    	if (debugMode == 1) {
                printf("int var initialization detected\n");
            }
	        }
	        if (startswith("char",parsedText.lineList[i])){
		    varArray[nbVariable].type = 'c';
	    	if (debugMode == 1) {
		    printf("char var initialization detected\n");
		    }
	        }
	        nbVariable++;
		    if (nbVariable >= nbVariableMax){
		    varArray = realloc(varArray, 10 + sizeof(varArray));
		    nbVariableMax += 10;
		    }
	    }
    }
    memset(parsedText.lineList, 0 ,sizeof(parsedText.lineList));
    if (line != "" && line != "\0" && line != "\n"){
    fprintf(functionTempFile, "\n");
    if (debugMode == 1){
        printf("backslash n written to line\n");
    }
    }
    }
    fclose(functionTempFile);
        //fclose(declarationTempFile);
    if (llvmMode == 1){
        ssize_t charNb;
	    size_t len;
        FILE* fptrTempFunction = fopen(functionTempFileName, "r");
	    char* lineFunction = NULL;
        while ((charNb = getline(&lineFunction, &len, fptrTempFunction)) != -1){
	        fprintf(fptrOutput,"%s",lineFunction);
	    }
        FILE* fptrTempDeclaration = fopen(declarationTempFileName, "r");
        char* lineDeclaration = NULL;
        while ((charNb = getline(&lineDeclaration, &len, fptrTempFunction)) != -1){
	        fprintf(fptrOutput,"%s",lineDeclaration);
	    }
    }
    //memset(line, 0, sizeof(line));
    fclose(fptr);
    fclose(fptrOutput);
    memset(&varArray,0, sizeof(varArray));
    memset(&funcArray,0,sizeof(funcArray));
    return 0;
}
