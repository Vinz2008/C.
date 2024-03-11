# Theory

## LLVM

The Cpoint compiler compiles the .cpoint source files in IR with the LLVM C++ API. Then LLVM transforms this cross-platform intermediate representation in an object file. Then Clang is used to link the object file, the standard library, packages from github and the libc.

## Args (Argc and Argv)

The compiler detects functions that are called "main" and automatically put its arguments as an int for argc and a char** for the argv. The arguments in the source files for the main function will be ignored.

## Numbers

By default, every number is a double.

## Reserved names by compilers (for functions)

- names containing names with more than one underscore after each others (ex : a___b)
- functions starting with i32__