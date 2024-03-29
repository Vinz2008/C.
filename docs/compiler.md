# Compiler

## Options

**-std** : select the path where is the std which will be builded

**-no-std** : make the compiler to not link to the std. It is the equivalent of -freestanding in gcc

**-c** : create an object file instead of an executable

**-target-triplet** : select the target triplet to select the target to compile to

**-verbose-std-build** : make the build of the standard library verbose. It is advised to activate this if the std doesn't build

**-no-delete-import-file** : make the compiler not delete the temporary source file which where the includes and the imports are replaced

**-no-gc** : make the compiler not add the functions for garbage collection

**-with-gc** : activate the garbage collector explicitally (it is by default activated)

**-no-imports** : deactivate imports in the compiler

**-rebuild-gc** : force rebuilding the garbage collector source code

**-no-rebuild-std** : make the compiler not rebuild the standard library. You probably only need to rebuild it when you change the target

**-linker-flags=** : select additional linker flags

**-d** : activate debug mode to see debug logs

**-o** : select the output file name

**-g** : enable debuginfos

**-silent** : make the compiler silent

**-build-mode** : select the build mode (release or debug)

**-fPIC** : make the compiler produce position-independent code

**-compile-time-sizeof** : the compiler gets the size of types at compile time and not at "runtime" (it is technically replaced by numbers at compile time by in llvm, but it is made independently of the target)

**-test** : enable tests

**-run-test** : run tests

**-strip**

**-c-translator** : 

**-use-native-target** : 

**-run** : 