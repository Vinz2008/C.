# C.

A programming language compiler written in C++ which is definitely not finished. It compile the C. code to llvm ir.

## Advantages

- simple C-like language
- blazingly fast compile times
- predictable name-mangling

## Clone git repo

To clone the git repo, run 

```
git clone --recurse-submodules
```

## WASM support

You can compile C. code to wasm using the ```-target-triplet wasm32-unknown-wasi``` option