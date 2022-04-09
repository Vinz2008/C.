#include "interpret.h"   
#include "libs/startswith.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#include "libs/removeCharFromString.h"

struct Variable {
char name[10];
char value[20];
char type;
};     

int interpret(char filename[], char filecompileOutput[],int debugMode, int compileMode) {
    int i;
    int i2;
    FILE *fptr;
    if (compileMode == 1) {
    FILE *fptrtemp;
    fptrtemp = fopen(filecompileOutput, "w");
    fclose(fptrtemp);
    }
    FILE *fptrOutput; //Only used if compiling
    fptrOutput = fopen(filecompileOutput, "w");
    char line[40];
    //printf("filename opening 3 : %s\n", filename);
    fptr = fopen(filename, "r");
    struct Variable varArray[20];
	int nbVariable = 0;
    if (fptr == NULL)
    {
        printf("Error! The file is empty\n");   
        exit(1);
    }
     while (fgets(line,40, fptr)) {
        //strcpy(line, removeCharFromString("\t", line));
        if (debugMode == 1) {
        printf("%s", line);
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
        int sizeLineList = 0;
        int isFunctionInt = 0;
        char lineList[10][10];
        char *pch = strtok(line," ");
        while (pch != NULL)
	    {
            sizeLineList++;
            if (debugMode == 1) {
            printf ("pch : %s\n",pch);
            }
	    strcpy(lineList[c], pch);
       for (i = 0; i < strlen(lineList[c]); i++) {
           if (debugMode == 1) {
            printf("char : %c\n",lineList[c][i]);
           }
            if (lineList[c][i] == '(') {
                posFirstParenthesis = i;
                if (debugMode == 1) {
                printf("posFirstParenthesis: %i\n",posFirstParenthesis);
                }
                for (i2 = i; i2 < strlen(lineList[c]); i2++ )
                {
                    if (lineList[c][i2] == ")"[0]) {
                        posLastParenthesis = i2;
                        if (debugMode == 1) {
                        printf("posLastParenthesis: %i\n",posLastParenthesis);
                        }
		        break;

                    }
                }
            }
	    else if (lineList[c][i] == "\""[0]) {
	    if (posLastQuote == i) {
        if (debugMode == 1) {
	    printf("end quote so pass it\n");
        }
	    }
	    else {
	    posFirstQuote = i;
        if (debugMode == 1) {
	    printf("posFirstQuote: %i\n", posFirstQuote);
        }    
	    for (i2 = i+ 1; i2 < strlen(lineList[c]); i2++) {
	    if (lineList[c][i2] == "\""[0]) {
	    posLastQuote = i2;
        if (debugMode == 1) {
	    printf("posLastQuote: %i\n", posLastQuote);
        }
	    break;
	    }
	    }
	    }
	    }

        }
    	//pch = strtok (NULL, " \t");
	    pch = strtok (NULL, " ");

	    c++;
	    }
        for (i = 0; i < sizeLineList; i++) {
            if (debugMode == 1) {
            printf("lineList[i]: %s\n", lineList[i]);
            }
            //if instructions
            if (startswith("print", lineList[i])){
                int i3 = 0;
                char stringToPrint[20];
                if (debugMode == 1) {
                printf("print detected\n");
                }
                for (i2 = posFirstQuote + 1; i2 < posLastQuote; i2++) {
                    if (debugMode == 1) {
                    printf("lineList[i][i2]: %c\n",lineList[i][i2]);
                    }
                    stringToPrint[i3] = lineList[i][i2];
                    i3++;    
                }
                if (compileMode == 1) {
                    //printf("compile print\n");
                    fprintf(fptrOutput, "printf(\"");
		    fprintf(fptrOutput, "%s",stringToPrint);
		    fprintf(fptrOutput, "\");\n");
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
            if (startswith("return", lineList[i])){
                if (debugMode == 1) {
                    printf("return detected\n");
                }
                char returnedThing[10];
                 strcpy(returnedThing, lineList[i+1]);
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
		}

            }
            if (startswith("if", lineList[i])){
                if (debugMode == 1) {
                    printf("if detected\n");
                }

            }
            if (startswith("func", lineList[i])){
                isFunctionInt = 0;
                if (debugMode == 1) {
                    printf("function detected\n");
                }
                if (startswith("int", lineList[i + 1])){
                    if (debugMode == 1) {
                    printf("function %s is an int\n", lineList[i+2]);
                    }
                    //isFunctionInt = 1;
		    if (compileMode == 1){
		    fprintf(fptrOutput, "int ");
		    }
                }
	        if (compileMode == 1){
		fprintf(fptrOutput, ";\n");
		}
            }
            if (startswith("int", lineList[0]) || startswith("char", lineList[0])){
                if (debugMode == 1) {
                    printf("int detected\n");
            	}	        
            strcpy(varArray[nbVariable].name, lineList[i + 1]);
	        strcpy(varArray[nbVariable].value, lineList[i + 2]);
	        if (startswith("int",lineList[i])){
		    varArray[nbVariable].type = 'i';
	    	if (debugMode == 1) {
                    printf("int var initialization detected\n");
            	}
	        }
	        if (startswith("char",lineList[i])){
		    varArray[nbVariable].type = 'c';
	    	if (debugMode == 1) {
		    printf("char var initialization detected\n");
		    }
	        }
	        nbVariable++;
	        }
        }
        }
    //memset(line, 0, sizeof(line));
    memset(varArray,0, sizeof(varArray));
    return 0;
}
