# Standard Library

## Strings

You can use strings in 2 ways : 

**i8 pointer**

```
func main(){
    var string : i8 ptr = "hello world"
}
```

**string struct**

```
import @std/print.cpoint
import @std/str.cpoint

func main(){
    var string : struct str = String::create("hello world")
    printstring(string)
    String::appendChar('a', string)
    var len = String::lenstr(string)
}
```

The string struct is dynamically allocated with the garbage collector.

## Garbage collector

C. uses bdwgc for a garbage collector to not need free everywhere like in C.

Unless specified, declarations of the functions for gc_malloc (and gc_init but it is less useful for using the language) and functions like gc_free, gc_realloc and gc_strdup can be used by adding externs in the file.

```
extern gc_strdup(s : i8 ptr) i8 ptr;
extern gc_free(s : void ptr) void;
extern gc_realloc(s : int ptr, size : int) int ptr;

func main(){
    var s : int ptr = gc_malloc(sizeof int)
    s = gc_realloc(s, 2 * sizeof int)
    gc_free(s)    
}
```

## Files

You can open files, write to them, delete them or create symlinks.

```
import @std/file.cpoint

func main(){
    var fd = file::open("test_file.txt")
    file::write(fd, "test content of file\n")
    file::close(fd)
    file::remove("test_file.txt")
}
```

## Panic

There is a panic function which will print the error and exit with an error the program.

```
import @std/panic.cpoint

func main(){
    panic("test error")
    // the program doesn't continue to here because it has exited
}
```
