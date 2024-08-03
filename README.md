# C.


A programming language compiler written in C++ which is definitely not finished. It compile the C. code to LLVM IR.

## Advantages

- simple C-like language
- blazingly fast compile times
- predictable name-mangling
- compiler less than 1.5MB stripped, ≈46MB not stripped
- classes support
- goto support
- automatic casting
- generics and closures
- rust-like pattern matching 
- gc and manual memory management
- extremely fast linking (mold used by default)
- includes a clang-based C compiler, an ar-like archiver and the lld linker (like zig)

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
- [x] Finish refactoring the code for operators to make it compatible with every types
- [x] Make redeclarations just the equal operator
- [x] Fix array members and array member redeclaration bugs
- [ ] Maybe verify and warn in the manual allocation module in the standard library if an address was already freed by verifying in a list
- [ ] Add type checking in separate file and remove implicit cast and only have explicit casts with the "cast" keyword
- [x] Add rust-styled enums with match
- [x] Add a syntax to import every cpoint files in package
- [ ] Add chained struct members
- [x] Add chained else if
- [x] Add unions support in import
- [x] Add enum support in import
- [x] Add automatically when calling panic the file and line number to panic call
- [x] Work on imports of template structs (for now in single file project you could include them) ( a way to fix problems with linking problems when generating the template would be to namespace the template with the filename)
- [x] Add macro functions (for example with a syntax like #function())
- [ ] Make if, for and while return values like in rust
- [x] Move preprocessing to import stage
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
- [ ] use by default in for loop ints for the variable and then add the possibility to set the type manually
- [x] Create a print/printfmt macro that will print vars/values .have using it be like #print("{} {}", a, 2), which will generate printfmt("%s %d", a, 2) by detecting types
- [x] Create a println macro equivalent to #print but adding automatically a '\n' at the end of the line
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
- [x] rerun benchmarks using hyperfine
- [x] Implement closures (lambdas) (use llvm trampoline : [useful link](https://www.reddit.com/r/ProgrammingLanguages/comments/mm9j4k/comment/gtshg44/?utm_source=share&utm_medium=web3x&utm_name=web3xcss&utm_term=1&utm_content=share_button))
- [x] Make closure captured vars usable in the closure (have a "is closure" flag for when doing the codegen of a closure) needing to use a struct named "closure" (if you need var a, you put "closure.a")
- [ ] Maybe in closure automatically generate the struct member code for the variables to use them transparently (if you need var a, you just put "a")
- [x] Make multithreaded cpoint-build
- [ ] Work on from-c translator
- [x] Cache object files in cpoint-build (compare timestamps of source files and object files to see if the file is needed to be compiled)
- [ ] Add syntax for input and outputs of variables in inline assembly like in Rust (https://doc.rust-lang.org/nightly/rust-by-example/unsafe/asm.html, https://doc.rust-lang.org/nightly/reference/inline-assembly.html)
- [x] Generate unreachables with unreachable (unreachable macro ?)
- [ ] Add "-backend" flag to select the backend (for also adding a libgccjit flag)
- [x] Add help flag (-h) to cpoint and cpoint-build
- [x] Implement a eprintln equivalent (writing to stderr)
- [x] Work on adding the sret attribute when needed to follow the x86 abi calling convention
- [x] Add folder selection in cpoint-build (like in make -C)
- [x] Add running test in cpoint-build
- [ ] Run automatically clang and create a out.o file for the c translator
- [ ] Replace sret store instruction with llvm.memcpy
- [x] Move everything about the abi (valist, sret, byval) in an abi.cpp file
- [ ] ENABLE CLANG-FORMAT
- [ ] Implement a format macro (like the format! macro in rust)
- [ ] Add REPL  
- [x] Add mold linker support to cpoint-build (and the linker driver in the cpoint exe ?)
- [x] Work on core library like in rust (make built-in types modules like i32 be imported by default)
- [x] Add support in members block for built-in types like i32
- [x] Make members block work in imports 
- [ ] Replace float and double with f32 and f64 ?
- [ ] Rename list to vec or vector in std
- [ ] Add a config file for default compiler flags, etc (and add a env var like CPOINTFLAGS ?)
- [ ] Fix error messages (wrong location, wrong line, etc)
- [ ] Refactor JIT code
- [ ] Work on an arena allocator 
- [x] Work on defer (to have memory management like in zig ?)
- [x] Make scopes expressions so you can declare scopes anywhere (ex : let a = { printd(2); var b = 2;  b + 1})
- [ ] Make variable work with scopes
- [x] Finish making all expression containing Body (vectors of ExprAST) using only an expression that can be a scope expression
- [ ] Fix cpoint-run which runs an old version of the code
- [ ] add const ptr and const types (const should be in type system)
- [ ] fix the bug with bindgen that makes const ptr args not be written in prototypes
- [ ] Remove int because it already exists ?  (and create an alias in core so you can use the alias but there would not be int_type and i32_type in the compiler, only i32_type)
- [x] Replace all ExprAST->clone()->codegen()->getType() with just ExprAST->get_type()
- [ ] Add noalias/align/dereferenceable/nonnull to function arguments and return values
- [x] Detect when a reordering of struct members (like does rust automatically is necessary) and do an informative warning about that
- [ ] Fix bug with global variable in custom section with no default initializer
- [ ] Create a way to cross-compile easily the compiler (target prefix on gcc ? clang when cross compiling ? clang by default ?)
- [x] Make closure private to file (like static functions in c)
- [ ] Split codegen.cpp (and ast.cpp ?) in match.cpp, closures.cpp, etc
- [ ] Make every string translated in french (and other languages ?) with gettext
- [ ] Refactor big functions in ast.cpp and codegen.cpp as smaller functions
- [ ] Remove useless Log::Info() calls
- [ ] Replace all uses emptyLoc by the real loc passed to the function 
- [ ] Add debuginfos 
- [ ] Add cc and ar like zig cc 
- [x] Add thread sanitizer (https://github.com/ziglang/zig/blob/master/src/zig_llvm.cpp#L307)
- [ ] Add thread sanitizer to cpoint-build
- [x] Add noreturn return value (return void and add the noreturn attribute to the function)
- [x] Remove protos for not existent functions from FunctionProtos (for example if there is a struct s with a member f, there will be the s__f and the f protos in it) 
- [ ] Make it possible to link llvm statically to the compiler 
- [ ] Fix bug when returning from function in if and else branch 
- [ ] Add lld to the compiler (like in zig) 
- [ ] Force default values to variables (maybe have in the future a default trait/function member like in rust) 
- [ ] Extend preprocessor functionalities (default vars, operators, else, etc)
- [x] Add a flag to only preprocess (-E ?)
- [ ] Create website for the programming language (use cssg ? svelte ?)
- [ ] Replace LogError with assert in verification that are garanteed to be true unless there is a bug in the compiler 
- [ ] Replace the globals variables by contexts (lexer context, compiler context that are global or passed to functions) 
- [ ] Get the target data layout from llvm (create functions for that) to get the alignement to calculate the reordering needed for struct members
- [x] Desactivate the warning for the struct reordering (especially in the std with struct that are accessible to C) (add extern before the struct or repr C after the struct keyword)
- [ ] Add warnings for unused labels (and unused functions ?) 
- [ ] Make warnings longer (for example the warnings about the types not being the same in operators)
- [ ] Replace some verification for identifiers by keywords verification
- [ ] Add a custom cli flag for default repr for structs 
- [ ] Add vector types for simd
- [ ] Add inout in inline asm args
- [ ] Make functions that are not static static if needed
- [ ] Add '...' to Constant Vectors exprs so the other numbers in the vectors would be the same as the last
- [ ] Add selecting functionalities of cpu in cli (ex : avx)
- [ ] Add a way to select the real linkname of a function in a string (like in rust) (it will help with intrisics : insted of hardcoding them, we could define them with another name in a file in core, ex : llvm.va_start) (even if we do that, we will still need to detect llvm intrisics because apparently it can't be called directly)
- [ ] Permit casting a vector from and to an array  
- [ ] Add support for struct and arrays to println
- [ ] Implement features like in rust
- [ ] Make the compiler less dependent on LLVM, so more backend agnostic which would enable the c backend and the custom asm backend (for this, for example see the targets.h TODO) 
- [ ] Make it possible to use the compiler with a non-clang linker (invoking the linker directly instead of a c compiler) (for example replace -Wl flags with normal flags. See the rustc_target in the rust project to see how they do it)
- [ ] Make a base file for each group of target in a base folder in the targets folder (like rust)
- [ ] If the cpu type is native, make llvm add the cpu features needed (https://github.com/rust-lang/rust/blob/2ccafed862f6906707a390caf180449dd64cad2e/compiler/rustc_codegen_llvm/src/llvm_util.rs#L517)
- [ ] Add an option for cpu types and features
- [ ] Add column number to debuginfos
- [ ] Add a isize and usize type that would be the same type of a pointer 
- [ ] Build the std with cpoint-build by default instead of make
- [ ] Add function members to enums
- [ ] Add templates to enums
- [ ] Make vars immutable by default
- [ ] Add c types (like c_type::bool to help interoperatibility with C code)
- [ ] Make types work with namespaces (structs and typedefs)
- [ ] Add inlined functions and other optimizations (like loop unrolling for for loops when we can know at compile time the number of iterations and it is low)

## Benchmarks compared to other languages

[Complete benchmarks](https://github.com/Vinz2008/C./tree/master/docs/benchmarks.md)

Length of benchmarks in seconds, lower is better

![fibonacci benchmark](https://raw.githubusercontent.com/Vinz2008/C./master/docs/tools/fibonacci.png)

![factors benchmark](https://raw.githubusercontent.com/Vinz2008/C./master/docs/tools/factors.png)

## Note about the name

C. is pronounced like C point. It was named like it and not C dot or period but C point because I just named the executable cpoint without thinking about it and then remembered that it is not a transparent translation from french and that it should have been name ccomma, but hey I am too lazy to fix it  ¯\\_(ツ)_/¯

## LLVM version used

The recommended LLVM version is 18.x. (17.x could work, but it is not garenteed to work without some edits in the code or disabling some flags)

## Locales

The locales are not installed by default. If you want the locales file installed on your system, run make install in the "locales" directory