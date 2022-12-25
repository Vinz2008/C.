#include <stdint.h>

struct str {
    char* ptr;
    int length;
    int maxlength;
    int factor; // the number of characters to preallocate when growing
};