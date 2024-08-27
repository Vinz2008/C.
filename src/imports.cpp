#include "imports.h"
#include <iostream>
#include <fstream>
#include <cstdarg>
#include <memory>
#include <vector>
#include <sstream>
//#include "errors.h"
#include "log.h"
#include "packages.h"
#include "types.h"
#include "preprocessor.h"
#include "config.h"
#include "utils.h"
#include "prelude.h"

#ifdef _WIN32
#include "windows.h"
#endif

static std::string IdentifierStr;
static std::string line;
std::ofstream out_file;
extern std::string std_path;
extern std::string filename;
std::ifstream imported_file;
int pos_line_file = 0;

// will be negative
int modifier_for_line_count = 0; 

bool is_in_mod = false;

/*class ArgVarImport {
    std::string name;
    int type;
public:
    ArgVarImport(const std::string &name, int type) : name(name), type(type) {}
};*/

static void import_error(const char* format, ...){
    std::va_list args;
    va_start(args, format);
    fprintf(stderr, "\e[0;31m" /*red*/ "Error in imports\n" /*reset*/ "\e[0m");
    vfprintf(stderr, format, args);
    fprintf(stderr, "\n");
    fprintf(stderr, "%s\n", line.c_str());
    va_end(args);
    exit(1);
}

