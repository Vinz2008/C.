#include "c_translator.h"
#include <fstream>
#include <cstdarg>
#include <iostream>
#include "ast.h"
#include "codegen.h"

std::unique_ptr<c_translator::Context> c_translator_context;

std::string generate_body_c(std::vector<std::unique_ptr<ExprAST>>& body);

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
        std::string function_str;
        function_str += return_type.to_c_type_str() + " " + function_name + "(";
        for (int i = 0; i < args.size(); i++){
            function_str += args.at(i).second.to_c_type_str() + " " + args.at(i).first;
            if (i != args.size()-1){
                function_str += ", ";
            }
        }
        if (is_variadic){
            function_str += ", ...";
        }
        function_str += ")";
        if (is_extern){
            function_str += ";";
        } else {
            function_str += "{\n";
            if (!body.empty()){
            //function_str += generate_body_c(body);
            for (int i = 0; i < body.size(); i++){
                std::string temp = body.at(i)->generate_c() + ";\n";
                if (i == body.size()-1 && temp.find("return") == std::string::npos && return_type.type != void_type){
                    temp = "return " + temp;
                }
                function_str += temp;
            }
            }
            function_str += "}\n";
        }
        return function_str;
    }
    std::unique_ptr<Function> Function::clone(){
            std::vector<std::unique_ptr<ExprAST>> bodyCloned;
            for (int i = 0; i < body.size(); i++){
                bodyCloned.push_back(body.at(i)->clone());
            }
            return std::make_unique<Function>(return_type, function_name, args, std::move(bodyCloned), is_extern, is_variadic);
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
    if (is_struct){
        return "struct " + struct_name;
    }
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
    case c_translator::long_long_type:
        s += "long long";
        break;
    default:
        s += "double";
        break;
    }
    if (is_ptr){
        for (int i = 0; i < nb_ptr; i++){
            s += "*";
        }
    }
    return s;
}

void types_used(int nb, ...){
    va_list args;
    va_start(args, nb);
    for (int i = 0; i < nb; i++){
        C_Type temp_type = *va_arg(args, C_Type*);
        //std::cout << temp_type.to_c_type_str() << std::endl;
        //std::cout << temp_type.type << std::endl;
        if (temp_type.type == c_translator::bool_type){
            if(std::find(c_translator_context->headers_to_add.begin(), c_translator_context->headers_to_add.end(), "stdbool.h") == c_translator_context->headers_to_add.end()){
                c_translator_context->headers_to_add.push_back("stdbool.h");
            }
        }

    }
    va_end(args);
}

std::string generate_body_c(std::vector<std::unique_ptr<ExprAST>>& body){
    std::string body_content = "";
    for (int i = 0; i < body.size(); i++){
        body_content += body.at(i)->generate_c() + ";\n";
    }
    return body_content;
}

c_translator::Function* FunctionAST::c_codegen(){
    std::string function_name = Proto->getName();
    C_Type return_type = C_Type(Proto->cpoint_type);
    std::vector<std::pair<std::string, C_Type>> args;
    for (int i = 0; i < Proto->Args.size(); i++){
        C_Type type_temp = C_Type(Proto->Args.at(i).second);
        args.push_back(std::make_pair(Proto->Args.at(i).first, type_temp));
        TYPES_USED(&type_temp);
    }
    if (function_name == "main"){
        return_type = C_Type(c_translator::int_type);
    }
    TYPES_USED(&return_type);
    std::vector<std::unique_ptr<ExprAST>> body;
    if (!Body.empty()){
    for (int i = 0; i < Body.size(); i++){
        body.push_back(std::move(Body.at(i)));
    }
    }
    auto functionTemp = new c_translator::Function(return_type, function_name, args, std::move(body), false, Proto->is_variable_number_args);
    c_translator_context->Functions.push_back(/*std::make_unique<c_translator::Function>(*functionTemp)*/ functionTemp->clone());
    return functionTemp;
}

c_translator::Function* PrototypeAST::c_codegen(){
    C_Type return_type = C_Type(cpoint_type);
    std::vector<std::pair<std::string, C_Type>> args;
    for (int i = 0; i < Args.size(); i++){
        C_Type type_temp = C_Type(Args.at(i).second);
        args.push_back(std::make_pair(Args.at(i).first, type_temp));
        TYPES_USED(&type_temp);
    }
    TYPES_USED(&return_type);
    auto functionTemp = new c_translator::Function(return_type, Name, args, {}, true, is_variable_number_args);
    c_translator_context->Functions.push_back(/*std::make_unique<c_translator::Function>(*functionTemp)*/ functionTemp->clone());
    return functionTemp;
}


