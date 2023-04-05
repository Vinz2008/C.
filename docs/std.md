# Standard Library



## Strings



## Garbage collector

C. uses bdwgc for a garbage collector to not need free everywhere like in C.

Unless specified, declarations of the functions for gc_malloc (and gc_init but it is less useful for using the language) and functions like gc_free, gc_realloc and gc_strdup can be used by adding externs in the file.

```
extern gc_strdup(s : i8 ptr) i8 ptr;
extern gc_free(s : int ptr) void;
extern gc_realloc(s : int ptr, size : int) int ptr;
 
func main(){
    var s : int ptr = gc_malloc(sizeof int)
    s = gc_realloc(s, 2 * sizeof int)
    gc_free(s)    
}
```


