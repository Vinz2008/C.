#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#ifdef _WIN32
#define write(fd, buffer, count) _write(fd, buffer, count)
#endif

int cwritefile(int fd, const char* buf){
    return write(fd, buf, strlen(buf));
}