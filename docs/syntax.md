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

You can declare variables with the ```var``` and the select a default value

```
var x = 1
```

You can also select the type of the variable by putting a colon and then a type name after the variable name

```
var x : int = 2
```

## Types

The available types are : 

- double

- int

- float

- i8

- void

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

You can just use argc and argv in the main function without having to declare them anywhere (to see more about it, see [this part of the docs](./theory.md#Args-(Argc-and-Argv))

```
func main() int {
    printi(argc)
}
```
