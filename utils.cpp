#include <iostream>

std::string get_os_from_target_triplet(std::string str){
    std::string r;
    int nb = 0;
    int pos = -1;
    for (int i = 0; i < str.size(); i++){
    if (str.at(i) == '-'){
    nb++;
    }
    if (nb >= 2){
    pos = i + 1;
    break;
    }
    }
    for (int i = pos; str.at(i) != '-'; i++){
    r += str.at(i);
    }
    std::cout << "os from target triplet : " << r << std::endl;
    return r;
}
