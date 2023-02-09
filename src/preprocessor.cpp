#include "preprocessor.h"
//#include <bits/stdc++.h>
#include <sstream>
#include "target-triplet.h"
#include "lexer.h"
#include "log.h"

std::unique_ptr<Preprocessor::Context> context;
std::string word;
std::stringstream wordstrtream;
static std::string line;

void init_context_preprocessor(){
    std::vector<std::unique_ptr<Preprocessor::Variable>> variables;
    context = std::make_unique<Preprocessor::Context>(std::move(variables));
}

void setup_preprocessor(std::string target_triplet){
    context->variables.push_back(std::make_unique<Preprocessor::Variable>("OS", get_os(target_triplet)));
    context->variables.push_back(std::make_unique<Preprocessor::Variable>("ARCH", get_arch(target_triplet)));
    context->variables.push_back(std::make_unique<Preprocessor::Variable>("VENDOR", get_vendor(target_triplet)));
    
}


static void skip_spaces(std::string line, int& pos){
    while (pos < line.size() && isspace(line.at(pos))){
        pos++;
    }
}

int get_next_word(std::string line, int& pos){
    word = "";
    while (pos < line.size() && (isalnum(line.at(pos)) || line.at(pos) == '=')){
        word += line.at(pos);
        pos++;
    }
    skip_spaces(line, pos);
    return 0;
}

void preprocess_instruction(std::string str){
    std::string instruction;
    int pos_line = 0;
    skip_spaces(line, pos_line);
    for (int i = pos_line + 2; i < str.size() && str.at(i) != ']' ; i++){
    instruction += str.at(i);
    }
    int pos = 0;
    Log::Preprocessor_Info() << "instruction : " << instruction << "\n";
    get_next_word(instruction, pos);
    if (word == "if"){
        get_next_word(instruction, pos);
        std::string r = word;
        get_next_word(instruction, pos);
        std::string op = word;
        get_next_word(instruction, pos);
        std::string l = word;
        int pos = context->get_variable_pos(r);
        if (pos == -1){
            fprintf(stderr, "PREPROCESSOR : unknown variable\n");
        } else {
            if (op == "=="){
            if (context->get_variable_value(r) == l){
                Log::Preprocessor_Info() << "if true" << "\n";
                //go_to_next_line();
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

void preprocess_replace_variable(std::string& str){
    if (context == nullptr){
        std::cout << "CONTEXT IS NULL" << std::endl;
    }
    context->replace_variable_preprocessor(str);
}
