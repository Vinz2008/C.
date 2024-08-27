#include <vector>
#include <iostream>
#include <memory>
//#include "log.h"

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
    void add_variable(std::unique_ptr<Variable> var);
    void remove_variable(std::string name);
    int get_variable_pos(std::string name);
    std::string get_variable_value(std::string name);
    void replace_variable_preprocessor(std::string& str);
};

}

void setup_preprocessor(std::string target_triplet);
void preprocess_instruction(std::string line, std::ifstream& file_code, int& pos_line_file);
void preprocess_replace_variable(std::string& str);
void init_context_preprocessor();