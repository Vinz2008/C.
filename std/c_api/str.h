#define DEFAULT_FACTOR 2

struct struct_str_c {
    char* ptr;
    int length;
    int maxlength;
    int factor; // the number of characters to preallocate when growing
};