std::string ForExprAST::generate_c() {
    std::string for_content = "for (";
    for_content += C_Type(VarType).to_c_type_str() + " " + VarName + " = " + Start->generate_c() +"; ";
    for_content += End->generate_c() + "; ";
    for_content += VarName + " = " + VarName + "+" + Step->generate_c();
	for_content += "){\n";
    //for_content += generate_body_c(Body); // TODO this
    /*for (int i = 0; i < Body.size(); i++){
        for_content += Body.at(i)->generate_c() + ";\n";
    }*/
    for_content += "}\n";
	return for_content; 
}

std::string NumberExprAST::generate_c(){
    std::string number_content = "";
    if (trunc(Val) == Val){
        number_content = std::to_string((long)Val);
    } else {
        number_content = std::to_string(Val);
    }
    std::replace(number_content.begin(), number_content.end(), ',', '.');
    return number_content;
}

std::string CallExprAST::generate_c(){
    std::string callexpr_content = "";
    callexpr_content += Callee + "(";
    for (int i = 0; i < Args.size(); i++){
        if (i != 0){
            callexpr_content += ",";
        }
        callexpr_content += Args.at(i)->generate_c();
    }
    callexpr_content += ")";
    return callexpr_content;
}

std::string VariableExprAST::generate_c(){
    return Name;
}

std::string BinaryExprAST::generate_c(){
    std::string binary_expr_content = "";
    binary_expr_content += LHS->generate_c() + Op + RHS->generate_c();
    return binary_expr_content;
}

std::string UnaryExprAST::generate_c(){
    std::string unary_expr_content = "";
    unary_expr_content += Opcode + Operand->generate_c();
    return unary_expr_content;
}

std::string VarExprAST::generate_c(){
    std::string var_expr_content = "";
    var_expr_content += C_Type(cpoint_type).to_c_type_str() + " " + VarNames.at(0).first + " = " + VarNames.at(0).second->generate_c();
    return var_expr_content;
}

std::string CastExprAST::generate_c(){
    std::string cast_expr_content = "";
    cast_expr_content += "(" + C_Type(type).to_c_type_str() + ")" + ValToCast->generate_c();
    return cast_expr_content;
}

std::string NullExprAST::generate_c(){
    c_translator_context->headers_to_add.push_back("stddef.h"); // is also defined in stdlib.h : https://en.cppreference.com/w/c/types/NULL
    return "NULL";
}

std::string ReturnAST::generate_c(){
    std::string return_content = "";
    return_content += "return " + returned_expr->generate_c();
    return return_content;
}

std::string BreakExprAST::generate_c(){
    return "break";
}

std::string GotoExprAST::generate_c(){
    return "goto " + label_name;
}

std::string LabelExprAST::generate_c(){
    return label_name + ":";
}

std::string LoopExprAST::generate_c(){
    std::string loop_expr_content = "";
    if (is_infinite_loop){
        loop_expr_content += "while (1){\n";
        loop_expr_content += generate_body_c(Body);
        for (int i = 0; i < Body.size(); i++){
            loop_expr_content += Body.at(i)->generate_c() + ";\n";
        }
        loop_expr_content += "}";
    }
    return loop_expr_content;
}

std::string StringExprAST::generate_c(){
    std::string str_replaced = "";
    for (int i = 0; i < str.size(); i++){
        if (str.at(i) == '\n'){
            str_replaced += "\\n";
        } else {
            str_replaced += str.at(i);
        }
    }
    return "\"" + str_replaced + "\"";
}

std::string CharExprAST::generate_c(){
    return (std::string)"'" + (char)c + (std::string)"'";
}

std::string IfExprAST::generate_c(){
    return "";
}

std::string WhileExprAST::generate_c(){
    return "";
}

std::string BoolExprAST::generate_c(){
    if (val){
        return "true";
    }
    return "false";
}

std::string AddrExprAST::generate_c(){
    return "&" + Expr->generate_c();
}

std::string DerefExprAST::generate_c(){
    return "*" + Expr->generate_c();
}

std::string SizeofExprAST::generate_c(){
    if (!is_type){
        return "sizeof(" + expr->to_string() + ")";
    } else {
        C_Type sizeof_type = C_Type(type);
        TYPES_USED(&sizeof_type);
        return "sizeof(" + sizeof_type.to_c_type_str() + ")";
    }
}

std::string AsmExprAST::generate_c(){
    // TODO : add input-output vars/regs
    return "asm(\"" + Args->assembly_code->str + "\")";
}