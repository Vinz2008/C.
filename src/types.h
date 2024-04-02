#pragma once
#include <iostream>
#include "llvm/IR/Type.h"
#include "llvm/IR/Value.h"
#include "llvm/IR/DerivedTypes.h"

enum types {
    double_type = -1,
    int_type = -2,
    float_type = -3,
    void_type = -4,
    i8_type = -5,
    i16_type = -6,
    i32_type = -7,
    i64_type = -8,
    i128_type = -9,
    u8_type = -10,
    u16_type = -11,
    u32_type = -12,
    u64_type = -13,
    u128_type = -14,
    bool_type = -15,
    argv_type = -1000,
};

class Cpoint_Type;

bool is_type(std::string type);
int get_type(std::string type);
std::string create_pretty_name_for_type(Cpoint_Type type);


class Cpoint_Type {
public:
    int type;
    bool is_ptr;
    int nb_ptr;
    bool is_array;
    int nb_element;
    bool is_struct;
    std::string struct_name;
    bool is_union;
    std::string union_name;
    bool is_enum;
    std::string enum_name;
    bool is_template_type;
    bool is_struct_template;
    bool is_empty;
    /*std::string*/ Cpoint_Type* struct_template_type_passed;
    bool is_function;
    std::vector<Cpoint_Type> args; // the first is the return type, the other are arguments
    Cpoint_Type* return_type;
    Cpoint_Type(int type, bool is_ptr = false, int nb_ptr = 0, bool is_array = false, int nb_element = 0, bool is_struct = false, const std::string& struct_name = "", bool is_union = false, const std::string& union_name = "", bool is_enum = false, const std::string& enum_name = "", bool is_template_type = false, bool is_struct_template = false, /*const std::string&*/ Cpoint_Type* struct_template_type_passed = nullptr, bool is_function = false, std::vector<Cpoint_Type> args = {}, Cpoint_Type* return_type = nullptr) 
                : type(type), is_ptr(is_ptr), nb_ptr(nb_ptr), is_array(is_array), nb_element(nb_element), is_struct(is_struct), struct_name(struct_name), is_union(is_union), union_name(union_name), is_enum(is_enum), enum_name(enum_name), is_template_type(is_template_type), is_struct_template(is_struct_template), struct_template_type_passed(struct_template_type_passed), is_function(is_function), args(args), return_type(return_type) {
                    is_empty = false;
                }
    Cpoint_Type() {
        this->is_empty = true;
    }
    friend std::ostream& operator<<(std::ostream& os, const Cpoint_Type& type){
        os << "{ type : " << type.type << " is_ptr : " << type.is_ptr << " nb_ptr : " << type.nb_ptr  << " is_struct : " << type.is_struct << " is_array : " << type.is_array << " nb_element : " << type.nb_element << " is_template_type : " << type.is_template_type << " }"; 
        return os;
    }
    friend bool operator==(const Cpoint_Type& lhs, const Cpoint_Type& rhs){
        return lhs.type == rhs.type && lhs.is_ptr == rhs.is_ptr && lhs.nb_ptr == rhs.nb_ptr && lhs.is_array == rhs.is_array && lhs.nb_element == rhs.nb_element && lhs.is_struct == rhs.is_struct && lhs.struct_name == rhs.struct_name && lhs.is_union == rhs.is_union && lhs.union_name == rhs.union_name && lhs.is_enum == rhs.is_enum && lhs.enum_name == rhs.enum_name && lhs.is_template_type == rhs.is_template_type && lhs.is_struct_template == rhs.is_struct_template && lhs.is_function == rhs.is_function;
    }
    const char* c_stringify(){
        std::string str = create_pretty_name_for_type(*this);
        return str.c_str();
    }

};

llvm::Type* get_type_llvm(Cpoint_Type cpoint_type);
llvm::Value* get_default_value(Cpoint_Type type);
llvm::Constant* get_default_constant(Cpoint_Type type);
Cpoint_Type get_cpoint_type_from_llvm(llvm::Type* llvm_type);
bool is_llvm_type_number(llvm::Type* llvm_type);
llvm::Type* get_array_llvm_type(llvm::Type* type, int nb_element);
bool is_decimal_number_type(Cpoint_Type type);
bool is_signed(Cpoint_Type type);
bool is_unsigned(Cpoint_Type type);
bool convert_to_type(Cpoint_Type typeFrom, llvm::Type* typeTo, llvm::Value* &val);
bool convert_to_type(Cpoint_Type typeFrom, Cpoint_Type typeTo, llvm::Value* &val);
int get_type_number_of_bits(Cpoint_Type type);
std::string get_string_from_type(Cpoint_Type type);
std::string create_mangled_name_from_type(Cpoint_Type type);
llvm::Constant* from_val_to_constant_infer(llvm::Value* val);
llvm::Constant* from_val_to_constant(llvm::Value* val, Cpoint_Type type);
int from_val_to_int(llvm::Value* val);
std::string from_cpoint_type_to_printf_format(Cpoint_Type type);
int find_struct_type_size(Cpoint_Type cpoint_type);
bool is_just_struct(Cpoint_Type type);
int struct_get_number_type(Cpoint_Type cpoint_type, int type);
bool is_struct_all_type(Cpoint_Type cpoint_type, int type);
llvm::VectorType* vector_type_from_struct(Cpoint_Type cpoint_type);