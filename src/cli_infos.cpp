#include "cli_infos.h"
#include "config.h"
#include <iostream>
#include <llvm/Config/llvm-config.h>

void print_help(){
    std::cout << "Usage : cpoint [options] file" << std::endl;
    std::cout << "Options : " << std::endl;
    std::cout << "  -std : Select the path where is the std which will be builded" << std::endl;
    std::cout << "  -no-std : Make the compiler to not link to the std. It is the equivalent of -freestanding in gcc" << std::endl;
    std::cout << "  -c : Create an object file instead of an executable" << std::endl;
    std::cout << "  -target-triplet : Select the target triplet to select the target to compile to" << std::endl;
    std::cout << "  -verbose-std-build : Make the build of the standard library verbose. It is advised to activate this if the std doesn't build" << std::endl;
    std::cout << "  -no-delete-import-file : " << std::endl;
    std::cout << "  -no-gc : Make the compiler not add functions for garbage collection" << std::endl;
    std::cout << "  -with-gc : Activate the garbage collector explicitally (it is by default activated)" << std::endl;
    std::cout << "  -no-imports : Deactivate imports in the compiler" << std::endl;
    std::cout << "  -rebuild-gc : Force rebuilding the garbage collector" << std::endl;
    std::cout << "  -no-rebuild-std : Make the compiler not rebuild the standard library. You probably only need to rebuild it when you change the target" << std::endl;
    std::cout << "  -linker-flags=[flags] : Select additional linker flags" << std::endl;
    std::cout << "  -d : Activate debug mode to see debug logs" << std::endl;
    std::cout << "  -o : Select the output file name" << std::endl;
    std::cout << "  -g : Enable debuginfos" << std::endl;
    std::cout << "  -silent : Make the compiler silent" << std::endl;
    std::cout << "  -build-mode : Select the build mode (release or debug)" << std::endl;
    std::cout << "  -fPIC : Make the compiler produce position-independent code" << std::endl;
    std::cout << "  -compile-time-sizeof : The compiler gets the size of types at compile time" << std::endl;
    std::cout << "  -test : Compile tests" << std::endl;
    std::cout << "  -run-test : Run tests" << std::endl;
    std::cout << "others : TODO" << std::endl;
}


void print_version(){
    std::cout << "cpoint : build " << BUILD_TIMESTAMP << std::endl;
}

void print_infos(std::string llvm_default_target_triple){
    std::cout << "Target of compiler build : " << TARGET << std::endl;
    std::cout << "Default target for LLVM codegen : " << llvm_default_target_triple <<  std::endl;
    std::cout << "LLVM version : " << LLVM_VERSION_STRING << std::endl;
}