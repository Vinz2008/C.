#include <string>
#include <fstream>
#include "ast.h"

#ifndef _C_TRANSLATOR_HEADER_
#define _C_TRANSLATOR_HEADER_

namespace c_translator {
    enum c_types {
        double_type = -1,
        int_type = -2,
        float_type = -3,
        void_type = -4,
        char_type = -5,
        short_type = -6,
        long_type = -7,
        bool_type = -8
    };
}

class C_Type {
public:
    int type;
    std::string to_c_type_str();
    C_Type(int type) : type(type) {}
    C_Type(Cpoint_Type cpoint_type){
        enum c_translator::c_types c_type;
        switch (cpoint_type.type){
            default:
            case double_type:
                c_type = c_translator::double_type;
                break;
            case i32_type:
            case int_type:
                c_type = c_translator::int_type;
                break;
            case float_type:
                c_type = c_translator::float_type;
                break;
            case void_type:
                c_type = c_translator::void_type;
                break;
            case i8_type:
                c_type = c_translator::char_type;
                break;
            case i16_type:
                c_type = c_translator::short_type;
                break;
            case i64_type:
                c_type = c_translator::long_type;
                break;
        }
        type = c_type;
    } 
};

namespace c_translator {
    class Function {
    public:
        C_Type return_type;
        std::vector<C_Type> args;
        std::string function_name;
        bool is_extern;
        std::string generate_c();
        Function(C_Type return_type, std::string function_name, std::vector<C_Type> args, bool is_extern = false) : return_type(return_type), function_name(function_name), args(args), is_extern(is_extern) {}
    };
    class Context {
    public:
        Context(){}
        std::vector<std::unique_ptr<Function>> Functions;
        std::vector<std::string> headers_to_add;
        void write_output_code(std::ofstream& stream);
    };
    void init_context();
    void generate_c_code(std::string filename);
}

#endif