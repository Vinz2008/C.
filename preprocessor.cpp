#include "preprocessor.h"
//#include <bits/stdc++.h>
#include <sstream>
#include "target-triplet.h"
#include "lexer.h"
#include "log.h"

std::unique_ptr<Preprocessor::Context> context;
std::string word;
std::stringstream wordstrtream;

void setup_preprocessor(std::string target_triplet){
    std::vector<std::unique_ptr<Preprocessor::Variable>> variables;
    variables.push_back(std::make_unique<Preprocessor::Variable>("os", get_os(target_triplet)));
    variables.push_back(std::make_unique<Preprocessor::Variable>("arch", get_arch(target_triplet)));
    variables.push_back(std::make_unique<Preprocessor::Variable>("vendor", get_vendor(target_triplet)));
    context = std::make_unique<Preprocessor::Context>(std::move(variables));
}

void init_parser(std::string str) {
    wordstrtream = std::stringstream(str);
}

int get_next_word(){
    if (!getline(wordstrtream, word, ' ')){
        return 1;
    }
    return 0;
}

void preprocess_instruction(std::string str){
    std::string instruction;
    for (int i = 2; i < str.size() && str.at(i) != ']' ; i++){
    instruction += str.at(i);
    }
    Log::Preprocessor_Info() << "instruction : " << instruction << "\n";
    init_parser(instruction);
    get_next_word();
    if (word == "if"){
        get_next_word();
        std::string r = word;
        get_next_word();
        std::string op = word;
        get_next_word();
        std::string l = word;
        int pos = context->get_variable_pos(r);
        if (pos == -1){
            fprintf(stderr, "PREPROCESSOR : unknown variable");
        } else {
            if (op == "=="){
            if (context->get_variable_value(r) == l){
                Log::Preprocessor_Info() << "if true" << "\n";
            } else {
                Log::Preprocessor_Info() << "if false" << "\n";
                while (get_line_returned() != "?[endif]"){
                    //std::cout << "line passed " << get_line_returned() << std::endl;
                    go_to_next_line();
                }
            }
            } else {
                fprintf(stderr, "PREPROCESSOR : unknown operator");
            }
        }

    } else if (word == "endif"){
        Log::Preprocessor_Info() << "endif" << "\n";
    }

}

void preprocess_replace_variable(std::string str){
    context->replace_variable_preprocessor(str);
}
