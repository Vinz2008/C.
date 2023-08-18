#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <limits.h>
#include <string.h>

bool startswith(char* str, char* line){
    int similarity = 0;
    int length = strlen(str);
    int i;
    for (i = 0; i < length; i++){
        if (str[i] == line[i]){
            similarity++;
        }
    }
    return similarity >= length;
}

int main(int argc, char** argv){
    if (argc < 1){
        printf("need at least one argument\n");
        printf("use -h to open the help\n");
        exit(1);
    }
    char* filename = NULL;
    for (int i = 0; i < argc; i++){
        if (strcmp("-h", argv[i]) == 0){

        } else {
            filename = argv[i];
        }
    }
    //char* filename = argv[1];
    if (!filename){
        printf("couldn't find a filename in the arguments\n");
        exit(1);
    }
    char line[256];
    FILE* f = fopen(filename, "r");
    if (!fgets(line, sizeof(line), f)){
        printf("couldn't get line from file\n");
        exit(1);
    }
    bool found_shebang = false;
    if (startswith("#!", line)){
        found_shebang = true;
        char* filename_without_shebang = malloc(sizeof(char) * 4096);
        snprintf(filename_without_shebang, 4096, "%s.without_shebang", filename);
        FILE* f_without_shebang = fopen(filename_without_shebang, "w");
        if (!f_without_shebang){
            printf("couldn't open temp file\n");
            exit(1);
        }
        while (fgets(line, sizeof(line), f)){
            fprintf(f_without_shebang, "%s", line);
        }
        fclose(f_without_shebang);
        filename = filename_without_shebang;
    }
    fclose(f);
    char* format = "cpoint -run -silent %s";
    size_t size_alloc_c = strlen(format) + strlen(filename);
    char* cmd = malloc(size_alloc_c * sizeof(char));
    snprintf(cmd, size_alloc_c * sizeof(char), format, filename);
    system(cmd);
    free(cmd);
    if (found_shebang){
        free(filename);
    }
    return 0;
}