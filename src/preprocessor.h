#include <vector>
#include <iostream>
#include <memory>
//#include <bits/stdc++.h>
#include "log.h"

namespace Preprocessor {

class Variable {
    std::string name;
    std::string value;
public:
    Variable(const std::string &name, const std::string value) : name(name), value(value) {}
    std::string getName() const { return name; }
    std::string getValue() const { return value; }
};


class Context {
public:
    std::vector<std::unique_ptr<Variable>> variables;
    Context(std::vector<std::unique_ptr<Variable>> variables) : variables(std::move(variables)) {}
    void add_variable(std::unique_ptr<Variable> var){
        variables.push_back(std::move(var));
    }
    void remove_variable(std::string name){
        for (int i = 0; i < variables.size(); i++){
            if (variables.at(i)->getName() == name){
                variables.erase(variables.begin() + i);
            }
        }
    }
    int get_variable_pos(std::string name){
        for (int i = 0; i < variables.size(); i++){
            if (variables.at(i)->getName() == name){
                return i;
            }
        }
        return -1;
    }
    std::string get_variable_value(std::string name){
        for (int i = 0; i < variables.size(); i++){
            if (variables.at(i)->getName() == name){
                return variables.at(i)->getValue();
            }
        }
        return "";
    }
    void replace_variable_preprocessor(std::string& str){
        std::string variable_name;
        Log::Preprocessor_Info() << "REPLACING VARIABLES" << "\n";
        for (int i = 0; i < variables.size(); i++){
            if (variables.at(i) != nullptr){
            variable_name = variables.at(i)->getName();
            size_t pos = 0;
            while (pos != std::string::npos){
            pos = str.find(variable_name, pos);
            //std::cout << "TEST PREPROCESSOR VAR" << std::endl;
            if (pos != std::string::npos){
                str.replace(pos, variable_name.length(), variables.at(i)->getValue());
            }
            }
            }
        }
    }
};

}

void setup_preprocessor(std::string target_triplet);
void preprocess_instruction(std::string str);
void preprocess_replace_variable(std::string& str);
void init_context_preprocessor();
