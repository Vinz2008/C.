#include <iostream>

#include "errors.h"

#ifndef _LOG_HEADER_
#define _LOG_HEADER_

//extern bool debug_mode;
extern bool silent_mode;

extern std::unique_ptr<Compiler_context> Comp_context;

namespace Log {
    namespace Color {
    enum Code {
        FG_RED      = 31,
        FG_GREEN    = 32,
        FG_BLUE     = 34,
        FG_DEFAULT  = 39,
        BG_RED      = 41,
        BG_GREEN    = 42,
        BG_BLUE     = 44,
        BG_DEFAULT  = 49
    };
    class Modifier {
        Code code;
    public:
        Modifier(Code pCode) : code(pCode) {}
        friend std::ostream&
        operator<<(std::ostream& os, const Modifier& mod) {
            return os << "\033[" << mod.code << "m";
        }
    };
    }
    struct Print {
        template< class T >
        Print& operator<<(const T& val){
            if (!silent_mode){
                std::cout << val;
            }
            return *this;
        }
    };
    struct Info {
        Info() {
            if (Comp_context->debug_mode){
            std::cout << "[INFO] ";
            }
        }
        template< class T >
        Info &operator<<(const T& val){
            if (Comp_context->debug_mode){
            std::cout<<val;
            }
            return *this;
        }
    };
    struct Warning {
        Warning(){
            Color::Modifier red(Color::FG_RED);
            std::cout<<red;
            std::cout << "[Warning] ";
        }
        template< class T >
        Warning &operator<<(const T& val){
            std::cout<<val;
            return *this;
        }
        ~Warning(){
            Color::Modifier def(Color::FG_DEFAULT);
            std::cout<<def;
            std::flush(std::cout);
        }
    };
    struct Preprocessor_Info {
        Preprocessor_Info(){
            if (Comp_context->debug_mode){
            std::cout << "[PREPROCESSOR INFO] ";
            }
        }
        template< class T >
        Preprocessor_Info &operator<<(const T& val){
            if (Comp_context->debug_mode){
            std::cout<<val;
            }
            return *this;
        }
    };
    struct Preprocessor_Error {
        Preprocessor_Error(){
            Color::Modifier red(Color::FG_RED);
            std::cout<<red;
            std::cout << "[PREPROCESSOR ERROR] ";
        }
        template< class T >
        Preprocessor_Error &operator<<(const T& val){
            std::cout<<val;
            return *this;
        }
        ~Preprocessor_Error(){
            Color::Modifier def(Color::FG_DEFAULT);
            std::cout<<def;
        }
    };
    struct Imports_Info {
        Imports_Info(){
            if (Comp_context->debug_mode){
            std::cout << "[IMPORTS INFO] ";
            }
        }
        template< class T >
        Imports_Info &operator<<(const T& val){
            if (Comp_context->debug_mode){
            std::cout<<val;
            }
            return *this;
        }
    };
    struct Build_Info {
        Build_Info(){
            if (Comp_context->debug_mode){
            std::cout << "[BUILD INFO] ";
            }
        }
        template< class T >
        Build_Info &operator<<(const T& val){
            if (Comp_context->debug_mode){
            std::cout<<val;
            }
            return *this;
        }
    };
}

#endif