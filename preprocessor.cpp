#include "preprocessor.h"

void preprocess(std::string str){
    std::string instruction;
    if (str.at(0) == '?' && str.at(1) == '['){
    for (int i = 2; i < str.size() && str.at(i) != ']' ; i++){
    instruction += str.at(i);
    }
    std::cout << "instruction : " << instruction << std::endl;
    }
}
