#include <iostream>
std::string r;

static std::string get_word(std::string str, int p){
    r.clear();
    int nb = 0;
    int pos = -1;
    if (p == 0){
    pos = 0;
    } else {
    for (int i = 0; i < str.size(); i++){
    if (nb >= p){
    pos = i;
    break;
    }
    if (str.at(i) == '-'){
    nb++;
    }
    }
    }
    for (int i = pos; i < str.size() - 1 && str.at(i) != '-'; i++){
    r += str.at(i);
    }
    return r;
}

std::string get_os(std::string str){
    std::string r = get_word(str, 2);
    std::cout << "os from target triplet : " << r << std::endl;
    return r;
}

std::string get_arch(std::string str){
    std::string r = get_word(str, 0);
    return r;
}
std::string get_vendor(std::string str){
    std::string r = get_word(str, 1);
    return r;
}
