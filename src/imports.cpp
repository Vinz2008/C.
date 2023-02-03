#include "imports.h"
#include <iostream>
#include <fstream>
#include <cstdarg>
#include <memory>
#include <vector>
#include <sstream>
#include "errors.h"
#include "log.h"
#include "types.h"

static std::string IdentifierStr;
static std::string line;
std::ofstream out_file;

class ArgVarImport {
    std::string name;
    int type;
public:
    ArgVarImport(const std::string &name, int type) : name(name), type(type) {}
};

void import_error(const char* format, ...){
    std::va_list args;
    va_start(args, format);
    fprintf(stderr, "\e[0;31m" /*red*/ "Error in imports\n" /*reset*/ "\e[0m");
    vfprintf(stderr, format, args);
    fprintf(stderr, "\n");
    fprintf(stderr, "%s\n", line.c_str());
    va_end(args);
    exit(1);
}

int get_nb_lines(std::ifstream& file_code, std::string filename){
    int numLines = 0;
    file_code.open(filename);
    std::string unused;
    while ( std::getline(file_code, unused) )
        ++numLines;
    file_code.close();
    return numLines;
}

void skip_spaces(std::string line, int& pos){
    while (pos < line.size() && isspace(line.at(pos))){
        pos++;
    }
}
void getIdentifierStr(std::string line, int& pos, std::string &IdentifierStr){
    IdentifierStr = "";
    while (pos < line.size() && isalnum(line.at(pos))){
        IdentifierStr += line.at(pos);
        pos++;
    }
    skip_spaces(line, pos);
}

void getPath(std::string line, int& pos, std::string &Path){
    Path = "";
    while (pos < line.size() && (isalnum(line.at(pos)) || line.at(pos) == '/' || line.at(pos) == '_' || line.at(pos) == '.' || isdigit(line.at(pos)))){
        Path += line.at(pos);
        pos++;
    }
}

void interpret_func(std::string line, int& pos, int nb_line, int pos_line){
    std::string declar = "";
    for (int i = pos; i < line.size(); i++){
        if (line.at(i) != '{'){
        declar += line.at(i);
        }
    }
    Log::Imports_Info() << "Declar : " << declar << "\n";
    if (pos_line != 0 && pos_line != nb_line){
        out_file << "\n";
    }
    out_file << "extern " << declar << ";";
}

void find_func(std::string line, int nb_line, int pos_line){
    int pos = 0;
    skip_spaces(line, pos);
    getIdentifierStr(line, pos, IdentifierStr);
    Log::Imports_Info() << "IdentifierStr : " << IdentifierStr << "\n";
    if (IdentifierStr == "func"){
        interpret_func(line, pos, nb_line, pos_line);
    }
}

void interpret_import(std::string line, int& pos_src){
    std::string Path;
    std::ifstream imported_file;
    skip_spaces(line, pos_src);
    getPath(line, pos_src, Path);
    Log::Imports_Info() << "Path : " << Path << "\n";
    int nb_line = get_nb_lines(imported_file, Path);
    imported_file.open(Path);
    if (imported_file.is_open()){
        int pos_line = 0;
        while (std::getline(imported_file, line)){
            Log::Imports_Info() << line << "\n";
            find_func(line, nb_line, pos_line);
            pos_line++;
        }
    }
    imported_file.close();
}

int find_import(std::string line){
    int pos_src = 0;
    skip_spaces(line, pos_src);
    getIdentifierStr(line, pos_src, IdentifierStr);
    Log::Imports_Info() << "IdentifierStr : " << IdentifierStr << "\n";
    if (IdentifierStr == "import"){
        interpret_import(line, pos_src);
        return 1;
    }
    return 0;
}

void generate_file_with_imports(std::string file_path, std::string out_path){
    std::ifstream file_code;
    std::string line;
    int nb_line = get_nb_lines(file_code, file_path);
    file_code.open(file_path);
    out_file.open(out_path);

    if (file_code.is_open()){
        int pos_line = 0;
        while (std::getline(file_code, line)){
            if (pos_line != nb_line && pos_line != 0){
                out_file << "\n";
            }
            Log::Imports_Info() << "line : " << line << "\n";
            if (find_import(line) == 0){
                out_file << line;
            }
            pos_line++;
        }
    }
    file_code.close();
    out_file.close();
}