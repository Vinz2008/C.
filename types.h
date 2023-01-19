#pragma once
#include <iostream>
#include "llvm/IR/Type.h"

enum types {
    double_type = -1,
    int_type = -2,
    float_type = -3,
    i8_type = -4,
    void_type = -5,
    argv_type = -1000,
};

bool is_type(std::string type);
int get_type(std::string type);
llvm::Type* get_type_llvm(int t, bool is_ptr);
