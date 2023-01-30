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
llvm::Type* get_type_llvm(int t, bool is_ptr = false, bool is_array = false, int nb_aray_elements = 0);

class Cpoint_Type {
public:
    int type;
    bool is_ptr;
    bool is_array;
    int nb_element;
    Cpoint_Type(int type, bool is_ptr = false, bool is_array = false, int nb_element = 0) : type(type), is_ptr(is_ptr), is_array(is_array), nb_element(nb_element) {}
};
