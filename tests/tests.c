#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define NB_FILES 6

int main(){
	char exe[] = "../cpoint";
	char* cmd = NULL;
	FILE* cmdf;
	for (int i = 0; i < NB_FILES; i++){
	int size = strlen(exe) + 1 + 7 + 1 + 7 + 1;
	cmd = malloc(size * sizeof(char));
	snprintf(cmd, size, "%s ../test%d.cpoint", exe, i+1);
	printf("cmd : %s\n", cmd);
	cmdf = popen(cmd, "r");
	char* out = malloc(1000 * sizeof(char));
	//fscanf(cmdf, "%s", out);
	fread(out, 1, 1000, cmdf);
	printf("out : %s\n", out);
	pclose(cmdf);
	free(cmd);
	free(out);
	}
	return 0;
}
