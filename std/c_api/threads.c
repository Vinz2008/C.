#include <pthread.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif
#include "gc.h"

pthread_t* c_create_threads(void* (threadFunc)(void*)){
    pthread_t* thread = GC_malloc(sizeof(pthread_t));
    pthread_create(thread, NULL, threadFunc, NULL);
    return thread;
}
