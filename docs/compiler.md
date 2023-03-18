# Compiler

## Options

**-std** : select the path where is the std which will be builded

**-no-std** : say to the compiler to not link to the std. It is the equivalent of -freestanding in gcc

**-c** : create an object file instead of an executable

**-target-triplet** : select the target triplet to select the target to compile to

**-verbose-std-build** : build the std verbosely. It is advised to activate this if the std doesn't build

**-no-delete-import-file** : make the compiler not delete the temporary source file which where the includes and the imports are replaced

**-verbose-std-build** : make the build of the standard library verbose

**-no-delete-import-file** : make the compiler not delete the temporary file with imports replaced with externs

**-no-gc** : make the compiler not add the functions for garbage collection

**-no-imports** : desactivate imports in the compiler

**-rebuild-gc** : force rebuilding the garbage collector source code

**-no-rebuild-std** : make the compiler not rebuild the standard library. You probably only need to rebuild it when you change the target

**-linker-flags=** : select additional linker flags


