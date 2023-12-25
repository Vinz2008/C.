# C.


A programming language compiler written in C++ which is definitely not finished. It compile the C. code to LLVM IR.

## Advantages

- simple C-like language
- blazingly fast compile times
- predictable name-mangling
- compiler less than 830KB stripped, ≈25.5MB not stripped
- classes support
- goto support
- automatic casting
- generics
- rust-like pattern matching 
- gc and manual memory management

## Cloning the git repo

To clone the git repo, run 

```
git clone --recurse-submodules https://github.com/Vinz2008/C.
```

## WASM support

You can compile C. code to wasm using the ```-target-triplet wasm32-unknown-wasi``` option

## TODO

- [x] Generics support
- [x] Importing structs with function members
- [ ] Finish refactoring the code for operators to make it compatible with every types
- [ ] Make redeclarations just the equal operator
- [x] Fix array members and array member redeclaration bugs
- [ ] Maybe verify and warn in the manual allocation module in the standard library if an address was already freed by verifying in a list
- [ ] Add type checking in separate file and remove implicit cast and only have explicit casts with the "cast" keyword
- [x] Add rust-styled enums with match
- [x] Add a syntax to import every cpoint files in package
- [ ] Add chained struct members
- [x] Add chained else if
- [ ] Fix bug with type inference and struct pointers (for example in linked_list.cpoint "var tail = self.tail" that we needed to replace with "var tail : struct node_linked_list ptr = self.tail")
- [ ] Add unions support in import
- [ ] Add enum support in import
- [x] Add automatically when calling panic the file and line number to panic call
- [x] Move functions from headers to cpp files to lower compile times
- [x] Work on imports of template structs (for now in single file project you could include them) ( a way to fix problems with linking problems when generating the template would be to namespace the template with the filename)
- [x] Add macro functions (for example with a syntax like #function())
- [ ] Make if, for and while return values like in rust
- [ ] Move preprocessing to import stage
- [x] Deduplicate identical strings when creating them by keeping them in a hashmap when generating them
- [x] Add the string version of the expression in the expect macro
- [ ] Add rust-like "traits" for simple types like i32 or float (It will be called "members" and not traits but it will be the same : add methods to types, but it will need to make the '.' an operator)  
- [x] Add number variable support to match (using it like a switch in c) 
- [x] Add static struct declaration (like with arrays) like in C ((struct struct_example){val1, val2})
- [ ] Use precompiled header for the compiler code to speed up compilation (problem : https://stackoverflow.com/questions/9580058/in-gcc-can-precompiled-headers-be-included-from-other-headers)
- [ ] Remove constructors in the language (to make it rust-like) ?
- [x] Add members to structs with "members" blocks
- [x] Add recursive install for binaries with subprojects with cpoint-build
- [x] In cpoint-build, find subdependencies, compile them and link them
- [ ] Warn about/Remove unused functions
- [x] In cpoint-build, find libraries linked to dependencies and link them
- [x] Add "-compile-time-sizeof" flag to have sizeof not use getelementptr and the compiler insert the values at compile time
- [ ] Have specific flags for optimizations (ex : -optimize-return) or checking (-check-arrays) and global flags (-O2, -C0 for checking level 0) 
- [ ] maybe create a config file to have specific profile for flags (you could have -build-mode fast-no-opti with custom profile that would be defined in the config)
- [ ] Make comments in structs, enums, etc work
- [x] Add custom numbers for enum without types included (example : variant being 0x200 or 0x100)
- [x] Make va_args work in C. (maybe reimplement printfmt in c and compare the generated assemblies : see it with objdump -S)
- [x] Implement dbg macro
- [ ] Automatically make additional args in variadic functions i32s ? (for now it works without)
- [x] Add hex notation (example 0x77)
- [x] Add section selection for global variables
- [x] Fix for loop (add verification at the start in special basic block) (after doing this, change '<' into '<=' in the fibonacci benchmark)
- [ ] Make the compiler less "double focused" 
    - [ ] make numbers that have no decimal part ints by default
    - [ ] use by default in for loop ints for the variable and then add the possibility to set the type manually
- [x] Create a print/printfmt macro that will print vars/values .have using it be like #print("{} {}", a, 2), which will generate printfmt("%s %d", a, 2) by detecting types
- [x] Create a println macro equivalent to #print but adding automatically a '\n' at the end of the line
- [x] Remove python from benchmark pictures (to see better the differences between the system programming languages)
- [x] add a deref expression ? (ex : "var a : int = deref p")
- [x] Make no_gc work (see gc.c)
- [x] Add a little toml file in the std location to track if the latest build has the same options (no_gc, target, etc) as the current one to see if it is needed to rebuild the standard library
- [x] Make no gc subprojects in cpoint-build be build first, then the gc ones or the other way to have the standard library rebuilt max 2 times
- [ ] Make custom values for enums work even with only the first set like in C (those after are just incremented by 1 each time, ex : the first is set manually to 2, so the second is 3, the third is 4, etc)
- [ ] Generate typedefed structs in bindgen
- [ ] Add a custom file written in cpoint to do complex builds (like build.rs or build.zig) (it could give infos using the stdout like rust does)
- [ ] Make declarations private with private keyword (it will just be passed by the lexer) or even blocks (pass private blocks in the same way as mod blocks in imports)
- [x] Only generate the externs when functions are called
- [x] Make import work on struct templates (copy the functions instead of generating externs) (see test21.cpoint)
- [x] Make no_gc work directly with the cpoint compiler without cpoint-build (then make "make run" have -no-gc in the Makefile)
- [x] Make bools returned by operators and in for loops,etc i1s and not doubles
- [x] Add stripping mode to the compiler and the build system
- [ ] rerun benchmarks using hyperfine
- [x] Implement closures (lambdas) (use llvm trampoline : [useful link](https://www.reddit.com/r/ProgrammingLanguages/comments/mm9j4k/comment/gtshg44/?utm_source=share&utm_medium=web3x&utm_name=web3xcss&utm_term=1&utm_content=share_button))
- [x] Make closure captured vars usable in the closure (have a "is closure" flag for when doing the codegen of a closure) needing to use a struct named "closure" (if you need var a, you put "closure.a")
- Maybe in closure automatically generate the struct member code for the variables to use them transparently (if you need var a, you just put "a")
- [x] Make multithreaded cpoint-build
- [ ] Work on from-c translator
- [x] Cache object files in cpoint-build (compare timestamps of source files and object files to see if the file is needed to be compiled)
- [ ] Add synthax for input and outputs of variables in inline assembly like in Rust (https://doc.rust-lang.org/nightly/rust-by-example/unsafe/asm.html, https://doc.rust-lang.org/nightly/reference/inline-assembly.html)
- [x] Generate unreachables with unreachable (unreachable macro ?)
- [ ] Add "-backend" flag to select the backend (for also adding a libgccjit flag)
- [x] Add help flag (-h) to cpoint and cpoint-build
- [ ] Implement a eprintln equivalent (writing to stderr)
- [ ] Work on adding the sret attribute when needed to follow the x86 abi calling convention
- [x] Add folder selection in cpoint-build (like in make -C)
- [x] Add running test in cpoint-build
- [ ] Run automatically clang and create a out.o file for the c translator
- [ ] Replace sret store instruction with llvm.memcpy
- [x] Move everything about the abi (valist, sret, byval) in an abi.cpp file
- [ ] Add attributes to functions (ex : noreturn)
- [ ] ENABLE CLANG-FORMAT

## Benchmarks compared to other languages

[Complete benchmarks](https://github.com/Vinz2008/C./tree/master/docs/benchmarks.md)

Length of benchmarks in seconds, lower is better

![fibonacci benchmark](https://raw.githubusercontent.com/Vinz2008/C./master/docs/tools/fibonacci.png)

![factors benchmark](https://raw.githubusercontent.com/Vinz2008/C./master/docs/tools/factors.png)

## Note about the name

C. is pronounced like C point. It was named like it and not C dot or period but C point because I just named the executable cpoint without thinking about it and then remembered that it is not a transparent translation from french and that it should have been name ccomma, but hey I am too lazy to fix it  ¯\\_(ツ)_/¯

## Locales

The locales are not installed by default. If you want the locales file installed on your system, run make install in the "locales" directory