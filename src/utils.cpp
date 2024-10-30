#include <fstream>
#include <iostream>
#include <dirent.h>
#include <functional>
#include <sys/stat.h>
#include <filesystem>
#include <cstring>

namespace fs = std::filesystem;


bool FileExists(std::string filename){
    std::ifstream file(filename);
    if(file.is_open()){
    return true;
    file.close();
    } 
    return false;
}
bool FolderExists(std::string foldername){
    struct stat sb;
    if (stat(foldername.c_str(), &sb) == 0){
        std::cout << "folder exists" << std::endl;
        return true;
    } else {
        return false;
    }
}

std::vector<std::string> PATHS;

bool doesExeExist(std::string filename){
    if (PATHS.empty()){
        char* PATH = getenv("PATH");
        PATH = strdup(PATH);
        char *path_string = strtok(PATH, ":");
        while (path_string != NULL) {
            PATHS.push_back(path_string);
            path_string = strtok(NULL, ":");
        }
        free(PATH);
    }
    for (int i = 0; i < PATHS.size(); i++){
        std::string exepath = PATHS.at(i) + "/" + filename;
        if (fs::exists(exepath)){
            return true;
        }
    }
    return false;
}

void listFiles(const std::string &path, std::function<void(const std::string &)> cb) {
    for (const fs::directory_entry& dir_entry : fs::recursive_directory_iterator(path)){
        if (dir_entry.is_regular_file()){
            cb(dir_entry.path().string());
        }
    }
}

std::string getFileExtension(std::string path){
    for (int i = path.size()-1; i > 0; i--){
        if (path.at(i) == '.'){
            return path.substr(i+1, path.size()-(i+1));
        }
    }
    return "";
}

int stringDistance(std::string s1, std::string s2) {
    // Create a table to store the results of subproblems
    std::vector<std::vector<int>> dp(s1.length() + 1, std::vector<int>(s2.length() + 1));
 
    // Initialize the table
    for (int i = 0; i <= s1.length(); i++) {
        dp[i][0] = i;
    }
    for (int j = 0; j <= s2.length(); j++) {
        dp[0][j] = j;
    }
 
    // Populate the table using dynamic programming
    for (int i = 1; i <= s1.length(); i++) {
        for (int j = 1; j <= s2.length(); j++) {
            if (s1[i-1] == s2[j-1]) {
                dp[i][j] = dp[i-1][j-1];
            } else {
                dp[i][j] = 1 + std::min(dp[i-1][j], std::min(dp[i][j-1], dp[i-1][j-1]));
            }
        }
    }
 
    // Return the edit distance
    return dp[s1.length()][s2.length()];
}

bool is_char_one_of_these(int c, std::string chars){
    for (int i = 0; i < chars.size(); i++){
        if (c == chars.at(i)){
            return true;
        }
    }
    return false;
}