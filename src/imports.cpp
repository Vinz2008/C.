#include "imports.h"
#include <iostream>
#include <fstream>
#include <cstdarg>
#include <memory>
#include <vector>
#include <sstream>
#include "errors.h"
#include "log.h"
#include "packages.h"
#include "types.h"
#include "preprocessor.h"
#include "config.h"

#ifdef _WIN32
#define realpath(N,R) _fullpath((R),(N),PATH_MAX)
#endif

static std::string IdentifierStr;
static std::string line;
std::ofstream out_file;
extern std::string std_path;
extern std::string filename;
int pos_line_file = 0;

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

static void skip_spaces(std::string line, int& pos){
    while (pos < line.size() && isspace(line.at(pos))){
        pos++;
    }
}
void getIdentifierStr(std::string line, int& pos, std::string &IdentifierStr){
    IdentifierStr = "";
    while (pos < line.size() && (isalnum(line.at(pos)) || line.at(pos) == '_')){
        IdentifierStr += line.at(pos);
        pos++;
    }
    skip_spaces(line, pos);
}

int getPath(std::string line, int& pos, std::string &Path){
    Path = "";
    while (pos < line.size() && (isalnum(line.at(pos)) || line.at(pos) == '/' || line.at(pos) == '_' || line.at(pos) == '.' || line.at(pos) == '@' || line.at(pos) == ':' || isdigit(line.at(pos)))){
        Path += line.at(pos);
        pos++;
    }
    if (Path.at(0) == '@'){
    	int pos_path = 1;
    	getIdentifierStr(Path, pos_path, IdentifierStr);
        Log::Imports_Info() << "IdentifierStr : " << IdentifierStr << "\n";
        std::string Path_temp;
	    if (IdentifierStr == "std"){
	    //pos_path += IdentifierStr.size();
	    Path_temp = std_path;
        } else if (IdentifierStr == "github"){
            pos_path += 1; // pass ':'
            getIdentifierStr(Path, pos_path, IdentifierStr);
            std::string username = IdentifierStr;
            pos_path += 1; // pass ':'
            getIdentifierStr(Path, pos_path, IdentifierStr);
            std::string reponame = IdentifierStr;
            download_package_github(username, reponame);
            add_package(reponame);
            //Log::Imports_Info() << "TEST" << "\n";
            Path_temp = DEFAULT_PACKAGE_PATH;
            Path_temp.append("/");
            Path_temp.append(reponame);
            build_package(Path_temp);
        } else {
            import_error("couldn't find after @ a normal import\n");
        }
        Log::Imports_Info() << "Path_temp : " << Path_temp << "\n";
        std::string end_str = Path.substr(pos_path, Path.size());
        Log::Imports_Info() << "end_str : " << end_str << "\n";
        Path_temp.append(end_str);
	    Path = Path_temp;
        Log::Imports_Info() << "Path : " << Path << "\n";
        return 1;
    }
    return 0;
}

void getPathFromFilePOV(std::string& Path, std::string file_src){
    Log::Imports_Info() << "file_src : " << file_src << "\n";
    Log::Imports_Info() << "Path : " << Path << "\n";
    int pos_last_slash = 0;
    for (int i = file_src.size() -1; i > 0; i--){
        if (file_src.at(i) == '/'){
            pos_last_slash = i;
            break;
        }
    }
    std::string Path_Temp = "";
    if (pos_last_slash != 0){
    std::string folder_src_path = file_src.substr(0, pos_last_slash+1);
    Path_Temp.append(folder_src_path);
    Log::Imports_Info() << "folder_src_path : " << folder_src_path << "\n";
    }
    Path_Temp.append(Path);
    Path = Path_Temp;
    Log::Imports_Info() << "TEST " << Path << "\n";
    Path = realpath(Path.c_str(), NULL);
    Log::Imports_Info() << "Path after serialization : " << Path << "\n";
}

void interpret_func(std::string line, int& pos, int nb_line, int& pos_line){
    std::string declar = "";
    for (int i = pos; i < line.size(); i++){
        if (line.at(i) != '{'){
        declar += line.at(i);
        }
    }
    Log::Imports_Info() << "Declar : " << declar << "\n";
    Log::Imports_Info() << "pos_line : " << pos_line << "\n";
    if (pos_line != 0 && pos_line != nb_line){
        out_file << "\n";
    }
    out_file << "extern " << declar << ";";
    pos_line++;
}

void find_func(std::string line, int nb_line, int& pos_line){
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
    if (getPath(line, pos_src, Path) == 0){
    getPathFromFilePOV(Path, filename);
    }
    Log::Imports_Info() << "Path : " << Path << "\n";
    int nb_line = get_nb_lines(imported_file, Path);
    imported_file.open(Path);
    if (imported_file.is_open()){
        int pos_line = 0;
        while (std::getline(imported_file, line)){
            Log::Imports_Info() << line << "\n";
            find_func(line, nb_line, pos_line);
        }
    }
    imported_file.close();
}

void interpret_include(std::string line, int& pos_src){
    std::string Path;
    std::ifstream included_file;
    skip_spaces(line, pos_src);
    getPath(line, pos_src, Path);
    getPathFromFilePOV(Path, filename);
    Log::Imports_Info() << "Path : " << Path << "\n";
    int nb_line = get_nb_lines(included_file, Path);
    included_file.open(Path);
    if (included_file.is_open()){
        int pos_line = 0;
        while (std::getline(included_file, line)){
            Log::Imports_Info() << line << "\n";
            Log::Imports_Info() << "pos_line : " << pos_line << "\n";
            out_file << line;
            if (pos_line != nb_line - 1){
                out_file << "\n";
            }
            pos_line++;
        }
    }
    included_file.close();
}

int find_import_or_include(std::string line){
    int pos_src = 0;
    skip_spaces(line, pos_src);
    getIdentifierStr(line, pos_src, IdentifierStr);
    Log::Imports_Info() << "IdentifierStr : " << IdentifierStr << "\n";
    if (IdentifierStr == "import"){
        interpret_import(line, pos_src);
        return 1;
    } if (IdentifierStr == "include"){
        interpret_include(line, pos_src);
        return 1;
    } else {

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
        //int pos_line = 0;
        while (std::getline(file_code, line)){
            if (pos_line_file != nb_line && pos_line_file != 0){
                out_file << "\n";
            }
            Log::Imports_Info() << "line : " << line << "\n";
            if (find_import_or_include(line) == 0){
                preprocess_replace_variable(line);
                out_file << line;
            }
            pos_line_file++;
        }
    }
    file_code.close();
    out_file.close();
}
