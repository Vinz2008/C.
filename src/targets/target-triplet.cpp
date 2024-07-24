#include <iostream>

static std::string get_word(std::string str, int p){
    std::string r = "";
    int nb = 0;
    int pos = -1;
    if (p == 0){
        pos = 0;
    } else {
        for (int i = 0; i < str.size(); i++){
            if (nb == p){
                pos = i;
                break;
            }
            if (str.at(i) == '-'){
                nb++;
            }
        }
    }
    if (pos == -1){
        return "";
    }
    for (int i = pos; i < str.size() && str.at(i) != '-'; i++){
        r += str.at(i);
    }
    return r;
}

std::string get_arch(std::string str){
    return get_word(str, 0);
}

std::string get_vendor(std::string str){
    return get_word(str, 1);
}


std::string get_os(std::string str){
    return get_word(str, 2);
}

std::string get_env(std::string str){
    return get_word(str, 3);
}