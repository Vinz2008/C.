#include <vector>
#include <string>
#include <fstream>
#include <filesystem>
#include "cli.h"

namespace fs = std::filesystem;

bool file_contains_test(std::string path){
    std::ifstream file(path);
    std::string line;
    while(getline(file, line)){
        if (line.find("test") != std::string::npos){
            int pos = line.find("test");
            line = line.substr(pos, line.size());
            // TODO : find not just "test", find also '"', and the { in order to find if there's a test
            int i = 0;
            while (i < line.size() && line.at(i) != '\"'){
                i++;
            }
            if (i != line.size()){
                return true; 
            }
        }
    }
    return false;
}

void buildTest(std::vector<std::string>& PathList, int pos){
    std::vector<std::string> linkTestPathList;
    std::string object_file_main = fs::path(PathList.at(pos)).replace_extension(".test.o").string();
    std::string exe_name = fs::path(PathList.at(pos)).replace_extension(".test").string();
    compileFile("", "-test", PathList.at(pos), "", object_file_main);
    linkTestPathList.push_back(object_file_main);
    for (int i = 0; i < PathList.size(); i++){
        if (i != pos){
            linkTestPathList.push_back(PathList.at(i));
        }
    }
    // TODO : find these from the config by passing it through args
    bool is_gc = true;
    bool is_strip_mode = false;
    linkFiles(linkTestPathList, exe_name, "", "", "", is_gc, is_strip_mode);
}

void buildTestObjectFiles(std::vector<std::string>& PathList){
    for (int i = 0; i < PathList.size(); i++){
        compileFile("", "", PathList.at(i), "");
    }
}

void buildTests(std::vector<std::string> PathList){
    buildTestObjectFiles(PathList);
    for (int i = 0; i < PathList.size(); i++){
        if (file_contains_test(PathList.at(i))){
            buildTest(PathList, i);
        }
    }
}

void runTests(std::vector<std::string> PathList){
    for (int i = 0; i < PathList.size(); i++){
        if (file_contains_test(PathList.at(i))){
            buildTest(PathList, i);
        }
    }
}