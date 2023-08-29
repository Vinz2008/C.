#include <iostream>

#include "errors.h"
#include "ast.h"

class StructDeclarAST;
class ExprAST;
class PrototypeAST;
class ReturnAST;
class FunctionAST;
class GlobalVariableAST;
class LoopExprAST;
class ModAST;
class TestAST;
class UnionDeclarAST;
class EnumDeclarAST;

std::unique_ptr<ExprAST> vLogError(const char* Str, va_list args, Source_location astLoc);
std::unique_ptr<ExprAST> LogError(const char *Str, ...);
std::unique_ptr<PrototypeAST> LogErrorP(const char *Str, ...);
std::unique_ptr<ReturnAST> LogErrorR(const char *Str, ...);
std::unique_ptr<StructDeclarAST> LogErrorS(const char *Str, ...);
std::unique_ptr<FunctionAST> LogErrorF(const char *Str, ...);
std::unique_ptr<GlobalVariableAST> LogErrorG(const char *Str, ...);
std::unique_ptr<LoopExprAST> LogErrorL(const char* Str, ...);
std::unique_ptr<ModAST> LogErrorM(const char* Str, ...);
std::unique_ptr<TestAST> LogErrorT(const char* Str, ...);
std::unique_ptr<UnionDeclarAST> LogErrorU(const char* Str, ...);
std::unique_ptr<EnumDeclarAST> LogErrorE(const char* Str, ...);
Value* LogErrorV(Source_location astLoc, const char *Str, ...);
Function* LogErrorF(Source_location astLoc, const char *Str, ...);
GlobalVariable* LogErrorGLLVM(const char *Str, ...);

#ifndef _LOG_HEADER_
#define _LOG_HEADER_

//extern bool debug_mode;
extern bool silent_mode;

extern std::unique_ptr<Compiler_context> Comp_context;
extern Source_location emptyLoc;

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
        // TODO : add lines number and file like in error
        Warning(Source_location loc){
            Color::Modifier red(Color::FG_RED);
            std::cout<<red;
            if (loc != emptyLoc){
                std::cout << "[Warning] at " << Comp_context->filename << ":" << loc.line_nb << ":" << ((loc.col_nb > 0) ? loc.col_nb-1 : loc.col_nb) << " ";
            } else {
                std::cout << "[Warning] ";
            }
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