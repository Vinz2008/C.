# C.

A programming language compiler written in C++ which is definitely not finished. It compile the C. code to LLVM IR.

## Advantages

- simple C-like language
- blazingly fast compile times
- predictable name-mangling
- less than 500 KB stripped, ≈12MB not stripped
- classes support
- goto support
- automatic casting

## Cloning the git repo

To clone the git repo, run 

```
git clone --recurse-submodules https://github.com/Vinz2008/C.
```

## WASM support

You can compile C. code to wasm using the ```-target-triplet wasm32-unknown-wasi``` option

## TODO

- [ ] Generics support
- [x] Importing structs with function members
- [ ] Finish refactoring the code for operators to make it compatible with every types
- [ ] Make redeclarations just the equal operator


## Benchmarks compared to other languages

[The benchmarks](https://github.com/Vinz2008/Language-benchmarks)

The benchmark are run on a sytem with a ryzen 5 5600g, 32go of ram and running Manjaro with linux 6.1.38-1.

Finding the 50th Fibonacci number using a recursive function

C. : 230.813s
Rust release (rustc 1.69.0) : 71.294s
Rust debug :
C (gcc 13.1.1) : 149.729s
Nim 1.6.10 release :  27.447s
Nim debug : 
Php :
Lua :
LuaJIT 2.1.0-beta3 : 288.73s
Crystal 1.8.2 : 118.306s
Go 1.20.5 : 133.766s
Java : 
V : 180.918s
Python 3.11.3 : 3957.103s
Javascript in firefox : 
