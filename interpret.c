#include "interpret.h"   
#include "libs/startswith.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#include "libs/removeCharFromString.h"
                       

int interpret(char filename[], int debugMode) {
    int i;
    int i2;
    FILE *fptr;
    char line[40];
    fptr = fopen(filename, "r");
    if (fptr == NULL)
    {
        printf("Error! The file is empty\n");   
        exit(1);
    }
     while (fgets(line,150, fptr)) {
        //strcpy(line, removeCharFromString("\t", line));
        if (debugMode == 1) {
        printf("%s", line);
        }
        char line2[40];
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
            if (lineList[c][i] == "("[0]) {
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
            char stringToPrint[20];
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
                if (debugMode == 1) {
                printf("stringToPrint: %s\n", stringToPrint);
                printf("---REAL OUTPUT---\n");
                }
                printf("%s\n", stringToPrint);
                if (debugMode == 1) {
                printf("-----------------\n");
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

            }
            if (startswith("function", lineList[i])){
                isFunctionInt = 0;
                if (debugMode == 1) {
                    printf("function detected\n");
                }
                if (startswith("int", lineList[i + 1])){
                    if (debugMode == 1) {
                    printf("function is an int\n");
                    }
                    //isFunctionInt = 1;
                }
            }
        }
        }

    return 0;
}
