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

struct AdditionalArgsList {
    size_t allocated_size;
    size_t used;
    char** list;
};

int main(int argc, char** argv){
    if (argc < 1){
        printf("need at least one argument\n");
        printf("use -h to open the help\n");
        exit(1);
    }
    char* filename = NULL;
    struct AdditionalArgsList additional_args = (struct AdditionalArgsList){
        .allocated_size = 1,
        .used = 0,
        .list = malloc(sizeof(char*)*1)
    };
    for (int i = 1; i < argc; i++){
        if (strcmp("-h", argv[i]) == 0 && !filename){
            // TODO
            fprintf(stderr, "not implemented for now");
            exit(1);
        } else if (!filename) {
            filename = argv[i];
        } else {
            additional_args.list = realloc(additional_args.list, additional_args.allocated_size+1);
            additional_args.allocated_size++;
            additional_args.list[additional_args.used] = argv[i];
            additional_args.used++;
        }
    }
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
        char* filename_without_shebang = malloc(sizeof(char) * FILENAME_MAX);
        snprintf(filename_without_shebang, FILENAME_MAX, "%s.without_shebang", filename);
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
    size_t size_alloc_c  = strlen(format) + strlen(filename);
    char* cmd = NULL;
    if (additional_args.used != 0) {
        char* format_append = " --run-args=\"%s\"";
        size_alloc_c += strlen(format_append);
        format = strdup(format);
        format = realloc(format, strlen(format) + strlen(format_append));
        strcat(format, format_append);
        size_t size_needed_for_args = 0;
        for (int i = 0; i < additional_args.used; i++){
            size_needed_for_args += strlen(additional_args.list[i])+2; // 2 for two spaces
        }
        size_alloc_c += size_needed_for_args;
        char* args = malloc(sizeof(size_needed_for_args));
        for (int i = 0; i < additional_args.used; i++){
            strcat(args, " ");
            strcat(args, additional_args.list[i]);
            strcat(args, " ");
        }
        cmd = malloc(size_alloc_c * sizeof(char));
        snprintf(cmd, size_alloc_c * sizeof(char), format, filename, args);
    } else {
        cmd = malloc(size_alloc_c * sizeof(char));
        snprintf(cmd, size_alloc_c * sizeof(char), format, filename);
    }
    system(cmd);
    free(cmd);
    if (found_shebang){
        free(filename);
    }
    return 0;
}