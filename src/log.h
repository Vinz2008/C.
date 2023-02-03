#include <iostream>

namespace Log {
    struct Info {
        Info() {
            std::cout << "[INFO] ";
        }
        template< class T >
        Info &operator<<(const T& val){
            std::cout<<val;
            return *this;
        }
    };
    struct Preprocessor_Info {
        Preprocessor_Info(){
            std::cout << "[PREPROCESSOR INFO] ";
        }
        template< class T >
        Preprocessor_Info &operator<<(const T& val){
            std::cout<<val;
            return *this;
        }
    };
    struct Imports_Info {
        Imports_Info(){
            std::cout << "[IMPORTS INFO] ";
        }
        template< class T >
        Imports_Info &operator<<(const T& val){
            std::cout<<val;
            return *this;
        }
    };
}