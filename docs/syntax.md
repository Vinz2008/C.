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

You can declare variable with the ```var``` and the select a default value

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
