#ifndef __WASM__
#include <pthread.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif
#include "gc.h"


pthread_t* c_create_threads(void* (threadFunc)(void*)){
#ifndef NO_STD
    pthread_t* thread = GC_malloc(sizeof(pthread_t));
    pthread_create(thread, NULL, threadFunc, NULL);
    return thread;
#else 
    return NULL;
#endif
}

void c_thread_spawn(void* (threadFunc)(void*)){
    pthread_t* thread = c_create_threads(threadFunc);
    pthread_detach(*thread);
}

#endif
