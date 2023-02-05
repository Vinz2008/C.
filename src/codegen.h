#include "llvm/IR/Instructions.h"
#include "types.h"
#include "ast.h"
#include <memory>

void InitializeModule(std::string filename);

class NamedValue {
public:
    llvm::AllocaInst* alloca_inst;
    Cpoint_Type type;
    NamedValue(llvm::AllocaInst* alloca_inst, Cpoint_Type type) : alloca_inst(alloca_inst), type(std::move(type)) {}
};

class Struct {
public:
    llvm::Type* struct_type;
    std::string struct_declaration_name;
    Struct(llvm::Type* struct_type, const std::string &struct_declaration_name) : struct_type(struct_type), struct_declaration_name(struct_declaration_name) {}
};

class StructDeclaration {
public:
    std::unique_ptr<StructDeclarAST> StructDeclar;
    llvm::Type* struct_type;
    StructDeclaration(std::unique_ptr<StructDeclarAST> StructDeclar, llvm::Type* struct_type) : StructDeclar(std::move(StructDeclar)), struct_type(struct_type) {}

};