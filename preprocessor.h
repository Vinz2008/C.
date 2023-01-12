#include <vector>
#include <iostream>

namespace preprocessor {

class Variable {
    std::string name;
    std::string value;
public:
    Variable(const std::string &name, const std::string value) : name(name), value(value) {}
};


class Context {
    std::vector<std::unique_ptr<Variable>> variables;
};

}
