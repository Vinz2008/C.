#include <string.h>
#include <stdio.h>
#include "startswith.h"

int startswith(char strStartingWith[], char code[]) {
    int similarity = 0;
    int length = strlen(strStartingWith);
    /*printf("length : %i\n", length);*/
    int i;
    for (i = 0; i < length/*-1*/; i++)
    {
        if (strStartingWith[i] == code[i])
        {
            similarity++;
        }
    }
    /*similarity++;*/
    /*printf("similarity %i\n", similarity);*/
    if (similarity >= length)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}
