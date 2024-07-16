#include "cli_infos.h"
#include "config.h"
#include <iostream>
#include <llvm/Config/llvm-config.h>

#include "gettext.h"

void print_help(){
    std::string help_string = _(
"Usage : cpoint [options] file\n\
Options : \n\
  -std : Select the path where is the std which will be builded\n\
  -no-std : Make the compiler to not link to the std. It is the equivalent of -freestanding in gcc\n\
  -c : Create an object file instead of an executable\n\
  -target-triplet : Select the target triplet to select the target to compile to\n\
  -verbose-std-build : Make the build of the standard library verbose. It is advised to activate this if the std doesn't build\n\
  -no-delete-import-file : \n\
  -no-gc : Make the compiler not add functions for garbage collection\n\
  -with-gc : Activate the garbage collector explicitally (it is by default activated)\n\
  -no-imports : Deactivate imports in the compiler\n\
  -rebuild-gc : Force rebuilding the garbage collector\n\
  -no-rebuild-std : Make the compiler not rebuild the standard library. You probably only need to rebuild it when you change the target\n\
  -linker-flags=[flags] : Select additional linker flags\n\
  -d : Activate debug mode to see debug logs\n\
  -o : Select the output file name\n\
  -g : Enable debuginfos\n\
  -silent : Make the compiler silent\n\
  -build-mode : Select the build mode (release or debug)\n\
  -fPIC : Make the compiler produce position-independent code\n\
  -compile-time-sizeof : The compiler gets the size of types at compile time\n\
  -test : Compile tests\n\
  -run-test : Run tests\n\
" );
    std::cout << help_string << std::endl;
}


void print_version(){
    std::cout << "cpoint : build " << BUILD_TIMESTAMP << std::endl;
}

void print_infos(std::string llvm_default_target_triple){
    std::cout << "Target of compiler build : " << TARGET << std::endl;
    std::cout << "Default target for LLVM codegen : " << llvm_default_target_triple <<  std::endl;
    std::cout << "LLVM version : " << LLVM_VERSION_STRING << std::endl;
}