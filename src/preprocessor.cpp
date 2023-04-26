#include "preprocessor.h"
//#include <bits/stdc++.h>
#include <sstream>
#include "target-triplet.h"
#include "lexer.h"
#include "log.h"

std::unique_ptr<Preprocessor::Context> context;
std::string word;
std::stringstream wordstrtream;
//static std::string line;

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
    while (pos < line.size() && (isalnum(line.at(pos)) || line.at(pos) == '=' || line.at(pos) == '\"' || line.at(pos) == '_')){
        word += line.at(pos);
        pos++;
    }
    skip_spaces(line, pos);
    return 0;
}

bool compare_line(std::string line, std::string pattern){
    int pos_cmp = 0;
    while (isspace(line.at(pos_cmp))){
        pos_cmp++;
    }
    int pos_cmp2 = line.size()-1;
    while (isspace(line.at(pos_cmp2))){
        pos_cmp2--;
    }
    std::string without_space_line = line.substr(pos_cmp, pos_cmp2+1);
    Log::Preprocessor_Info() << "without space line for comparison : " << without_space_line << "\n";
    if (without_space_line == pattern){
        Log::Preprocessor_Info() << "comparison is true" << "\n";
    }
    return without_space_line == pattern;
}

void preprocess_instruction(std::string line){
    Log::Info() << "LINE : " << line << "\n";
    std::string instruction;
    int pos_line = 0;
    skip_spaces(line, pos_line);
    for (int i = pos_line + 2; i < line.size() && line.at(i) != ']' ; i++){
    instruction += line.at(i);
    }
    int pos = 0;
    Log::Preprocessor_Info() << "instruction : " << instruction << "\n";
    get_next_word(instruction, pos);
    if (word == "if"){
        get_next_word(instruction, pos);
        std::string l = word;
        get_next_word(instruction, pos);
        std::string op = word;
        get_next_word(instruction, pos);
        std::string r = word;
        int pos = context->get_variable_pos(l);
        if (pos == -1){
            fprintf(stderr, "PREPROCESSOR : unknown variable %s\n", l.c_str());
        } else {
            if (op == "=="){
            if (context->get_variable_value(l) == r){
                Log::Preprocessor_Info() << "if true" << "\n";
                //go_to_next_line();
            } else {
                Log::Preprocessor_Info() << "if false" << "\n";
                while (!compare_line(get_line_returned(), "?[endif]")){
                    Log::Preprocessor_Info() << "line passed " << get_line_returned() << "\n";
                    go_to_next_line();
                }
            }
            } else {
                fprintf(stderr, "PREPROCESSOR : unknown operator");
            }
        }

    } else if (word == "endif"){
        Log::Preprocessor_Info() << "endif" << "\n";
    } else if (word == "define"){
        get_next_word(instruction, pos);
        std::string varName = word;
        Log::Preprocessor_Info() << "varName : " << varName << "\n";
        get_next_word(instruction, pos);
        std::string value = word;
        Log::Preprocessor_Info() << "value : " << value << "\n";
        context->add_variable(std::make_unique<Preprocessor::Variable>(varName, value));
        Log::Preprocessor_Info() << "Number of variables in context : " << context->variables.size() << "\n";
    } else if (word == "undefine"){
        get_next_word(instruction, pos);
        std::string varName = word;
        context->remove_variable(varName);
    } else if (word == "warning"){
        std::string warning = "";
        get_next_word(instruction, pos);
        while (word != ""){
            warning += (" " + word);
            get_next_word(instruction, pos);
        }
        Log::Warning() << warning << "\n";
    } else if (word == "error"){
        std::string error = "";
        get_next_word(instruction, pos);
        while (word != ""){
            error += (" " + word);
            get_next_word(instruction, pos);
        }
        Log::Preprocessor_Error() << error << "\n";
        exit(1);
    }

}

void preprocess_replace_variable(std::string& str){
    if (context == nullptr){
        std::cout << "CONTEXT IS NULL" << std::endl;
    }
    context->replace_variable_preprocessor(str);
}
