#include "builtin_macros.h"
#include <memory>
#include <ctime>

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

std::unique_ptr<ExprAST> generate_asm_macro(std::vector<std::unique_ptr<ExprAST>>& ArgsMacro){
    if (ArgsMacro.size() != 1){
        return LogError("Wrong number of args for %s macro function call : expected %d, got %d", "asm", 1, ArgsMacro.size());
    }
    StringExprAST* str = nullptr;
    if (dynamic_cast<StringExprAST*>(ArgsMacro.at(0).get())){
        str = dynamic_cast<StringExprAST*>(ArgsMacro.at(0).get());
    } else {
        return LogError("Wrong type of args for %s macro function call : expected a string", "asm");
    }
    std::string assembly_code = str->str;
    return std::make_unique<AsmExprAST>(assembly_code);
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
        //Args.push_back(std::make_unique<StringExprAST>(basic_message + " : "));
        return LogError("You can't use the todo macro with args for now"); // TODO
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

std::unique_ptr<ExprAST> generate_print_macro(std::vector<std::unique_ptr<ExprAST>>& ArgsMacro){
    // call internal function
    std::vector<std::unique_ptr<ExprAST>> Args = clone_vector<ExprAST>(ArgsMacro);
    return std::make_unique<CallExprAST>(emptyLoc, "cpoint_internal_print", std::move(Args), Cpoint_Type());
}
