#include "preprocessor.h"

std::unique_ptr<Preprocessor::Context> context;

void setup_preprocessor(){
    std::vector<std::unique_ptr<Preprocessor::Variable>> variables;
    context = std::make_unique<Preprocessor::Context>(std::move(variables));
}

void preprocess(std::string str){
    std::string instruction;
    if (str.at(0) == '?' && str.at(1) == '['){
    for (int i = 2; i < str.size() && str.at(i) != ']' ; i++){
    instruction += str.at(i);
    }
    std::cout << "instruction : " << instruction << std::endl;
    } else {
        context->replace_variable_preprocessor(str);
    }
}