#define INTERNAL_FUNC_PREFIX "cpoint_internal_"

#define BUILD_TIMESTAMP __TIMESTAMP__

#define NOT_IMPLEMENTED() do { \
    fprintf(stderr, "NOT IMPLEMENTED yet at %s:%d\n", __FILE__, __LINE__); \
    exit(1); \
} while (0)
