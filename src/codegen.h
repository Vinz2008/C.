#include "llvm/IR/Instructions.h"
#include "types.h"
#include "ast.h"
#include <memory>
#include <unordered_map>

void InitializeModule(std::string filename);

class NamedValue {
public:
    llvm::AllocaInst* alloca_inst;
    Cpoint_Type type;
    llvm::Type* struct_type;
    std::string struct_declaration_name;
    NamedValue(llvm::AllocaInst* alloca_inst, Cpoint_Type type, llvm::Type* struct_type = get_type_llvm(Cpoint_Type(double_type)), const std::string &struct_declaration_name = "") : alloca_inst(alloca_inst), type(std::move(type)), struct_type(struct_type), struct_declaration_name(struct_declaration_name) {}
};

class TemplateProto {
public:
    std::unique_ptr<FunctionAST> functionAST;
    std::string template_type_name;
    TemplateProto(std::unique_ptr<FunctionAST> functionAST, const std::string& template_type_name) : functionAST(std::move(functionAST)), template_type_name(template_type_name) {}
};

class StructDeclar {
public:
    std::unique_ptr<StructDeclarAST> declarAST;
    std::string template_type_name;
    StructDeclar(std::unique_ptr<StructDeclarAST> declarAST, const std::string& template_type_name) : declarAST(std::move(declarAST)), template_type_name(template_type_name) {}
};

class TemplateCall {
public:
    /*std::string*/ Cpoint_Type type;
    std::unique_ptr<FunctionAST> functionAST;
    TemplateCall(Cpoint_Type type, std::unique_ptr<FunctionAST> functionAST) : type(type), functionAST(std::move(functionAST)) {}
};

class TemplateStructCreation {
public:
    /*std::string*/ Cpoint_Type type;
    std::unique_ptr<StructDeclarAST> structDeclarAST;
    TemplateStructCreation(Cpoint_Type type, std::unique_ptr<StructDeclarAST> structDeclarAST) : type(type), structDeclarAST(std::move(structDeclarAST)) {}
};

class GlobalVariableValue {
public:
    Cpoint_Type type;
    GlobalVariable* globalVar;
    GlobalVariableValue(Cpoint_Type type, GlobalVariable* globalVar) : type(type), globalVar(globalVar) {}
};

class Struct {
public:
    llvm::Type* struct_type;
    std::string struct_declaration_name;
    Struct(llvm::Type* struct_type, const std::string &struct_declaration_name) : struct_type(struct_type), struct_declaration_name(struct_declaration_name) {}
};

/*
class ClassDeclaration {
public:
    llvm::Type* class_type;
    std::vector<std::pair<std::string,Cpoint_Type>> members;
    ClassDeclaration(llvm::Type* class_type, std::vector<std::pair<std::string,Cpoint_Type>> members) : class_type(class_type), members(std::move(members)) {}
};


class StructDeclaration {
public:
    std::unique_ptr<StructDeclarAST> StructDeclar;
    llvm::Type* struct_type;
    std::vector<std::pair<std::string,Cpoint_Type>> members;
    StructDeclaration(std::unique_ptr<StructDeclarAST> StructDeclar, llvm::Type* struct_type, std::vector<std::pair<std::string,Cpoint_Type>> members) : StructDeclar(std::move(StructDeclar)), struct_type(struct_type), members(std::move(members)) {}
};
*/


class StructDeclaration {
public:
    llvm::Type* struct_type;
    std::vector<std::pair<std::string,Cpoint_Type>> members;
    std::vector<std::string> functions;
    StructDeclaration(llvm::Type* struct_type, std::vector<std::pair<std::string,Cpoint_Type>> members, std::vector<std::string> functions) : struct_type(struct_type), members(std::move(members)), functions(std::move(functions)) {}
};

class UnionDeclaration {
public:
    llvm::Type* union_type;
    std::vector<std::pair<std::string,Cpoint_Type>> members;
    UnionDeclaration(llvm::Type* union_type, std::vector<std::pair<std::string,Cpoint_Type>> members) : union_type(union_type), members(std::move(members)) {}
};

class EnumDeclaration {
public:
    llvm::Type* enumType;
    std::unique_ptr<EnumDeclarAST> EnumDeclar;
    EnumDeclaration(llvm::Type* enumType, std::unique_ptr<EnumDeclarAST> EnumDeclar) : enumType(enumType), EnumDeclar(std::move(EnumDeclar)) {} 
};

class ExternToGenerate {
public:
    std::string Name;
    llvm::FunctionType* functionType;
    std::vector<std::pair<std::string, Cpoint_Type>> Args;
    ExternToGenerate(std::string Name, llvm::FunctionType* functionType, std::vector<std::pair<std::string, Cpoint_Type>> Args) : Name(Name), functionType(functionType), Args(Args) {}
};

std::string module_function_mangling(std::string module_name, std::string function_name);
//void codegenTemplates();
//void codegenStructTemplates();
//void generateTests();
std::string get_struct_template_name(std::string struct_name, /*std::string*/ Cpoint_Type type);
void generateExterns();

Value *LogErrorV(Source_location astLoc, const char *Str, ...);