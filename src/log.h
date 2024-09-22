#include <iostream>
#include <sstream>

#include "llvm/IR/Function.h"
#include "llvm/IR/Value.h"

using namespace llvm;

#include "errors.h"
//#include "ast.h"

class ExprAST;
class PrototypeAST;
class ReturnAST;
class StructDeclarAST;
class FunctionAST;
class GlobalVariableAST;
class LoopExprAST;
class ModAST;
class TestAST;
class UnionDeclarAST;
class EnumDeclarAST;
class MembersDeclarAST;

void vLogError(const char* Str, va_list args, Source_location astLoc);
void LogError(const char *Str, ...);
std::unique_ptr<ExprAST> LogErrorE(const char *Str, ...);
std::unique_ptr<PrototypeAST> LogErrorP(const char *Str, ...);
std::unique_ptr<ReturnAST> LogErrorR(const char *Str, ...);
std::unique_ptr<StructDeclarAST> LogErrorS(const char *Str, ...);
std::unique_ptr<FunctionAST> LogErrorF(const char *Str, ...);
std::unique_ptr<GlobalVariableAST> LogErrorG(const char *Str, ...);
std::unique_ptr<LoopExprAST> LogErrorL(const char* Str, ...);
std::unique_ptr<ModAST> LogErrorM(const char* Str, ...);
std::unique_ptr<TestAST> LogErrorT(const char* Str, ...);
std::unique_ptr<UnionDeclarAST> LogErrorU(const char* Str, ...);
std::unique_ptr<EnumDeclarAST> LogErrorEnum(const char* Str, ...);
Value* LogErrorV(Source_location astLoc, const char *Str, ...);
Function* LogErrorF(Source_location astLoc, const char *Str, ...);
GlobalVariable* LogErrorGLLVM(const char *Str, ...);
std::unique_ptr<MembersDeclarAST> LogErrorMembers(const char *Str, ...);

#ifndef _LOG_HEADER_
#define _LOG_HEADER_

//extern bool debug_mode;
extern bool silent_mode;

extern std::unique_ptr<Compiler_context> Comp_context;
extern Source_location emptyLoc;


// TODO : move this in cpp file
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
            // TODO : add here and warnings : if (!silent_mode)
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
    // first part of the warning that is in red
    struct WarningHead {
        std::stringstream head_stream;
        WarningHead(Source_location loc){
            /*Color::Modifier red(Color::FG_RED);
            head_stream << red;*/
            if (loc != emptyLoc){
                head_stream << "[Warning] at " << Comp_context->filename << ":" << loc.line_nb << ":" << ((loc.col_nb > 0) ? loc.col_nb-1 : loc.col_nb) << " ";
            } else {
                head_stream << "[Warning] ";
            }
        }
        template< class T >
        WarningHead &operator<<(const T& val){
            head_stream << val;
            return *this;
        }
        /*void end(){
            Color::Modifier def(Color::FG_DEFAULT);
            std::cout << def;
        }
        ~WarningHead(){
            Color::Modifier def(Color::FG_DEFAULT);
            std::cout << def;
        }*/
    };
    // the optional content that is in the normal color
    struct WarningContent {
        std::stringstream content_stream;
        template< class T >
        WarningContent &operator<<(const T& val){
            //std::cout << val;
            content_stream << val;
            return *this;
        }
        /*void end(){
            std::cout << content_stream.str() <<"----\n";
        }*/
        /*~WarningContent(){
            std::flush(std::cout);
        }*/
    };
    struct Warning {
        Source_location loc;
        struct WarningHead head;
        struct WarningContent content;
        template< class T >
        Warning &operator<<(const T& val){
            head << val;
            return *this;
        }
        void end(){
            Color::Modifier red(Color::FG_RED);
            Color::Modifier def(Color::FG_DEFAULT);
            std::cout << red << head.head_stream.str() << def;
            if (loc.line != ""){
                std::cout << "\t" << loc.line << "\n";
                std::cout << "\t";
                for (int i = 0; i < loc.col_nb-2; i++){
                    std::cout << " ";
                }
                std::cout << "^\n";
            }
            if (content.content_stream.str() != ""){
                std::cout << content.content_stream.str() << "----\n";
            }
            std::flush(std::cout);
        }
        // TODO : refactor the warning struct as a builder pattern like in rust
        Warning(Source_location loc = emptyLoc) : loc(loc), head(loc) {}
        /*Warning(Source_location loc = emptyLoc){
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
        }*/
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