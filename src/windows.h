#define realpath(N,R) _fullpath((R),(N),PATH_MAX)
#define WEXITSTATUS(status) status