static int get_nb_lines(std::ifstream& file_code, std::string filename){
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
static void getIdentifierStr(std::string line, int& pos, std::string &IdentifierStr){
    IdentifierStr = "";
    while (pos < line.size() && (isalnum(line.at(pos)) || line.at(pos) == '_' || line.at(pos) == '-')){
        IdentifierStr += line.at(pos);
        pos++;
    }
    skip_spaces(line, pos);
}

static bool getPath(std::string line, int& pos, std::vector<std::string>& Paths){
    std::string Path = "";
    while (pos < line.size() && (isalnum(line.at(pos)) || line.at(pos) == '/' || line.at(pos) == '_' || line.at(pos) == '.' || line.at(pos) == '@' || line.at(pos) == ':' || line.at(pos) == '-' || isdigit(line.at(pos)))){
        Path += line.at(pos);
        pos++;
    }
    if (Path.at(0) == '@'){
    	int pos_path = 1;
    	getIdentifierStr(Path, pos_path, IdentifierStr);
        Log::Imports_Info() << "IdentifierStr : " << IdentifierStr << "\n";
        std::string Path_temp;
        bool importing_multiple_files = false;
        bool is_package = false;
	    if (IdentifierStr == "std"){
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
            Path_temp = DEFAULT_PACKAGE_PATH;
            Path_temp.append("/");
            Path_temp.append(reponame);
            build_package(Path_temp);
        } else {
            is_package = true;
            std::string path_repo = DEFAULT_PACKAGE_PATH;
            path_repo += '/';
            path_repo += IdentifierStr;
            Log::Imports_Info() << "if folder exists " << path_repo << "\n";
            if (FolderExists(path_repo)){
                Path_temp = path_repo;
                build_package(Path_temp);
            } else {
                import_error("couldn't find after @ a normal import\n");
            }
        }
        Log::Imports_Info() << "Path_temp : " << Path_temp << "\n";
        if (is_package && pos_path >= Path.size()-1){
            importing_multiple_files = true;
            std::string package_folder = Path_temp;
            listFiles(package_folder, [&Paths](std::string path){
                Log::Imports_Info() << "path " << path << "\n";
                if (getFileExtension(path) == "cpoint"){
                    Paths.push_back(path);
                }
            });
        } else {
        std::string end_str = Path.substr(pos_path, Path.size());
        Log::Imports_Info() << "end_str : " << end_str << "\n";
        Path_temp.append(end_str);
        }
        Path = Path_temp;
        Log::Imports_Info() << "Path : " << Path << "\n";
        if (!importing_multiple_files){
            Paths.push_back(Path);
        }
        return true;
    }
    Paths.push_back(Path);
    return false;
}

static void getPathFromFilePOV(std::string& Path, std::string file_src){
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
    //Log::Imports_Info() << "TEST " << Path << "\n";
    Path = realpath(Path.c_str(), NULL);
    Log::Imports_Info() << "Path after serialization : " << Path << "\n";
}

static std::string interpet_func_generics(std::string line, int pos, int nb_line, int& pos_line, bool should_write_to_file = true){
    std::string declar = "";
    for (int i = pos; i < line.size(); i++){
        if (line.at(i) != '{'){
        declar += line.at(i);
        }
    }
    if (pos_line != 0 && pos_line != nb_line){
        out_file << "\n";
    }
    declar = "func " + declar + "{" + "\n";
    pos_line++;
    modifier_for_line_count++;
    int in_func_nb_of_opened_braces = 1;
    while (1){
        Log::Imports_Info() << "declar in body func generic : " << declar << "\n";
        if (!std::getline(imported_file, line)){
            break;
        }
        pos_line++;
        declar += line + "\n";
        modifier_for_line_count++;
        if (line.find('{') != std::string::npos){
            in_func_nb_of_opened_braces++;
        }
        if (line.find('}') != std::string::npos){
            in_func_nb_of_opened_braces--;
            if (in_func_nb_of_opened_braces == 0){
                break;
            }
        }
    }
    if (should_write_to_file){
        out_file << declar;
    }
    return declar;
}

static std::string interpret_func(std::string line, int& pos, int nb_line, int& pos_line, bool should_write_to_file = true){
    if (line.find('~') != std::string::npos){
        return interpet_func_generics(line, pos, nb_line, pos_line, should_write_to_file);
    }
    
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
    if (should_write_to_file){
    out_file << "extern " << declar << ";";
    pos_line++;
    modifier_for_line_count++;
    }
    return declar;
}

void find_patterns(std::string line, int nb_line, int& pos_line, std::ifstream& file_code);

int nb_of_opened_braces_struct;

static std::string interpret_struct_generics(std::string line, int& pos, int nb_line, int& pos_line){
    std::string declar = "";
    for (int i = pos; i < line.size(); i++){
        if (line.at(i) != '{'){
        declar += line.at(i);
        }
    }
    declar += "{\n";
    pos_line++;
    modifier_for_line_count++;
    int in_func_nb_of_opened_braces = 1;
    while (1){
        Log::Imports_Info() << "declar in body func generic : " << declar << "\n";
        if (!std::getline(imported_file, line)){
            break;
        }
        pos_line++;
        declar += line + "\n";
        modifier_for_line_count++;
        if (line.find('{') != std::string::npos){
            in_func_nb_of_opened_braces++;
        }
        if (line.find('}') != std::string::npos){
            in_func_nb_of_opened_braces--;
            if (in_func_nb_of_opened_braces == 0){
                break;
            }
        }
    }
    return declar;
}

static void interpret_extern(std::string line, int& pos_line){
    out_file << line << "\n";
    pos_line++;
    modifier_for_line_count++;
}

static void interpret_struct(std::string line, int& pos, int nb_line, int& pos_line){
    if (line.find('~') != std::string::npos){
        out_file << "struct " << interpret_struct_generics(line, pos, nb_line, pos_line);
        return;
    }
    std::string struct_declar = "";
    nb_of_opened_braces_struct = 0;
    line = line.substr(pos, line.size()-1);
    while (true){
        if (line.find('{') != std::string::npos){
            Log::Imports_Info() << "found { in struct" << "\n";
            nb_of_opened_braces_struct++;
        }
        if (line.find('}') != std::string::npos){
            Log::Imports_Info() << "found } in struct" << "\n";
            nb_of_opened_braces_struct--;
            if (nb_of_opened_braces_struct == 0){
                Log::Imports_Info() << "break struct at line : " << line << "\n";
                struct_declar += line;
                break;
            }
        }
        int pos = 0;
        while (pos < line.size() && isspace(line.at(pos))){
            Log::Info() << "skip space char : " << (char)line.at(pos) << "\n";
            pos++;
        }
        getIdentifierStr(line, pos, IdentifierStr);
        Log::Info() << "IdentifierStr : " << IdentifierStr << "\n";
        if (IdentifierStr == "func"){
            Log::Info() << "pos : " << pos << "\n";
            Log::Imports_Info() << "found func in struct" << "\n";
            std::string declar = interpret_func(line, pos, nb_line, pos_line, false);
            struct_declar += "    extern " + declar + " ";
            int in_func_nb_of_opened_braces = 0;
            while (1){
                if (line.find('{') != std::string::npos){
                    in_func_nb_of_opened_braces++;
                }
                if (line.find('}') != std::string::npos){
                    in_func_nb_of_opened_braces--;
                    if (in_func_nb_of_opened_braces == 0){
                        break;
                    }
                }
                if (!std::getline(imported_file, line)){
                    break;
                }
                pos_line++;
            }
        } else if (IdentifierStr == "extern"){
            interpret_extern(line, pos_line);
        } else {
            struct_declar += line;
        }
        pos = 0;
        if (!std::getline(imported_file, line)){
            break;
        } else {
            pos_line++;
            struct_declar += '\n';
            modifier_for_line_count++;
        }
    }
    Log::Imports_Info() << "struct_declar : " << struct_declar << "\n";
    if (pos_line != 0 && pos_line != nb_line){
        out_file << "\n";
    }
    out_file << "struct " << struct_declar;
    pos_line++;
}

static void interpret_enum(std::string line, int& pos, int nb_line, int& pos_line){
    std::string enum_declar = "";
    while (true){
        for (int i = pos; i < line.size(); i++){
        if (line.at(i) != '}'){
            enum_declar += line.at(i);
        } else {
            enum_declar += line.at(i);
            goto after_while;
        }
        }
        pos = 0;
        if (!std::getline(imported_file, line)){
            break;
        } else {
            enum_declar += '\n';
            modifier_for_line_count++;
        }
    }
after_while:
    if (pos_line != 0 && pos_line != nb_line){
        out_file << "\n";
    }
    out_file << "enum " << enum_declar;
    pos_line++;
}

static void interpret_union(std::string line, int& pos, int nb_line, int& pos_line){
    std::string union_declar = "";
    while (true){
        for (int i = pos; i < line.size(); i++){
        if (line.at(i) != '}'){
            union_declar += line.at(i);
        } else {
            union_declar += line.at(i);
            goto after_while;
        }
        }
        pos = 0;
        if (!std::getline(imported_file, line)){
            break;
        } else {
            union_declar += '\n';
            modifier_for_line_count++;
        }
    }
after_while:
    if (pos_line != 0 && pos_line != nb_line){
        out_file << "\n";
    }
    out_file << "union " << union_declar;
    pos_line++;
}

void interpret_type(std::string line, int& pos_line){
    out_file << "\n" << line << "\n";
    pos_line++;
}

int nb_of_opened_braces = 0;
bool first_mod_opened = false;

static inline std::string& ltrim(std::string& s, const char* t = " \t\n\r\f\v"){
    s.erase(0, s.find_first_not_of(t));
    return s;
}


void interpret_mod(std::string line, int& pos, int nb_line, int& pos_line, std::ifstream &file_code){
    out_file << '\n' << line;
    pos_line++;
    int nb_of_opened_braces = 0;
    //bool is_in_first_mod = first_mod_opened;
    if (!first_mod_opened){
        first_mod_opened = true;
    } else {
        nb_of_opened_braces = 0;
    }
    while (std::getline(imported_file, line)){
        pos_line++;
        Log::Imports_Info() << line << "\n";
        if (ltrim(line).rfind("mod", 0) != 0){ // line startswith mod
        if (line.find('{') != std::string::npos){
            Log::Imports_Info() << "line mod contains {" << "\n";
            nb_of_opened_braces++;
        }
        if (line.find('}') != std::string::npos){
            nb_of_opened_braces--;
        }
        Log::Imports_Info() << "nb_of_opened_braces : " << nb_of_opened_braces << "\n";
        if (line.find("}") != std::string::npos && nb_of_opened_braces == -1){
            Log::Imports_Info() << "close mod block" << "\n";
            /*if  (!is_in_first_mod){
                out_file << ""
            }*/
            out_file << "\n" << line;
            modifier_for_line_count++;
            break;
        }
        }
        find_patterns(line, nb_line, pos_line, file_code);
    }
}

bool first_members_opened = false;
int nb_of_opened_braces_members;

void interpret_members(std::string line, int& pos, int nb_line, int& pos_line, std::ifstream &file_code){
    out_file << '\n' << line;
    pos_line++;
    modifier_for_line_count++;
    if (!first_members_opened){
        first_members_opened = true;
    } else {
        nb_of_opened_braces_members = 0;
    }
    while (std::getline(imported_file, line)){
        pos_line++;
        Log::Imports_Info() << line << "\n";
        if (line.find('{') != std::string::npos){
            Log::Imports_Info() << "line mod contains {" << "\n";
            nb_of_opened_braces_members++;
        }
        if (line.find('}') != std::string::npos){
            nb_of_opened_braces_members--;
        }
        Log::Imports_Info() << "nb_of_opened_braces : " << nb_of_opened_braces_members << "\n";
        if (line == "}" && nb_of_opened_braces_members == -1){
            Log::Imports_Info() << "close mod block" << "\n";
            out_file << '\n' << line;
            modifier_for_line_count++;
            break;
        }
        find_patterns(line, nb_line, pos_line, file_code);
    }
}

// find funcs, structs, etc
void find_patterns(std::string line, int nb_line, int& pos_line, std::ifstream& file_code){
    int pos = 0;
    if (line.find("?[") != std::string::npos){
        if (line.find("define") != std::string::npos){
        preprocess_instruction(line, file_code, pos_line_file);
        //out_file << line << "\n";
        }
        pos_line++;
        modifier_for_line_count++;
        return;
    }
    skip_spaces(line, pos);
    getIdentifierStr(line, pos, IdentifierStr);
    Log::Imports_Info() << "IdentifierStr : " << IdentifierStr << "\n";
    if (IdentifierStr == "func"){
        interpret_func(line, pos, nb_line, pos_line);
    } else if (IdentifierStr == "struct"){
        Log::Imports_Info() << "STRUCT FOUND" << "\n";
        interpret_struct(line, pos, nb_line, pos_line);
    } else if (IdentifierStr == "enum"){
        interpret_enum(line, pos, nb_line, pos_line);
    } else if (IdentifierStr == "union"){
        interpret_union(line, pos, nb_line, pos_line);
    } else if (IdentifierStr == "extern"){
        interpret_extern(line, pos_line);
    } else if (IdentifierStr == "mod"){
        interpret_mod(line, pos, nb_line, pos_line, file_code);
    } else if (IdentifierStr == "members"){
        interpret_members(line, pos, nb_line, pos_line, file_code);
    } else if (IdentifierStr == "type"){
        interpret_type(line, pos_line);
    }
}

void import_file(std::string Path, bool is_special_path){
    if (!is_special_path){
        getPathFromFilePOV(Path, filename);
    }
    Log::Imports_Info() << "Path : " << Path << "\n";
    int nb_line = get_nb_lines(imported_file, Path);
    imported_file.open(Path);
    if (imported_file.is_open()){
        int pos_line = 0;
        while (std::getline(imported_file, line)){
            Log::Imports_Info() << line << "\n";
            
            find_patterns(line, nb_line, pos_line, imported_file);
            pos_line++;
        }
    }
    imported_file.close();
}

void interpret_import(std::string line, int& pos_src){
    std::string Path;
    std::vector<std::string> Paths;
    skip_spaces(line, pos_src);
    bool is_special_path = getPath(line, pos_src, Paths);

    for (int i = 0; i < Paths.size(); i++){
        Log::Imports_Info() << "Importing file " << Paths.at(i) << "\n";
        import_file(Paths.at(i), is_special_path);
    }
}

void interpret_include(std::string line, int& pos_src){
    std::string Path;
    std::vector<std::string> Paths;
    std::ifstream included_file;
    skip_spaces(line, pos_src);
    getPath(line, pos_src, Paths);
    // for now include doesn't support multiple files. Maybe TODO
    Path = Paths.at(0);
    getPathFromFilePOV(Path, filename);
    Log::Imports_Info() << "Path : " << Path << "\n";
    int nb_line = get_nb_lines(included_file, Path);
    modifier_for_line_count += nb_line;
    included_file.open(Path);
    if (included_file.is_open()){
        int pos_line = 0;
        while (std::getline(included_file, line)){
            bool contains_non_whitespace_or_new_line = line.find_first_not_of(" \t\n\r\0") != std::string::npos;
            if (!contains_non_whitespace_or_new_line){
                continue;
            }
            Log::Imports_Info() << line << "\n";
            Log::Imports_Info() << "pos_line : " << pos_line << "\n";
            preprocess_replace_variable(line);
            if (line.size() >= 2 && line.at(0) == '?' && line.at(1) == '['){
                preprocess_instruction(line, included_file, pos_line);
            } else {
            out_file << line;
            if (pos_line != nb_line - 1){
                out_file << "\n";
            }
            }
            //modifier_for_line_count++;
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
    } else if (IdentifierStr == "include"){
        interpret_include(line, pos_src);
        return 1;
    }
    return 0;
}

// TODO : rename to preprocess_file
int generate_file_with_imports(std::string file_path, std::string out_path){
    std::ifstream file_code;
    std::string line;
    int nb_line = get_nb_lines(file_code, file_path);
    file_code.open(file_path);
    if (out_path == ""){
        out_file.open("temp_stdout.txt");
    } else {
        out_file.open(out_path);
    }
    int nb_imports_or_include = 0;
    if (Comp_context->std_mode){ // TODO : add a way to include core with no-std (explicit folder ?)
        include_prelude(std_path);
        out_file << "\n";
    }
    if (file_code.is_open()){
        //int pos_line = 0;
        bool last_line_macro = false;
        bool last_line_import_or_include = false;
        while (std::getline(file_code, line)){
            if (!last_line_macro && pos_line_file != nb_line && pos_line_file != 0){
                out_file << "\n";
            }
            bool contains_non_whitespace_or_new_line = line.find_first_not_of(" \t\n\0\r") != std::string::npos;
            if (last_line_import_or_include && !contains_non_whitespace_or_new_line){
                continue;
            }
            last_line_macro = false;
            last_line_import_or_include = false;
            Log::Imports_Info() << "line : " << line << "\n";
            if (line.size() >= 2 && line.at(0) == '?' && line.at(1) == '['){
                Log::Info() << "FOUND PREPROCESSOR INSTRUCTION" << "\n";
                preprocess_instruction(line, file_code, pos_line_file);
                last_line_macro = true;
            } else {
                preprocess_replace_variable(line);
                if (find_import_or_include(line) == 0){
                    out_file << line;
                } else {
                    nb_imports_or_include++;
                    last_line_import_or_include = true;
                }
            }
            pos_line_file++;
        }
    }
    file_code.close();
    out_file.close();
    if (out_path == ""){
        std::ifstream out_file_ifs("temp_stdout.txt");
        std::string stdout_content((std::istreambuf_iterator<char>(out_file_ifs)),(std::istreambuf_iterator<char>()));
        std::cout << stdout_content << std::endl;
        out_file_ifs.close();
        std::remove("temp_stdout.txt");
    }
    return nb_imports_or_include;
}


void add_test_imports(){
    std::vector<std::string> Paths;
    int pos_src = 0;
    std::string line = "@std/assert.cpoint";
    bool is_special_path = getPath(line, pos_src, Paths);
    import_file(Paths.at(0), is_special_path);
}