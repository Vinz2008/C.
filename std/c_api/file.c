#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

int cwritefile(int fd, const char* buf){
    return write(fd, buf, strlen(buf));
}