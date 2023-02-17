#include "llvm/IR/Instructions.h"
#include "types.h"
#include "ast.h"
#include <memory>
#include <map>

void InitializeModule(std::string filename);

class NamedValue {
public:
    llvm::AllocaInst* alloca_inst;
    Cpoint_Type type;
    llvm::Type* struct_type;
    std::string struct_declaration_name;
    NamedValue(llvm::AllocaInst* alloca_inst, Cpoint_Type type, llvm::Type* struct_type = get_type_llvm(Cpoint_Type(double_type)), const std::string &struct_declaration_name = "") : alloca_inst(alloca_inst), type(std::move(type)), struct_type(struct_type), struct_declaration_name(struct_declaration_name) {}
};

class Struct {
public:
    llvm::Type* struct_type;
    std::string struct_declaration_name;
    Struct(llvm::Type* struct_type, const std::string &struct_declaration_name) : struct_type(struct_type), struct_declaration_name(struct_declaration_name) {}
};

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
