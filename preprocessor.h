#include <vector>
#include <iostream>
#include <memory>

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
    std::vector<std::unique_ptr<Variable>> variables;
public:
    Context(std::vector<std::unique_ptr<Variable>> variables) : variables(std::move(variables)) {}
    void add_variable(std::unique_ptr<Variable> var){
        variables.push_back(std::move(var));
    }
    void replace_variable_preprocessor(std::string str){
        std::string variable_name;
        for (int i = 0; i < variables.size(); i++){
        variable_name = variables.at(i)->getName();
        size_t pos = str.find(variable_name);
        str.replace(pos, variable_name.length(), variables.at(i)->getValue());
        }
    }
};

}

void setup_preprocessor();
