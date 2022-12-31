#include <iostream>
#include "llvm/IR/Type.h"

enum types {
    double_type = -1,
    int_type = -2,
    float_type = -3
};

bool is_type(std::string type);
int get_type(std::string type);
llvm::Type* get_type_llvm(int t);
