#include "abi.h"
#include "types.h"
#include "llvm/TargetParser/Triple.h"

extern std::unique_ptr<IRBuilder<>> Builder;
extern std::unique_ptr<LLVMContext> TheContext;
extern Triple TripleLLVM;

int get_pointer_size(){ // TODO : remove this (it is not used)
    if (TripleLLVM.isArch32Bit()){
        return 32;
    } else if (TripleLLVM.isArch64Bit()){
        return 64;
    }
    return 64;
}

Value* GetVaAdressSystemV(std::unique_ptr<ExprAST> va){
    if (!dynamic_cast<VariableExprAST*>(va.get())){
        return LogErrorV(emptyLoc, "Not implemented internal function to get address of va list with amd64 systemv abi for non variables");
    }
    auto zero = llvm::ConstantInt::get(*TheContext, llvm::APInt(64, 0, true));
    auto varVa = dynamic_cast<VariableExprAST*>(va.get());
    auto vaCodegened = va->codegen();
    return Builder->CreateGEP(vaCodegened->getType(), get_var_allocation(varVa->Name), {zero, zero}, "gep_for_va");
}


bool should_return_struct_with_ptr(Cpoint_Type cpoint_type){
    int max_size = 8; // 64 bit = 8 byte
    if (TripleLLVM.isArch32Bit()){
        max_size = 4;
    } else if (TripleLLVM.isArch64Bit()){
        max_size = 8;
    }
    int size_struct = find_struct_type_size(cpoint_type);
    if (size_struct > max_size){
        return true;
    }
    return false;
}

bool should_pass_struct_byval(Cpoint_Type cpoint_type){
    if (cpoint_type.is_just_struct() && is_struct_all_type(cpoint_type, float_type)){
        int float_nb = struct_get_number_type(cpoint_type, float_type);
        if (float_nb <= 2){
            return false;
        } else {
            return true;
        }
    }
    int max_size = 8; // 64 bit = 8 byte
    if (TripleLLVM.isArch32Bit()){
        max_size = 4; // 32 bit
    } else if (TripleLLVM.isArch64Bit()){
        max_size = 8;
    }
    int size_struct = find_struct_type_size(cpoint_type);
    if (size_struct > max_size){
        return true;
    }
    return false;
}

void addArgSretAttribute(Argument& Arg, Type* type){
    Arg.addAttr(Attribute::getWithStructRetType(*TheContext, type));
    Arg.addAttr(Attribute::getWithAlignment(*TheContext, Align(8)));
}

static int type_size(Cpoint_Type type){
    if (type.is_ptr){
        return (TripleLLVM.isArch64Bit()) ? 8 : 4;
    }
    return type.get_number_of_bits()/8;

}

static bool reorder_struct_sorter(const std::pair<std::string,Cpoint_Type>& lhs, const std::pair<std::string,Cpoint_Type>& rhs){
    return type_size(lhs.second) > type_size(rhs.second);
}

/*struct reorder_struct_sorter{
    inline bool operator() (const std::pair<std::string,Cpoint_Type>& lhs, const std::pair<std::string,Cpoint_Type>& rhs){
        return type_size(lhs.second) < type_size(rhs.second);
    }
};*/

// return value is if the struct should be reordered
bool reorder_struct(StructDeclaration* structDeclaration, std::string structName, std::vector<std::pair<std::string,Cpoint_Type>>& return_members){
    // for now only have a reorder warning for structs without complex types in it (TODO)
    for (int i = 0; i < structDeclaration->members.size(); i++){
        Cpoint_Type arg_type = structDeclaration->members.at(i).second;
        if (arg_type.type == other_type  && !arg_type.is_ptr){
            return false;
        }
    }
    //Cpoint_Type last_type; // starts as empty
    std::vector<std::pair<std::string,Cpoint_Type>> tempMembers = structDeclaration->members;
    std::sort(tempMembers.begin(), tempMembers.end(), &reorder_struct_sorter);
    bool should_reorder = false;
    for (int i = 0; i < tempMembers.size(); i++){
        if (tempMembers.at(i).first != structDeclaration->members.at(i).first){
            should_reorder = true;
        }
    }
    if (should_reorder){
        /*std::cout << "OLD STRUCT : " << "\n";
        std::cout << "struct " + structName + "{\n";
        for (int i = 0; i < structDeclaration->members.size(); i++){
            std::cout << "\tvar " << structDeclaration->members.at(i).first << " : " << create_pretty_name_for_type(structDeclaration->members.at(i).second) << "\n";
        }
        std::cout << "}" << "\n";
        std::cout << "NEW STRUCT : " << "\n";
        std::cout << "struct " + structName + "{\n";
        for (int i = 0; i < tempMembers.size(); i++){
            std::cout << "\tvar " << tempMembers.at(i).first << " : " << create_pretty_name_for_type(tempMembers.at(i).second) << "\n";
        }
        std::cout << "}" << std::endl;*/
        return_members = tempMembers;
    }
    return should_reorder;
    /*for (int i = 0; i < structDeclaration->members.size(); i++){
        Cpoint_Type arg_type = structDeclaration->members.at(i).second;
        if (!last_type.is_empty && type_size(arg_type) > last_type){

        }
    }*/
}