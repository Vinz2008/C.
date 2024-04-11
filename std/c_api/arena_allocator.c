#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

// TODO : reimplement this in cpoint

#define ARENA_DEFAULT_SIZE 1024

typedef struct arena {
    size_t offset;
    size_t capacity;
    uint8_t* data;
} Arena;

Arena internal_arena_create(){
    uint8_t* mem = malloc(ARENA_DEFAULT_SIZE); // TODO : maybe switch to mmap or sbrk
    Arena arena = {
        .data = mem,
        .capacity = ARENA_DEFAULT_SIZE,
        .offset = 0
    };
    return arena;
}

void* internal_arena_alloc(Arena* a, size_t size){
    if (a->offset + size > a->capacity){
        a->data = realloc(a->data, a->capacity + ARENA_DEFAULT_SIZE);
    }
    void* p = a->data + a->offset;
    a->offset += size;
    return p;
}

void internal_arena_destroy(Arena* a){
    free(a->data);
}