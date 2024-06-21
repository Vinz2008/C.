#include "vars.h"
#include <unordered_map>
#include <memory>
#include "codegen.h"

extern std::unordered_map<std::string, std::unique_ptr<NamedValue>> NamedValues;
extern std::unordered_map<std::string, std::unique_ptr<GlobalVariableValue>> GlobalVariables;

bool is_var_local(std::string name){
    if (NamedValues[name]){
        return true;
    } else if (GlobalVariables[name]){
        return false;
    } else {
        return false;
    }
}

bool is_var_global(std::string name){
    if (NamedValues[name]){
        return false;
    } else if (GlobalVariables[name]){
        return true;
    } else {
        return false;
    }
}

Cpoint_Type* get_variable_type(std::string name){
    if (NamedValues[name]){ 
        return &NamedValues[name]->type;
    } else if (GlobalVariables[name]) { 
        return &GlobalVariables[name]->type;
    } else {
        return nullptr;
    }
}

bool var_exists(std::string name){
    return (GlobalVariables[name] != nullptr || NamedValues[name] != nullptr);
}

// to get either AllocaInst or GlobalVariable
Value* get_var_allocation(std::string name){
    if (NamedValues[name]){
        return NamedValues[name]->alloca_inst;
    } else if (GlobalVariables[name]){
        return GlobalVariables[name]->globalVar;
    } else {
        return nullptr;
    }
}