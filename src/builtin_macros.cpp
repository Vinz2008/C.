#include "builtin_macros.h"
#include <memory>
#include <ctime>

void add_manually_extern(std::string fnName, Cpoint_Type cpoint_type, std::vector<std::pair<std::string, Cpoint_Type>> ArgNames, unsigned Kind, unsigned BinaryPrecedence, bool is_variable_number_args, bool has_template, std::string TemplateName);

extern std::string filename;

std::unique_ptr<StringExprAST> get_filename_tok(){
    std::string filename_without_temp = filename;
    int temp_pos;
    if ((temp_pos = filename.rfind(".temp")) != std::string::npos){
        filename_without_temp = filename.substr(0, temp_pos);
    }
    return std::make_unique<StringExprAST>(filename_without_temp);
}

std::unique_ptr<StringExprAST> stringify_macro(std::unique_ptr<ExprAST> expr){
    return std::make_unique<StringExprAST>(expr->to_string());
}

std::unique_ptr<ExprAST> generate_expect(std::vector<std::unique_ptr<ExprAST>>& ArgsMacro){
    std::vector<std::pair<std::string, Cpoint_Type>> args_expect;
    args_expect.push_back(std::make_pair("expression", Cpoint_Type(double_type)));
    args_expect.push_back(std::make_pair("filename", Cpoint_Type(i8_type, true)));
    args_expect.push_back(std::make_pair("function", Cpoint_Type(i8_type, true)));
    args_expect.push_back(std::make_pair("expr_str", Cpoint_Type(i8_type, true)));
    add_manually_extern("expectxplusexpr", Cpoint_Type(void_type), std::move(args_expect), 0, 30, false, false, "");
    std::vector<std::unique_ptr<ExprAST>> Args;
    std::vector<std::unique_ptr<ExprAST>> ArgsEmpty;
    if (ArgsMacro.size() != 1){
        return LogError("Wrong number of args for %s macro function call : expected %d, got %d", "expect", 1, ArgsMacro.size());
    }
    Args.push_back(ArgsMacro.at(0)->clone());
    Args.push_back(get_filename_tok());
    Args.push_back(std::make_unique<CallExprAST>(emptyLoc, "cpoint_internal_get_function_name", std::move(ArgsEmpty), Cpoint_Type(double_type)));
    Args.push_back(stringify_macro(ArgsMacro.at(0)->clone()));
    return std::make_unique<CallExprAST>(emptyLoc, "expectxplusexpr", std::move(Args), Cpoint_Type(double_type));
}

std::unique_ptr<ExprAST> generate_panic(std::vector<std::unique_ptr<ExprAST>>& ArgsMacro){
    std::vector<std::pair<std::string, Cpoint_Type>> args_panic;
    args_panic.push_back(std::make_pair("string", Cpoint_Type(i8_type, true)));
    args_panic.push_back(std::make_pair("filename", Cpoint_Type(i8_type, true)));
    args_panic.push_back(std::make_pair("function", Cpoint_Type(i8_type, true)));
    add_manually_extern("panicx", Cpoint_Type(void_type), std::move(args_panic), 0, 30, false, false, "");
    std::vector<std::unique_ptr<ExprAST>> Args;
    std::vector<std::unique_ptr<ExprAST>> ArgsEmpty;
    if (ArgsMacro.size() != 1){
        return LogError("Wrong number of args for %s macro function call : expected %d, got %d", "expect", 1, ArgsMacro.size());
    }
    Args.push_back(ArgsMacro.at(0)->clone());
    Args.push_back(get_filename_tok());
    Args.push_back(std::make_unique<CallExprAST>(emptyLoc, "cpoint_internal_get_function_name", std::move(ArgsEmpty), Cpoint_Type(double_type)));
    return std::make_unique<CallExprAST>(emptyLoc, "panicx", std::move(Args), Cpoint_Type(double_type));
}

std::unique_ptr<StringExprAST> generate_time_macro(){
    std::string time_str;
    char* time_str_c = (char*)malloc(sizeof(char) * 21); // TODO maybe increase this ? or think about if it is the right number
    std::time_t current_time;
    std::tm* time_info;
    time(&current_time);
    time_info = localtime(&current_time);
    strftime(time_str_c, 21, "%H:%M:%S", time_info);
    time_str = time_str_c;
    free(time_str_c);
    return std::make_unique<StringExprAST>(time_str);
}

std::unique_ptr<ExprAST> generate_env_macro(std::vector<std::unique_ptr<ExprAST>>& ArgsMacro){
    if (ArgsMacro.size() != 1){
        return LogError("Wrong number of args for %s macro function call : expected %d, got %d", "env", 1, ArgsMacro.size());
    }
    StringExprAST* str = nullptr;
    if (dynamic_cast<StringExprAST*>(ArgsMacro.at(0).get())){
        str = dynamic_cast<StringExprAST*>(ArgsMacro.at(0).get());
    } else {
        return LogError("Wrong type of args for %s macro function call : expected a string", "env");
    }
    std::string env_name = str->str;
    auto path_val = std::getenv(env_name.c_str());
    if (path_val == nullptr){
        return LogError("Env variable %s doesn't exist for %s macro function call", env_name.c_str(), "env");
    }
    return std::make_unique<StringExprAST>((std::string)path_val);
}

