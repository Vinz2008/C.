#include <string>
#include <fstream>
#include <memory>
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
    bool is_ptr;
    int nb_ptr;
    std::string to_c_type_str();
    C_Type(int type, bool is_ptr) : type(type), is_ptr(is_ptr) {}
    C_Type(Cpoint_Type cpoint_type){
        enum c_translator::c_types c_type = c_translator::int_type;
        switch (cpoint_type.type){
            default:
                std::cout << "Unknown type" << std::endl;
                break;
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
        is_ptr = cpoint_type.is_ptr;
        nb_ptr = cpoint_type.nb_ptr;
    } 
};

namespace c_translator {
    /*class Expr {
    public:
        Expr(){}
        virtual ~Expr() = default;
        virtual std::string generate_c() = 0;
        virtual std::unique_ptr<Expr> clone() = 0;
    };*/
    class Function {
    public:
        C_Type return_type;
        std::vector<std::pair<std::string, C_Type>> args;
        std::string function_name;
        bool is_extern;
        //std::vector<std::unique_ptr<Expr>> body; 
        std::vector<std::unique_ptr<ExprAST>> body; 
        std::string generate_c();
        std::unique_ptr<Function> clone(); /*{
            std::vector<std::unique_ptr<ExprAST>> bodyCloned;
            for (int i = 0; i < body.size(); i++){
                //bodyCloned.push_back(body.at(i)->clone());
            }
            return std::make_unique<Function>(return_type, function_name, args, std::move(bodyCloned), is_extern);
        }*/
        Function(C_Type return_type, std::string function_name, std::vector<std::pair<std::string, C_Type>> args, std::vector<std::unique_ptr<ExprAST>> body, bool is_extern = false) : return_type(return_type), function_name(function_name), args(args), body(std::move(body)), is_extern(is_extern) {}
    };
    class Context {
    public:
        Context(){}
        std::vector<std::unique_ptr<Function>> Functions;
        std::vector<std::string> headers_to_add;
        void write_output_code(std::ofstream& stream);
    };
    /*class ForExpr : Expr {
        ForExpr()
    }:*/
    void init_context();
    void generate_c_code(std::string filename);
}

#endif