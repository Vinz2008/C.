#include<sys/time.h>
#include <stdio.h>

long long __internal_get_time(){
    struct timeval tv; 
    gettimeofday(&tv, NULL);
    unsigned long long millisecondsSinceEpoch =
    (unsigned long long)(tv.tv_sec) * 1000 +
    (unsigned long long)(tv.tv_usec) / 1000;
    return millisecondsSinceEpoch;
}