#include "ast.h"
#include "c_translator.h"
#include <fstream>
#include <cstdarg>
#include <iostream>

std::unique_ptr<c_translator::Context> c_translator_context;

#define NUMARGS(...)  (sizeof((C_Type*[]){__VA_ARGS__})/sizeof(C_Type*))
#define TYPES_USED(...) (types_used(NUMARGS(__VA_ARGS__), __VA_ARGS__))

namespace c_translator {
    void init_context(){
        c_translator_context = std::make_unique<c_translator::Context>();
    }
    void generate_c_code(std::string filename){
        std::ofstream file_out(filename);
        c_translator_context->write_output_code(file_out);
    }
    std::string Function::generate_c(){
        std::string body;
        body += return_type.to_c_type_str() + " " + function_name + "(";
        for (int i = 0; i < args.size(); i++){
            body += args.at(i).to_c_type_str() + " arg" + std::to_string(i);
            if (i != args.size()-1){
                body += ", ";
            }
        }
        body += ")";
        if (is_extern){
            body += ";";
        } else {
            body += "{}\n";
        }
        return body;
    }
    void Context::write_output_code(std::ofstream& stream){
        for (int i = 0; i < headers_to_add.size(); i++){
            stream << "#include <" << headers_to_add.at(i) << ">\n";
        }
        if (headers_to_add.size() > 1){
            stream << "\n";
        }
        for (int i = 0; i < Functions.size(); i++){
            stream << Functions.at(i)->generate_c() << "\n";
        }
    }

}

std::string C_Type::to_c_type_str(){
    std::string s = "";
    switch (type){
    case c_translator::double_type:
        s += "double";
        break;
    case c_translator::int_type:
        s += "int";
        break;
    case c_translator::float_type:
        s += "float";
        break;
    case c_translator::void_type:
        s += "void";
        break;
    case c_translator::char_type:
        s += "char";
        break;
    case c_translator::short_type:
        s += "short";
        break;
    case c_translator::bool_type:
        s += "bool";
        break;
    default:
        break;
    }
    return s;
}

void types_used(int nb, ...){
    va_list args;
    va_start(args, nb);
    for (int i = 0; i < nb; i++){
        C_Type temp_type = *va_arg(args, C_Type*);
        //std::cout << temp_type.to_c_type_str() << std::endl;
        if (temp_type.type == c_translator::bool_type){
            if(std::find(c_translator_context->headers_to_add.begin(), c_translator_context->headers_to_add.end(), "stdbool.h") == c_translator_context->headers_to_add.end()){
                c_translator_context->headers_to_add.push_back("stdbool.h");
            }
        }

    }
    va_end(args);
}


c_translator::Function* FunctionAST::c_codegen(){
    std::string function_name = Proto->getName();
    C_Type return_type = C_Type(Proto->cpoint_type);
    std::vector<C_Type> args;
    for (int i = 0; i < Proto->Args.size(); i++){
        C_Type type_temp = C_Type(Proto->Args.at(i).second);
        args.push_back(type_temp);
        TYPES_USED(&type_temp);
    }
    if (function_name == "main"){
        return_type = C_Type(c_translator::int_type);
    }
    TYPES_USED(&return_type);
    auto functionTemp = new c_translator::Function(return_type, function_name, args,  false);
    c_translator_context->Functions.push_back(std::make_unique<c_translator::Function>(*functionTemp));
    return functionTemp;
}

c_translator::Function* PrototypeAST::c_codegen(){
    C_Type return_type = C_Type(cpoint_type);
    std::vector<C_Type> args;
    for (int i = 0; i < Args.size(); i++){
        C_Type type_temp = C_Type(Args.at(i).second);
        args.push_back(type_temp);
        TYPES_USED(&type_temp);
    }
    TYPES_USED(&return_type);
    auto functionTemp = new c_translator::Function(return_type, Name, args, true);
    c_translator_context->Functions.push_back(std::make_unique<c_translator::Function>(*functionTemp));
    return functionTemp;
}


