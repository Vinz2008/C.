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
- [ ] Fix array members and array member redeclaration bugs


## Benchmarks compared to other languages

[Complete benchmarks](https://github.com/Vinz2008/C./tree/master/docs/benchmarks.md)

Length of benchmarks in seconds, lower is better

![fibonacci benchmark](https://raw.githubusercontent.com/Vinz2008/C./master/docs/tools/fibonacci.png)

![factors benchmark](https://raw.githubusercontent.com/Vinz2008/C./master/docs/tools/factors.png)