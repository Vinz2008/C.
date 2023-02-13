#include <fstream>

bool FileExists(std::string filename){
    std::ifstream file(filename);
    if(file.is_open()){
    return 1;
    file.close();
    } else {
    return 0;
    }
}