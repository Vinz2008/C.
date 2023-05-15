# Syntax

## Functions

A function is created with the keywork ```func```

```
func main(){
    ...
}
```

You can select the type of the function by putting a type after the parenthesis

```
func test() int {
    ...
}
```

## Extern

You can declare functions with the ```extern``` keyword to say to the compiler that it exists outside of the cpoint file.

```
extern printd(x : double) void;
fun main(){
    printd(10)
}
```

## Variables

You can declare variables with the ```var``` keyword sand select a default value

```
var x = 1
```

You can also select the type of the variable by putting a colon and then a type name after the variable name

```
var x : int = 2
```

If you don't select manually the type, it will be infered

```
func main(){
    var a = "test"
    var b = 2
}
```

Here, the variable a will be an i8 ptr (technically just an opaque pointer in the backend because llvm is removing typed pointers) and the variable b will be a double

## Global Variables

You can also have global variables

```
var a : double = 2
func main(){
    printd(a)
}
```

You can also have variables that are constants

```
var const a : double = 2
func main(){
    printd(a)
}
```

## Types

The available types are : 

- double

- int

- float

- i8

- i16

- i32

- i64

- void

(Note : i8 is the equivalent in c of a char)

```
var x : float = 1.5
```

You can use the ```ptr``` keyword to make the type a ptr type

```
var x : i8 ptr = "This is a string"
```

You can also make it an array by using square brackets and the number of element.

```
var x[10] : int 
```

(The array implementation is for now broken, so using array for now is not recommended)

## If conditions

You can create conditions with the ```if``` keyword.

```
import @std/print.cpoint

func main(){
    if argc == 2 {
        printstr("2 args")
    }
}
```

You can also use the ```else``` keyword to do things when the condition is false

```
import @std/print.cpoint

func main(){
    if argc == 2 {
        printstr("2 args")
    } else {
        printstr("not 2 args")
    }
}
```

## While loops

You can create a while loop with the ```while``` keyword to create a loop that will do something until a condition is true.

```
import @std/print.cpoint

func main(){
    var i = 0
    while i < 10 {
        printd(i)
        i = i + 1
    }
}
```



## For loops

You can create a for loop with the ```for``` keyword to create a loop that will do something until a condition is true and initalize a variable and select a value to add to it in the syntax.

```
import @std/print.cpoint

func main(){
    for i = 1, i < 10, 1.0 {
        printd(i)
    }
}
```

## Structs

To declare a struct, put a ```struct``` keyword.

```
struct test {
    var x : i8
    var x : float
}
```

 To declare a struct variable, put `struct` and then the name of the struct type in the type declaration of the variable

```
var x : struct test
```

## Imports and includes

To import the functions from a file, use the ```import``` keyword.

```
import ./test.cpoint
```

You can use '@' to change to path in different ways.

- With std
  
  You can put std to say the path of the standard libray.
  
  ```
  import @std/print.cpoint
  ```

- with github
  
  You can put github to say to the compiler that the file is in github repository.
  
  The part after the '@' should be in this form : @github:[USERNAME]:[REPO NAME]/[FILENAME]
  
  ```
  import @github:Vinz2008:test_cpoint_package/test.cpoint
  ```

You can also use the ```include``` keyword to insert the entire content of a file. 

Only use this with Externs (for example to interface with C code that can't be imported) because if you put in files that are included functions, they will be in multiple object file when you will be linking the executable. It is strongly advised to use .hpoint file for files that should be included to distinguish source files and "header files".

```
include @std/libc.hpoint
```

## Arguments

You can just use argc and argv in the main function without having to declare them anywhere (to see more about it, see [this part of the docs](./theory.md#Args-(Argc-and-Argv)))

```
func main() int {
    printi(argc)
}
```

## Labels and gotos

You can use goto to jump to labels that you put in your code. It can be useful for error handling.

```
func main(){
label test
    goto test
}
```

## Loops

You can create an infinite loop with the ```loop``` keyword.

```
import @std/print.cpoint

func main(){
    loop {
        printd(3)
    }    
}
```
