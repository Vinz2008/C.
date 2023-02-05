# Compiler

## Options

**-std** : select the path where is the std which will be builded

**-no-std** : say to the compiler to not link to the std. It is the equivalent of -freestanding in gcc

**-c** : create an object file instead of an executable

**-target-triplet** : select the target triplet to select the target to compile to

**-verbose-std-build** : build the std verbosely. It is advised to activate this if the std doesn't build

**-no-delete-import-file** : make the compiler not delete the temporary source file which where the includes and the imports are replaced


