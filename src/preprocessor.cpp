#include "preprocessor.h"
//#include <bits/stdc++.h>
#include <sstream>
#include <fstream>
#include "targets/target-triplet.h"
#include "lexer.h"
#include "log.h"
//#include "ast.h"

std::unique_ptr<Preprocessor::Context> context;
static std::string word;
static std::stringstream wordstrtream;

namespace Preprocessor {
    void Context::add_variable(std::unique_ptr<Variable> var){
        variables.push_back(std::move(var));
    }
    void Context::remove_variable(std::string name){
        for (int i = 0; i < variables.size(); i++){
            if (variables.at(i)->getName() == name){
                variables.erase(variables.begin() + i);
            }
        }
    }
    int Context::get_variable_pos(std::string name){
        for (int i = 0; i < variables.size(); i++){
            if (variables.at(i)->getName() == name){
                return i;
            }
        }
        return -1;
    }
    std::string Context::get_variable_value(std::string name){
        for (int i = 0; i < variables.size(); i++){
            if (variables.at(i)->getName() == name){
                return variables.at(i)->getValue();
            }
        }
        return "";
    }
    void Context::replace_variable_preprocessor(std::string& str){
        std::string variable_name;
        for (int i = 0; i < variables.size(); i++){
            if (variables.at(i) != nullptr){
            variable_name = variables.at(i)->getName();
            size_t pos = 0;
            while (pos != std::string::npos){
                pos = str.find(variable_name, pos);
                if (pos != std::string::npos){
                    Log::Preprocessor_Info() << "REPLACING VARIABLE" << "\n";
                    str.replace(pos, variable_name.length(), variables.at(i)->getValue());
                }
            }
            }
        }
    }
}

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

static int get_next_word(std::string line, int& pos){
    word = "";
    while (pos < line.size() && (isalnum(line.at(pos)) || line.at(pos) == '=' || line.at(pos) == '\"' || line.at(pos) == '_')){
        word += line.at(pos);
        pos++;
    }
    skip_spaces(line, pos);
    return 0;
}

static bool compare_line(std::string line, std::string pattern){
    int pos_cmp = 0;
    int pos_cmp2 = 0;
    if (line.size() > 0){
    while (isspace(line.at(pos_cmp))){
        pos_cmp++;
    }
    pos_cmp2 = line.size()-1;
    while (isspace(line.at(pos_cmp2))){
        pos_cmp2--;
    }
    }
    std::string without_space_line = line.substr(pos_cmp, pos_cmp2+1);
    Log::Preprocessor_Info() << "without space line for comparison : " << without_space_line << "\n";
    if (without_space_line == pattern){
        Log::Preprocessor_Info() << "comparison is true" << "\n";
    }
    return without_space_line == pattern;
}

// TODO : refactor this code with a vector of expressions (with in the the next operator or empty string if it is the end of the expression) or a simple AST
static void preprocess_if(std::string instruction, int& pos, std::ifstream& file_code, int& pos_line_file){
    std::string l2 = "";
    std::string op2 = "";
    std::string r2 = "";
    std::string operator_in_between = "";
    //bool found_extra_expr = false;
    get_next_word(instruction, pos);
    std::string l = word;
    get_next_word(instruction, pos);
    std::string op = word;
    get_next_word(instruction, pos);
    std::string r = word;
    get_next_word(instruction, pos);
    if (word != ""){
        operator_in_between = word;
        get_next_word(instruction, pos);
        l2 = word;
        get_next_word(instruction, pos);
        op2 = word;
        get_next_word(instruction, pos);
        r2 = word;
        get_next_word(instruction, pos);
    }
    int pos_var = context->get_variable_pos(l);
    if (pos_var == -1){
        fprintf(stderr, "PREPROCESSOR : unknown variable %s\n", l.c_str());
    } else {
        if (op == "=="){
        bool is_if_true = false;
        is_if_true = context->get_variable_value(l) == r;
        if (operator_in_between != ""){
            if (operator_in_between == "or"){
                bool is_if_true2 = false;
                if (op2 == "=="){
                    is_if_true2 = context->get_variable_value(l2) == r2;
                } else {
                    fprintf(stderr, "PREPROCESSOR : unknown operator '%s' in between expressions", op2.c_str());
                }
                is_if_true = (is_if_true || is_if_true2);
            } else {
                fprintf(stderr, "PREPROCESSOR : unknown operator '%s' in between expressions", op.c_str());
            }
        }
        
        if (is_if_true){
            Log::Preprocessor_Info() << "if true" << "\n";
        } else {
            Log::Preprocessor_Info() << "if false" << "\n";
            std::string line = "";
             while (!compare_line(/*get_line_returned()*/ line, "?[endif]")){
                Log::Preprocessor_Info() << "line passed " << get_line_returned() << "\n";
                /*go_to_next_line();*/ std::getline(file_code, line);
                pos_line_file++;
            }
        }
        } else {
            fprintf(stderr, "PREPROCESSOR : unknown operator");
        }
    }
}

void preprocess_instruction(std::string line, std::ifstream& file_code, int& pos_line_file){
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
        preprocess_if(instruction, pos, file_code, pos_line_file);
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
        Log::Warning(emptyLoc) << warning << "\n";
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