std::unique_ptr<StringExprAST> generate_concat_macro(std::vector<std::unique_ptr<ExprAST>>& ArgsMacro){
    std::string concatenated_str = "";
    for (int i = 0; i < ArgsMacro.size(); i++){
        if (dynamic_cast<StringExprAST*>(ArgsMacro.at(i).get())){
            auto s = dynamic_cast<StringExprAST*>(ArgsMacro.at(i).get());
            concatenated_str += s->str;
        } else {
            concatenated_str += ArgsMacro.at(i)->to_string();
        }
    }
    return std::make_unique<StringExprAST>(concatenated_str);
}

std::unique_ptr<ExprAST> generate_asm_macro(std::unique_ptr<ArgsInlineAsm> ArgsMacro){
    return std::make_unique<AsmExprAST>(std::move(ArgsMacro));
}

std::unique_ptr<ExprAST> generate_todo_macro(std::vector<std::unique_ptr<ExprAST>>& ArgsMacro){
    if (ArgsMacro.size() > 2){
        return LogError("Wrong number of args for %s macro function call : expected less or equal to %d, got %d", "todo", 2, ArgsMacro.size());
    }
    std::vector<std::unique_ptr<ExprAST>> Args;
    std::string basic_message = "not implemented";
    if (ArgsMacro.empty()){
        Args.push_back(std::make_unique<StringExprAST>(basic_message + "\n"));
    } else {
        if (!dynamic_cast<StringExprAST*>(Args.at(0).get())){
            return LogError("Expected a string arg in the todo macro");
        }
        std::unique_ptr<StringExprAST> arg_expr = get_Expr_from_ExprAST<StringExprAST>(std::move(Args.at(0)));
        std::string custom_message = arg_expr->str;
        Args.push_back(std::make_unique<StringExprAST>(basic_message + " : " + custom_message));
    }
    return std::make_unique<CallExprAST>(emptyLoc, "printf", std::move(Args), Cpoint_Type());
}

std::unique_ptr<ExprAST> generate_dbg_macro(std::vector<std::unique_ptr<ExprAST>>& ArgsMacro){
    if (ArgsMacro.size() != 1){
        return LogError("Wrong number of args for %s macro function call : expected %d, got %d", "dbg", 1, ArgsMacro.size());
    }
    std::vector<std::unique_ptr<ExprAST>> Args;
    Args.push_back(std::move(ArgsMacro.at(0)));
    return std::make_unique<CallExprAST>(emptyLoc, "cpoint_internal_dbg", std::move(Args), Cpoint_Type());
}

void add_manually_extern(std::string fnName, Cpoint_Type cpoint_type, std::vector<std::pair<std::string, Cpoint_Type>> ArgNames, unsigned Kind, unsigned BinaryPrecedence, bool is_variable_number_args, bool has_template, std::string TemplateName);

std::unique_ptr<ExprAST> generate_print_macro(std::vector<std::unique_ptr<ExprAST>>& ArgsMacro, bool is_println, bool is_error){
    // call internal function
    std::vector<std::pair<std::string, Cpoint_Type>> args_printf;
    Cpoint_Type stderr_type = Cpoint_Type(i32_type, true);
    if (is_error){
        // maybe create a va_arg function in standard library that can simplify this and remove the need to pass stderr ?
        GlobalVariableAST("stderr", false, true, stderr_type, nullptr, false, nullptr, "").codegen();
        args_printf.push_back(std::make_pair("file", stderr_type));
    }
    args_printf.push_back(std::make_pair("format", Cpoint_Type(i8_type, true)));
    std::string function_name = "printf";
    if (is_error){
        function_name = "fprintf";
    }
    add_manually_extern(function_name, Cpoint_Type(i32_type), std::move(args_printf), 0, 30, true, false, "");
    std::vector<std::unique_ptr<ExprAST>> Args = clone_vector<ExprAST>(ArgsMacro);
    if (!dynamic_cast<StringExprAST*>(Args.at(0).get())){
        return LogError("First argument of the print macro is not a constant string");
    }
    if (is_println){
        auto stringExpr = dynamic_cast<StringExprAST*>(Args.at(0).get());
        stringExpr->str += "\n";
    }

    if (is_error){
        Args.insert(Args.begin(), std::make_unique<VariableExprAST>(emptyLoc, "stderr", stderr_type));
    }

    return std::make_unique<CallExprAST>(emptyLoc, "cpoint_internal_print", std::move(Args), Cpoint_Type());
}

std::unique_ptr<ExprAST> generate_unreachable_macro(){
    // call internal function
    std::vector<std::unique_ptr<ExprAST>> Args;
    return std::make_unique<CallExprAST>(emptyLoc, "cpoint_internal_unreachable", std::move(Args), Cpoint_Type());
}

std::unique_ptr<ExprAST> generate_assume_macro(std::vector<std::unique_ptr<ExprAST>>& ArgsMacro){
    if (ArgsMacro.size() != 1){
        return LogError("Wrong number of args for %s macro function call : expected %d, got %d", "assume", 1, ArgsMacro.size());
    }
    std::vector<std::unique_ptr<ExprAST>> Args = clone_vector<ExprAST>(ArgsMacro);
    return std::make_unique<CallExprAST>(emptyLoc, "cpoint_internal_assume", std::move(Args), Cpoint_Type());
}