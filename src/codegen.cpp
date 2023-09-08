#include "codegen.h"
#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"
#include "llvm/IR/DIBuilder.h"
#include <iostream>
#include <map>
#include <stack>
#include <cstdarg>
#include "ast.h"
#include "lexer.h"
#include "types.h"
#include "log.h"
#include "debuginfo.h"
#include "operators.h"
#include "checking.h"
#include "templates.h"
#include "config.h"

using namespace llvm;

std::unique_ptr<LLVMContext> TheContext;
std::unique_ptr<Module> TheModule;
std::unique_ptr<IRBuilder<>> Builder;
std::map<std::string, std::unique_ptr<NamedValue>> NamedValues;
std::map<std::string, std::unique_ptr<GlobalVariableValue>> GlobalVariables;
std::map<std::string, std::unique_ptr<PrototypeAST>> FunctionProtos;
std::map<std::string, std::unique_ptr<StructDeclaration>> StructDeclarations;
std::map<std::string, std::unique_ptr<UnionDeclaration>> UnionDeclarations;
std::map<std::string, std::unique_ptr<EnumDeclaration>> EnumDeclarations;

std::map<std::string, std::unique_ptr<TemplateProto>> TemplateProtos;

std::map<std::string, std::unique_ptr<StructDeclar>> TemplateStructDeclars;
std::vector<std::string> modulesNamesContext;

std::map<std::string, Function*> GeneratedFunctions;

std::stack<BasicBlock*> blocksForBreak; // TODO : put this in Compiler_Context

std::vector<std::unique_ptr<TemplateCall>> TemplatesToGenerate;

std::vector<std::unique_ptr<TemplateStructCreation>> StructTemplatesToGenerate;

std::pair<std::string, /*std::string*/ Cpoint_Type> TypeTemplateCallCodegen; // contains the type of template in function call
std::string TypeTemplateCallAst = ""; // TODO : replace this by a vector to have multiple templates in the future ?


std::vector<std::unique_ptr<TestAST>> testASTNodes;

std::map<std::string, Value*> StringsGenerated;


extern std::map<std::string, int> BinopPrecedence;
extern std::unique_ptr<DIBuilder> DBuilder;
extern struct DebugInfo CpointDebugInfo;

extern std::string filename;
extern std::string first_filename;

extern Source_location emptyLoc;

extern bool debug_info_mode;

//extern bool test_mode;
//extern bool std_mode;
//extern bool gc_mode;
//extern bool is_release_mode;

// TODO : return at last line of function after if returns the iftmp and not the return value : fix this

Value *LogErrorV(Source_location astLoc, const char *Str, ...);

void add_manually_extern(std::string fnName, Cpoint_Type cpoint_type, std::vector<std::pair<std::string, Cpoint_Type>> ArgNames, unsigned Kind, unsigned BinaryPrecedence, bool is_variable_number_args, bool has_template, std::string TemplateName);

std::string struct_function_mangling(std::string struct_name, std::string name){
  std::string mangled_name = struct_name + "__" + name;
  return mangled_name;
}

std::string module_function_mangling(std::string module_name, std::string function_name){
  std::string mangled_name = module_name + "___" + function_name;
  return mangled_name;
}

Value* getTypeId(Value* valueLLVM){
    Type* valType = valueLLVM->getType();
    Cpoint_Type cpoint_type = get_cpoint_type_from_llvm(valType);
    return ConstantFP::get(*TheContext, APFloat((double)cpoint_type.type));
}


Value* refletionInstruction(std::string instruction, std::vector<std::unique_ptr<ExprAST>> Args){
    if (instruction == "typeid"){
        if (Args.size() > 1){
            return LogErrorV(emptyLoc, "wrong number of arguments for reflection function");
        }
        return getTypeId(Args.at(0)->codegen());
    } else if (instruction == "getstructname"){
        if (Args.size() > 1){
            return LogErrorV(emptyLoc, "wrong number of arguments for reflection function");
        }
        Value* val = Args.at(0)->codegen();
        if (!val->getType()->isStructTy()){
            return LogErrorV(emptyLoc, "argument is not a struct in getmembername");
        }
        auto structType = static_cast<StructType*>(val->getType());
        std::string structName = (std::string)structType->getStructName();
        Log::Info() << "get struct name : " << structName << "\n";
        return Builder->CreateGlobalStringPtr(StringRef(structName.c_str()));
    } else if (instruction == "getmembernb"){
        if (Args.size() > 1){
            return LogErrorV(emptyLoc, "wrong number of arguments for reflection function");
        }
        Value* val = Args.at(0)->codegen();
        if (!val->getType()->isStructTy()){
            return LogErrorV(emptyLoc, "argument is not a struct in getmembername");
        }
        auto structType = static_cast<StructType*>(val->getType());
        int nb_member = -1;
        nb_member = structType->getNumElements();
        Log::Info() << "getmembernbname : " << nb_member << "\n";
        return ConstantFP::get(*TheContext, APFloat((double)nb_member));
    }
    return LogErrorV(emptyLoc, "Unknown Reflection Instruction");
}

Function *getFunction(std::string Name) {
  // First, see if the function has already been added to the current module.
  if (auto *F = TheModule->getFunction(Name)){
    Log::Info() << "TheModule->getFunction " << Name << "\n";
    return F;
  }

  // If not, check whether we can codegen the declaration from some existing
  // prototype.
  auto FI = FunctionProtos.find(Name);
  if (FI != FunctionProtos.end()){
    Log::Info() << "FI->second->codegen() " << Name << "\n";
    return FI->second->codegen();
  }
  Log::Info() << "nullptr " << Name << "\n";
  // If no existing prototype exists, return null.
  return nullptr;
}

static AllocaInst *CreateEntryBlockAlloca(Function *TheFunction,
                                          StringRef VarName, Cpoint_Type type) {
  IRBuilder<> TmpB(&TheFunction->getEntryBlock(),
                 TheFunction->getEntryBlock().begin());
  return TmpB.CreateAlloca(get_type_llvm(type), 0,
                           VarName);
}

BasicBlock* get_basic_block(Function* TheFunction, std::string name){
  for (Function::iterator b = TheFunction->begin(), be = TheFunction->end(); b != be; ++b){
    BasicBlock* BB = &(*b);
    Log::Info() << "BasicBlock : " << (std::string)BB->getName() << "\n";
    if (BB->getName() == name){
      return BB;
    }
  }
  return nullptr;
}

std::string get_struct_template_name(std::string struct_name, /*std::string*/ Cpoint_Type type){
    //return struct_name + "____" + type;
    return struct_name + "____" + create_mangled_name_from_type(type);
}

Value* callLLVMIntrisic(std::string Callee, std::vector<std::unique_ptr<ExprAST>>& Args){
  Callee = Callee.substr(5, Callee.size());
  Log::Info() << "llvm intrisic called " << Callee << "\n";
  llvm::Intrinsic::IndependentIntrinsics intrisicId = llvm::Intrinsic::not_intrinsic;
  if (Callee == "va_start"){
    intrisicId = Intrinsic::vastart;
  } else if (Callee == "va_end"){
    intrisicId = Intrinsic::vaend;
  } else if (Callee == "abs"){
    intrisicId = Intrinsic::abs;
  } else if (Callee == "cos"){
    intrisicId = Intrinsic::cos;
  } else if (Callee == "addressofreturnaddress"){
    intrisicId = Intrinsic::addressofreturnaddress;
  } else if (Callee == "bswap"){
    intrisicId = Intrinsic::bswap;
  } else if (Callee == "exp"){
    intrisicId = Intrinsic::exp;
  } else if (Callee == "fabs"){
    intrisicId = Intrinsic::fabs;
  } else if (Callee == "floor"){
    intrisicId = Intrinsic::floor;
  } else if (Callee == "log"){
    intrisicId = Intrinsic::log;
  } else if (Callee == "memcpy"){
    intrisicId = Intrinsic::memcpy;
  } else if (Callee == "memmove"){
    intrisicId = Intrinsic::memmove;
  } else if (Callee == "memset"){
    intrisicId = Intrinsic::memset;
  } else if (Callee == "trap"){
    intrisicId = Intrinsic::trap;
  } else if (Callee == "trunc"){
    intrisicId = Intrinsic::trunc;
  } else if (Callee == "read_register"){
    intrisicId = Intrinsic::read_register;
  } 
  Function *CalleeF = Intrinsic::getDeclaration(TheModule.get(), intrisicId);
  std::vector<Value *> ArgsV;
  for (unsigned i = 0, e = Args.size(); i != e; ++i) {
    Value* temp_val = Args[i]->codegen();
    ArgsV.push_back(temp_val);
    if (!ArgsV.back())
      return nullptr;
  }
  // add additional args for intrisics
  if (Callee == "memcpy" || Callee == "memset" || Callee == "memmove") {
    ArgsV.push_back(BoolExprAST(false).codegen()); // the bool is if it is volatile
  }
  if (Callee == "abs"){
    ArgsV.push_back(BoolExprAST(false).codegen());  // the bool is if it is a poison value
  }
  return Builder->CreateCall(CalleeF, ArgsV, "calltmp");
}

Value* getSizeOfStruct(AllocaInst *A){
    std::vector<Value*> ArgsV;
    ArgsV.push_back(A);
    ArgsV.push_back(ConstantInt::get(*TheContext, llvm::APInt(1, 0, true))); // return -1 if object size is unknown
    ArgsV.push_back(ConstantInt::get(*TheContext, llvm::APInt(1, 1, true))); // return unknown size if null is passed
    ArgsV.push_back(ConstantInt::get(*TheContext, llvm::APInt(1, 0, true))); // is the calculation made at runtime
    std::vector<Type*> OverloadedTypes;
    OverloadedTypes.push_back(get_type_llvm(int_type));
    OverloadedTypes.push_back(get_type_llvm(i64_type));
    Function *CalleeF = Intrinsic::getDeclaration(TheModule.get(), Intrinsic::objectsize, OverloadedTypes);
    return Builder->CreateCall(CalleeF, ArgsV, "sizeof_calltmp");
}

Value *NumberExprAST::codegen() {
  // TODO : create an int if the number is not decimal
  /*if (trunc(Val) == Val){
    return ConstantInt::get(*TheContext, APInt(32, (int)Val, true));
  } else {
    return ConstantFP::get(*TheContext, APFloat(Val));
  }*/
  return ConstantFP::get(*TheContext, APFloat(Val));
}

Value* StringExprAST::codegen() {
  Log::Info() << "Before Codegen " << str << '\n';
  Value* string;
  if (StringsGenerated[str]){
    string = StringsGenerated[str];
  } else {
    string = Builder->CreateGlobalStringPtr(StringRef(str.c_str()));
    StringsGenerated[str] = string;
  }
  return string;
}

Value *VariableExprAST::codegen() {
  // Look this variable up in the function.
  if (GlobalVariables[Name] != nullptr){
    return Builder->CreateLoad(get_type_llvm(type), GlobalVariables[Name]->globalVar, Name.c_str());
  }
  if (FunctionProtos[Name] != nullptr){
    Log::Info() << "Using function pointer" << "\n";
    std::vector<Type *> args;
    for (int i = 0; i < FunctionProtos[Name]->Args.size(); i++){
      args.push_back(get_type_llvm(FunctionProtos[Name]->Args.at(i).second));
    }
    Type* return_type = get_type_llvm(FunctionProtos[Name]->cpoint_type);
    Log::Info() << "Created Function return type" << "\n";
    FunctionType* functionType = FunctionType::get(return_type, args, FunctionProtos[Name]->is_variable_number_args);
    Log::Info() << "Created Function type" << "\n";
    return GeneratedFunctions[Name];
    //return Builder->CreateLoad(functionType, FunctionProtos[Name]->codegen(), Name.c_str());
  }
  if (NamedValues[Name] == nullptr) {
  return LogErrorV(this->loc, "Unknown variable name %s", Name.c_str());
  }
  AllocaInst *A = NamedValues[Name]->alloca_inst;
  if (!A)
    return LogErrorV(this->loc, "Unknown variable name %s", Name.c_str());
  return Builder->CreateLoad(A->getAllocatedType(), A, Name.c_str());
}

Value* StructMemberExprAST::codegen() {
    Log::Info() << "STRUCT MEMBER CODEGEN" << "\n";
    AllocaInst* Alloca = nullptr;
    if (StructName == "reflection"){
        goto if_reflection;
    }
    if (NamedValues[StructName] == nullptr){
        return LogErrorV(this->loc, "Can't find struct that is used for a member");
    }
    Alloca = NamedValues[StructName]->alloca_inst;
    Log::Info() << "struct type : " <<  NamedValues[StructName]->type << "\n";
    if (!NamedValues[StructName]->type.is_struct ){ // TODO : verify if is is really  struct (it didn't work for example with the self of structs function members)
      return LogErrorV(this->loc, "Using a member of variable even though it is not a struct");
    }
    Log::Info() << "StructName : " << StructName << "\n";
    Log::Info() << "StructName len : " << StructName.length() << "\n";
    if (NamedValues[StructName] == nullptr){
      Log::Info() << "NamedValues[StructName] nullptr" << "\n";
    }
    //Log::Info() << "struct_declaration_name : " << NamedValues[StructName]->struct_declaration_name << "\n"; // USE FOR NOW STRUCT NAME FROM CPOINT_TYPE
    Log::Info() << "struct_declaration_name : " << NamedValues[StructName]->type.struct_name << "\n";
    Log::Info() << "struct_declaration_name length : " << NamedValues[StructName]->type.struct_name.length() << "\n";

    //bool is_function_call = false;
if_reflection:
    if (is_function_call){
      if (StructName == "reflection"){
        return refletionInstruction(MemberName, std::move(Args));
      }
      bool found_function = false;
      auto functions =  StructDeclarations[NamedValues[StructName]->type.struct_name]->functions;
      for (int i = 0; i < functions.size(); i++){
      Log::Info() << "functions.at(i) : " << functions.at(i) << "\n";
      if (functions.at(i) == MemberName){
        //is_function_call = true;
        found_function = true;
        Log::Info() << "Is function Call" << "\n";
      }
      }
      if (!found_function){
        return LogErrorV(this->loc, "Unknown struct function member called : %s\n", MemberName.c_str());
      }
      std::vector<llvm::Value*> CallArgs;
      CallArgs.push_back(NamedValues[StructName]->alloca_inst);
      for (int i = 0; i < Args.size(); i++){
        CallArgs.push_back(Args.at(i)->codegen());
      }
      Function *F = getFunction(struct_function_mangling(NamedValues[StructName]->type.struct_name, MemberName));
      if (!F){
        Log::Info() << "struct_function_mangling(StructName, MemberName) : " << struct_function_mangling(NamedValues[StructName]->type.struct_name, MemberName) << "\n";
        return LogErrorV(this->loc, "The function member %s called doesn't exist mangled in the scope", MemberName.c_str());
      }
      return Builder->CreateCall(F, CallArgs, "calltmp_struct"); 
    }
    auto members = StructDeclarations[NamedValues[StructName]->type.struct_name]->members;
    Log::Info() << "members.size() : " << members.size() << "\n";
    int pos = -1;
    for (int i = 0; i < members.size(); i++){
      Log::Info() << "members.at(i).first : " << members.at(i).first << " MemberName : " << MemberName << "\n";
      if (members.at(i).first == MemberName){
        pos = i;
        break;
      }
    }
    Cpoint_Type member_type = members.at(pos).second;
    Log::Info() << "Pos for GEP struct member " << pos << "\n";
    auto zero = llvm::ConstantInt::get(*TheContext, llvm::APInt(64, 0, true));
    auto index = llvm::ConstantInt::get(*TheContext, llvm::APInt(32, pos, true));
    Cpoint_Type cpoint_type = NamedValues[StructName]->type;
    //Value* tempval = Builder->CreateLoad(get_type_llvm(cpoint_type), Alloca, StructName);
    if (cpoint_type.is_ptr){
      cpoint_type.is_ptr = false;
    }
    Log::Info() << "cpoint_type struct : " << cpoint_type << "\n";
    
    Value* ptr = Builder->CreateGEP(get_type_llvm(cpoint_type), /*tempval*/ Alloca, { zero, index});
    Value* value = Builder->CreateLoad(get_type_llvm(member_type), ptr, StructName);
    return value;
}

Value* UnionMemberExprAST::codegen(){
  AllocaInst* Alloca = nullptr;
  if (NamedValues[UnionName] == nullptr){
    return LogErrorV(this->loc, "Can't find union that is used for a member");
  }
  Alloca = NamedValues[UnionName]->alloca_inst;
  if (!NamedValues[UnionName]->type.is_union){
    return LogErrorV(this->loc, "Using a member of variable even though it is not an union");
  }
  auto members = UnionDeclarations[NamedValues[UnionName]->type.union_name]->members;
  int pos = -1;
  for (int i = 0; i < members.size(); i++){
    if (members.at(i).first == MemberName){
      pos = i;
      break;
    }
  }
  Cpoint_Type member_type = members.at(pos).second;
  return Builder->CreateLoad(get_type_llvm(member_type), Alloca, "load_union_member");
}

Value* ConstantArrayExprAST::codegen(){
  std::vector<Constant*> arrayMembersVal;
  for (int i = 0; i < ArrayMembers.size(); i++){
    Value* Vtemp = ArrayMembers.at(i)->codegen();
    Constant* constant;
    if (! (constant = dyn_cast<Constant>(Vtemp))){
      return LogErrorV(this->loc, "The %d value of constant array is not a constant", i);
    }
    arrayMembersVal.push_back(constant);
  }
  llvm::ArrayType* arrayType = ArrayType::get(arrayMembersVal.at(0)->getType(), arrayMembersVal.size());
  // verify if everyone of them are constants of the same type
  return llvm::ConstantArray::get(arrayType, arrayMembersVal);
}

Value* ConstantStructExprAST::codegen(){
    std::vector<Constant*> structMembersVal;
    for (int i = 0; i < StructMembers.size(); i++){
        Value* Vtemp = StructMembers.at(i)->codegen();
        if (get_type_llvm(StructDeclarations[struct_name]->members.at(i).second) != Vtemp->getType()){
            convert_to_type(get_cpoint_type_from_llvm(Vtemp->getType()), get_type_llvm(StructDeclarations[struct_name]->members.at(i).second), Vtemp);
        }
        Constant* constant;
        if (! (constant = dyn_cast<Constant>(Vtemp))){
            return LogErrorV(this->loc, "The %d value of constant array is not a constant", i);
        }
        // convert type
        /*if (get_type_llvm(StructDeclarations[struct_name]->members.at(i).second) != constant->getType()){
            convert_to_type(get_cpoint_type_from_llvm(constant->getType()), get_type_llvm(StructDeclarations[struct_name]->members.at(i).second), constant);
        }*/
        structMembersVal.push_back(constant);
    }
    auto structType = StructDeclarations[struct_name]->struct_type;
    return ConstantStruct::get(dyn_cast<StructType>(structType), structMembersVal);
}


// TODO : transform this into an operator to use it in situations like : "self.list[0]"
Value* ArrayMemberExprAST::codegen() {
  Log::Info() << "ARRAY MEMBER CODEGEN" << "\n";
  if (!NamedValues[ArrayName]){
    return LogErrorV(this->loc, "Tried to get a member of an array that doesn't exist : %s", ArrayName.c_str());
  }
  Cpoint_Type cpoint_type = NamedValues[ArrayName]->type;
  auto index = posAST->codegen();
  if (!index){
    return LogErrorV(this->loc, "error in array index");
  }
  Value* firstIndex = index;
  bool is_constant = false;
  if (dyn_cast<Constant>(index) && cpoint_type.nb_element > 0){
    is_constant = true;
    Constant* indexConst = dyn_cast<Constant>(index);
    if (!bound_checking_constant_index_array_member(indexConst, cpoint_type, this->loc)){
      return nullptr;
    }
  }

  index = Builder->CreateFPToUI(index, Type::getInt32Ty(*TheContext), "cast_gep_index");
  if (!is_llvm_type_number(index->getType())){
    return LogErrorV(this->loc, "index for array is not a number\n");
  }
  AllocaInst* Alloca = NamedValues[ArrayName]->alloca_inst;
  auto zero = llvm::ConstantInt::get(*TheContext, llvm::APInt(64, 0, true));
  std::vector<Value*> indexes = { zero, index};
  Log::Info() << "Cpoint_type for array member before : " << cpoint_type << "\n";
  if (cpoint_type.is_ptr && !cpoint_type.is_array){
    Log::Info() << "array for array member is ptr" << "\n";
    if (cpoint_type.nb_ptr > 1){
      cpoint_type.nb_ptr--;
    } else {
      cpoint_type.is_ptr = false;
      cpoint_type.nb_ptr = 0;
      cpoint_type.nb_element = 0;
    }
    Log::Info() << "Cpoint_type for array member which is ptr : " << cpoint_type << "\n";
    indexes = {index};
  }

  Log::Info() << "Cpoint_type for array member : " << cpoint_type << "\n";
  if (!is_constant && cpoint_type.nb_element > 0 && !Comp_context->is_release_mode && Comp_context->std_mode && index){
    if (!bound_checking_dynamic_index_array_member(firstIndex, cpoint_type)){
      return nullptr;
    }
  }
  Cpoint_Type member_type = cpoint_type;
  member_type.is_array = false;
  member_type.nb_element = 0;
  
  Type* type_llvm = get_type_llvm(cpoint_type);
  Value* ptr = Builder->CreateGEP(type_llvm, Alloca, indexes);
  Value* value = Builder->CreateLoad(get_type_llvm(member_type), ptr, ArrayName);
  return value;
}

Type* StructDeclarAST::codegen(){
  Log::Info() << "CODEGEN STRUCT DECLAR" << "\n";
  StructType* structType = StructType::create(*TheContext);
  structType->setName(Name);
  std::vector<Type*> dataTypes;
  std::vector<std::pair<std::string,Cpoint_Type>> members;
  std::vector<std::string> functions;
  for (int i = 0; i < Vars.size(); i++){
    std::unique_ptr<VarExprAST> VarExpr = std::move(Vars.at(i));
    if (!VarExpr){
        Log::Info() << "VarExpr is nullptr" << "\n";
    }
    if (VarExpr->cpoint_type.struct_name == Name && VarExpr->cpoint_type.is_ptr){
        VarExpr->cpoint_type.is_struct = false;
        VarExpr->cpoint_type.struct_name = "";
    }
    Type* var_type = get_type_llvm(VarExpr->cpoint_type);
    dataTypes.push_back(var_type);
    std::string VarName = VarExpr->VarNames.at(0).first;
    members.push_back(std::make_pair(VarName, VarExpr->cpoint_type));
  }
  structType->setBody(dataTypes);
  Log::Info() << "added struct declaration name " << Name << " to StructDeclarations" << "\n";
  StructDeclarations[Name] = std::make_unique<StructDeclaration>(structType, members, functions);
  for (int i = 0; i < Functions.size(); i++){
    std::unique_ptr<FunctionAST> FunctionExpr = std::move(Functions.at(i));
    std::string function_name;
    if (FunctionExpr->Proto->Name == Name){
    // Constructor
    function_name = "Constructor__Default";
    } else {
    function_name = FunctionExpr->Proto->Name;
    }

    std::string mangled_name_function = struct_function_mangling(Name, function_name);
    Cpoint_Type self_pointer_type = get_cpoint_type_from_llvm(structType->getPointerTo());
    self_pointer_type = Cpoint_Type(double_type, true, 0, false, 0, true, Name);
    FunctionExpr->Proto->Args.insert(FunctionExpr->Proto->Args.begin(), std::make_pair("self", self_pointer_type));
    FunctionExpr->Proto->Name = mangled_name_function;
    FunctionExpr->codegen();
    functions.push_back(function_name);  
  }
  for (int i = 0; i < ExternFunctions.size(); i++){
    std::unique_ptr<PrototypeAST> ProtoExpr = std::move(ExternFunctions.at(i));
    std::string function_name;
    if (ProtoExpr->Name == Name){
    // Constructor
    function_name = "Constructor__Default";
    } else {
    function_name = ProtoExpr->Name;
    }
    std::string mangled_name_function = struct_function_mangling(Name, function_name);
    Cpoint_Type self_pointer_type = Cpoint_Type(double_type, true, 1, false, 0, true, Name);
    ProtoExpr->Args.insert(ProtoExpr->Args.begin(), std::make_pair("self", self_pointer_type));
    ProtoExpr->Name = mangled_name_function;
    ProtoExpr->codegen();
    functions.push_back(function_name);
  }

  StructDeclarations[Name] = std::make_unique<StructDeclaration>(structType, std::move(members), std::move(functions));
  Log::Info() << "appending to StructDeclarations struct " << Name << "\n";
  return structType;
}

Type* UnionDeclarAST::codegen(){
  StructType* unionType = StructType::create(*TheContext);
  unionType->setName(Name);
  // take the biggest type and just have it in the struct and bitcast for the other types
  std::vector<std::pair<std::string,Cpoint_Type>> members;
  std::vector<Type*> dataType;
  Cpoint_Type biggest_type = Cpoint_Type(double_type);
  int biggest_type_size = 0;
  for (int i = 0; i < Vars.size(); i++){
    if (get_type_number_of_bits(Vars.at(i)->cpoint_type) > biggest_type_size){
      biggest_type = Vars.at(i)->cpoint_type;
      biggest_type_size = get_type_number_of_bits(Vars.at(i)->cpoint_type);
    }
  }
  for (int i = 0; i < Vars.size(); i++){
    std::unique_ptr<VarExprAST> VarExpr = std::move(Vars.at(i));
    Type* var_type = get_type_llvm(VarExpr->cpoint_type);
    std::string VarName = VarExpr->VarNames.at(0).first;
    members.push_back(std::make_pair(VarName, VarExpr->cpoint_type));
  }
  dataType.push_back(get_type_llvm(biggest_type));
  unionType->setBody(dataType);
  UnionDeclarations[Name] = std::make_unique<UnionDeclaration>(unionType, members);
  return unionType;
}

Type* EnumDeclarAST::codegen(){
    Type* enumType = nullptr;
    // TODO : add this optimization for enum without tag later (you need to verify and change the code in match to include this possibility)
    /*if (!enum_member_contain_type){
        enumType = get_type_llvm(int_type);
        EnumDeclarations[Name] = std::make_unique<EnumDeclaration>(enumType, std::move(this->clone()));
        return nullptr;
    }*/
    Log::Info() << "codegen EnumDeclarAST with type" << "\n";
    std::vector<Type*> dataType;
    StructType* enumStructType = StructType::create(*TheContext);
    enumStructType->setName(Name);
    dataType.push_back(get_type_llvm(Cpoint_Type(int_type)));
    Log::Info() << "EnumDeclarAST TEST" << "\n";
    int biggest_type_size = 0;
    Cpoint_Type biggest_type = Cpoint_Type(double_type);
    for (int i = 0; i < EnumMembers.size(); i++){
        if (EnumMembers.at(i)->Type){
        if (get_type_number_of_bits(*(EnumMembers.at(i)->Type)) > biggest_type_size){
            biggest_type = *(EnumMembers.at(i)->Type);
            biggest_type_size = get_type_number_of_bits(*(EnumMembers.at(i)->Type));
        }
        }
    }
    dataType.push_back(get_type_llvm(biggest_type));
    enumStructType->setBody(dataType);
    EnumDeclarations[Name] = std::make_unique<EnumDeclaration>(enumStructType, std::move(this->clone()));
    return enumStructType;
}

// TODO : add the creation of a struct here (struct {tag, value} in pseudo code). Maybe even replace it for the current implementation in RedeclarationExprAST
Value* EnumCreation::codegen(){
    return nullptr;
}

void string_vector_erase(std::vector<std::string>& strings, std::string string){
    std::vector<std::string>::iterator iter = strings.begin();
    while (iter != strings.end())
    {
        if (*iter == string){
            iter = strings.erase(iter);
        }
        else {
           ++iter;
        }
    }

}

Value* MatchExprAST::codegen(){
    Function *TheFunction = Builder->GetInsertBlock()->getParent();
    if (NamedValues[matchVar] == nullptr){
        return LogErrorV(this->loc, "Match var is unknown");
    }
    auto enumType = EnumDeclarations[NamedValues[matchVar]->type.enum_name]->enumType;
    auto enumDeclar = EnumDeclarations[NamedValues[matchVar]->type.enum_name]->EnumDeclar->clone();
    auto zero = llvm::ConstantInt::get(*TheContext, llvm::APInt(64, 0, true));
    auto index_tag = llvm::ConstantInt::get(*TheContext, llvm::APInt(32, 0, true));
    auto index_val = llvm::ConstantInt::get(*TheContext, llvm::APInt(32, 1, true));
    Value* tag_ptr = Builder->CreateGEP(get_type_llvm(NamedValues[matchVar]->type), NamedValues[matchVar]->alloca_inst, { zero, index_tag }, "tag_ptr");
    Value* val_ptr = Builder->CreateGEP(get_type_llvm(NamedValues[matchVar]->type), NamedValues[matchVar]->alloca_inst, { zero, index_val }, "val_ptr");
    //Value* tag_ptr = NamedValues[matchVar]->alloca_inst;
    Value* tag = Builder->CreateLoad(get_type_llvm(int_type), tag_ptr, matchVar);
    BasicBlock *AfterMatch = BasicBlock::Create(*TheContext, "after_match");
    std::vector<std::string> membersNotFound;
    for (int i = 0; i < enumDeclar->EnumMembers.size(); i++){
        membersNotFound.push_back(enumDeclar->EnumMembers.at(i)->Name);
    }
    for (int i = 0; i < matchCases.size(); i++){
        std::unique_ptr<matchCase> matchCaseTemp = matchCases.at(i)->clone();
        if (matchCaseTemp->is_underscore){
            //Builder->CreateBr(AfterMatch);
            for (int i = 0; i < matchCaseTemp->Body.size(); i++){
                matchCaseTemp->Body.at(i)->codegen();
            }
            membersNotFound.clear();
            break;
        } else {
        string_vector_erase(membersNotFound, matchCaseTemp->enum_member);
        if (matchCaseTemp->enum_name != NamedValues[matchVar]->type.enum_name){
            return LogErrorV(this->loc, "The match case is using a member of a different enum than the one in the expression");
        }
        int pos = -1;
        for (int j = 0; j < enumDeclar->EnumMembers.size(); j++){
            if (enumDeclar->EnumMembers[j]->Name == matchCaseTemp->enum_member){
                pos = j;
            }
        }
        if (pos == -1){
            return LogErrorV(this->loc, "Couldn't find the member of this enum in match case");
        }
        //enumDeclar->EnumMembers[pos]
        //enumDeclar->EnumMembers[pos]
        Value* cmp = operators::LLVMCreateCmp(tag, ConstantInt::get(get_type_llvm(int_type), APInt(32, (uint64_t)pos)));
        cmp = Builder->CreateFCmpONE(cmp, ConstantFP::get(*TheContext, APFloat(0.0)), "ifcond");
        BasicBlock *ThenBB = BasicBlock::Create(*TheContext, "then_match", TheFunction);
        BasicBlock *ElseBB = BasicBlock::Create(*TheContext, "else_match");
        Builder->CreateCondBr(cmp, ThenBB, ElseBB);
        Builder->SetInsertPoint(ThenBB);
        if (matchCaseTemp->var_name != ""){
            if (!enumDeclar->EnumMembers[pos]->contains_value){
                return LogErrorV(this->loc, "Enum Member doesn't contain a value");
            }
            AllocaInst *Alloca = CreateEntryBlockAlloca(TheFunction, matchCaseTemp->var_name, *enumDeclar->EnumMembers[pos]->Type);
            Value* enum_val = Builder->CreateLoad(get_type_llvm(*enumDeclar->EnumMembers[pos]->Type), val_ptr, "enum_val_load");
            Builder->CreateStore(enum_val, Alloca);
            NamedValues[matchCaseTemp->var_name] = std::make_unique<NamedValue>(Alloca, *enumDeclar->EnumMembers[pos]->Type);
            Log::Info() << "Create var for match : " << enumDeclar->EnumMembers[pos]->Name << "\n";
            //enumDeclar->EnumMembers[pos]->
        }
        //enumDeclar->EnumMembers[pos]
        for (int i = 0; i < matchCaseTemp->Body.size(); i++){
            matchCaseTemp->Body.at(i)->codegen();
        }
        Builder->CreateBr(AfterMatch);
        //Builder->CreateBr(ElseBB);
        //TheFunction->getBasicBlockList().push_back(ElseBB);
        TheFunction->insert(TheFunction->end(), ElseBB);
        Builder->SetInsertPoint(ElseBB);
        }
    }
    if (!membersNotFound.empty()){
        std::string list_members_not_found_str = "";
        int i;
        for (i = 0; i < membersNotFound.size()-1; i++){
            list_members_not_found_str += membersNotFound.at(i) + ", ";
        }
        list_members_not_found_str += membersNotFound.at(i);
        return LogErrorV(this->loc, "These members were not found in the match case : %s\n", list_members_not_found_str.c_str());
    }
    Builder->CreateBr(AfterMatch);
    //TheFunction->getBasicBlockList().push_back(AfterMatch);
    TheFunction->insert(TheFunction->end(), AfterMatch);
    Builder->SetInsertPoint(AfterMatch);
    return Constant::getNullValue(get_type_llvm(double_type));
}

#if ARRAY_MEMBER_OPERATOR_IMPL
Value* getArrayMember(Value* array, Value* index){
    Log::Info() << "ARRAY MEMBER CODEGEN" << "\n";
    if (!index){
        return LogErrorV(emptyLoc, "error in array index");
    }
    std::string ArrayName = (std::string)array->getName();
    auto type_llvm = array->getType();
    auto zero = llvm::ConstantInt::get(*TheContext, llvm::APInt(64, 0, true));
    Value* firstIndex = index;
    bool is_constant = false;
    // TODO : for now this verification is not compatible
    /*if (dyn_cast<Constant>(index) && cpoint_type.nb_element > 0){
        is_constant = true;
        Constant* indexConst = dyn_cast<Constant>(index);
        if (!bound_checking_constant_index_array_member(indexConst, cpoint_type, this->loc)){
            return nullptr;
        }
    }*/
    index = Builder->CreateFPToUI(index, Type::getInt32Ty(*TheContext), "cast_gep_index");
    if (!is_llvm_type_number(index->getType())){
        return LogErrorV(emptyLoc, "index for array is not a number\n");
    }

    // TODO : same as above check
    /*if (!is_constant && cpoint_type.nb_element > 0 && !Comp_context->is_release_mode && Comp_context->std_mode && index){
    if (!bound_checking_dynamic_index_array_member(firstIndex, cpoint_type)){
      return nullptr;
    }
    }*/
    
    std::vector<Value*> indexes = { zero, index};
    
    
    Value* ptr = Builder->CreateGEP(type_llvm, array, indexes);
    auto member_type_llvm = array->getType()->getArrayElementType();
    Value* value = Builder->CreateLoad(member_type_llvm, ptr, ArrayName);
    return value;
}
#endif

Value *BinaryExprAST::codegen() {
  Value *L = LHS->codegen();
  Value *R = RHS->codegen();
  if (!L || !R)
    return nullptr;
  if (L->getType() != R->getType()){
    Log::Warning(this->loc) << "Types are not the same for the binary operation '" << Op << "' to the " << create_pretty_name_for_type(get_cpoint_type_from_llvm(L->getType())) << " and " << create_pretty_name_for_type(get_cpoint_type_from_llvm(R->getType())) << " types" << "\n";
    convert_to_type(get_cpoint_type_from_llvm(R->getType()), L->getType(), R);
  }
  // TODO : make every operators compatibles with ints and other types. Possibly also refactor this in multiple function in maybe a dedicated operators.cpp file
  if (Op == "=="){
    return operators::LLVMCreateCmp(L, R);
  }
  if (Op == "||"){
    return operators::LLVMCreateLogicalOr(L, R);
  }
  if (Op == "&&"){
    return operators::LLVMCreateLogicalAnd(L, R);
  }
  if (Op == "!="){
    return operators::LLVMCreateNotEqualCmp(L, R);
  }
  if (Op == "<="){
    return operators::LLVMCreateSmallerOrEqualThan(L, R);
  }
  if (Op == ">="){
    return operators::LLVMCreateGreaterOrEqualThan(L, R);
  }
  if (Op == ">>"){
    return Builder->CreateLShr(L, R, "shiftrtmp");
  }
  if (Op == "<<"){
    return Builder->CreateShl(L, R, "shiftltmp");
  }
  switch (Op.at(0)) {
  case '+':
    return operators::LLVMCreateAdd(L, R);
  case '-':
    return operators::LLVMCreateSub(L, R);
  case '*':
    return operators::LLVMCreateMul(L, R);
  case '%':
    return operators::LLVMCreateRem(L, R);
  case '<':
    return operators::LLVMCreateSmallerThan(L, R);
  case '/':
    return operators::LLVMCreateDiv(L, R);
  case '>':
    return operators::LLVMCreateGreaterThan(L, R);
  case '^':
    L = Builder->CreateXor(L, R, "xortmp");
    return Builder->CreateUIToFP(L, Type::getDoubleTy(*TheContext), "booltmp");
  case '|':
    L = Builder->CreateOr(L, R, "ortmp");
    return Builder->CreateUIToFP(L, Type::getDoubleTy(*TheContext), "booltmp");
#if ARRAY_MEMBER_OPERATOR_IMPL
  case '[':
    // TODO get from rhs and lhs with getelementptr the array member
    return getArrayMember(L, R);
#endif
  case '=': {
    VariableExprAST *LHSE = static_cast<VariableExprAST *>(LHS.get());
    if (!LHSE)
      return LogErrorV(this->loc, "destination of '=' must be a variable");
    Value *Val = RHS->codegen();
    if (!Val)
      return nullptr;
    Value *Variable = NamedValues[LHSE->getName()]->alloca_inst;
    if (!Variable)
      return LogErrorV(this->loc, "Unknown variable name");
    Builder->CreateStore(Val, Variable);
    return Val;
  } break;
  default:
    //return LogErrorV(this->loc, "invalid binary operator");
    break;
  }
  Function *F = getFunction(std::string("binary") + Op);
  assert(F && "binary operator not found!");
  Value *Ops[2] = { L, R };
  return Builder->CreateCall(F, Ops, "binop");
}

Value *CallExprAST::codegen() {
  // Look up the name in the global module table.
  Log::Info() << "function called " << Callee << "\n";
  std::string internal_func_prefix = "cpoint_internal_";
  bool is_internal = false;
  CpointDebugInfo.emitLocation(this);
  if (StructDeclarations[Callee] != nullptr){
    // TODO : maybe replace struct_test(1, 2) by struct_test {1, 2} ? so maybe remove this
    // function called is constructor of struct
    std::string constructor_name = struct_function_mangling(Callee, "Constructor__Default");
    if (FunctionProtos[constructor_name] != nullptr){
        Callee = constructor_name;
    } else {
        //std::make_unique<GlobalVariableAST>("__const_struct_" + Callee, true, false, Cpoint_Type(double_type, false, 0, false, 0, true, Callee), );
        std::vector<Constant*> constantArgs;
        for (int i = 0; i < Args.size(); i++){
            auto Val = Args.at(i)->codegen();
            auto ConstantVal = dyn_cast<Constant>(Val);
            if (!ConstantVal){
                return LogErrorV(this->loc, "You can't initialize a struct without a constructor with non constant values");
            }
            constantArgs.push_back(ConstantVal);
        }
        return ConstantStruct::get(dyn_cast<StructType>(StructDeclarations[Callee]->struct_type), ArrayRef<Constant*>(constantArgs));
    }
  }
  if (Callee.rfind(internal_func_prefix, 0) == 0){
    is_internal = true;
    Callee = Callee.substr(internal_func_prefix.size(), Callee.size());
    Log::Info() << "internal function called " << Callee << "\n";
    if (Callee.rfind("llvm_", 0) == 0){
      return callLLVMIntrisic(Callee, Args);
    }
    if (Callee == "get_filename"){
      std::string filename_without_temp = filename;
      int temp_pos;
      if ((temp_pos = filename.rfind(".temp")) != std::string::npos){
        filename_without_temp = filename.substr(0, temp_pos);
      }
      return StringExprAST(filename_without_temp).codegen();
    }
    if (Callee == "get_function_name"){
      Function *TheFunction = Builder->GetInsertBlock()->getParent();
      return StringExprAST((std::string)TheFunction->getName()).codegen();
    }
  }
  bool is_function_template = TemplateProtos[Callee] != nullptr;
  if (is_function_template){
    callTemplate(Callee, template_passed_type);
  }
  Function *CalleeF = getFunction(Callee);
  //Function *TheFunction = Builder->GetInsertBlock()->getParent();
  Log::Info() << "is_function_template : " << is_function_template << "\n";
  if (!CalleeF && !is_function_template)
    return LogErrorV(this->loc, "Unknown function referenced %s", Callee.c_str());
  /*if (TemplateProtos[Callee] != nullptr){
    callTemplate(Callee);
  }*/
  Log::Info() << "CalleeF->arg_size : " << CalleeF->arg_size() << "\n";
  Log::Info() << "Args.size : " << Args.size() << "\n";
  if (FunctionProtos[Callee] == nullptr){
    return LogErrorV(this->loc, "Incorrect Function %s", Callee.c_str());
  }
  if (FunctionProtos[Callee]->is_variable_number_args){
    Log::Info() << "Variable number of args" << "\n";
    if (!(Args.size() >= CalleeF->arg_size())){
      return LogErrorV(this->loc, "Incorrect number of arguments passed : %d args but %d expected", Args.size(), CalleeF->arg_size());
    }
  } else {
    // If argument mismatch error.
  if (CalleeF->arg_size() != Args.size())
    return LogErrorV(this->loc, "Incorrect number of arguments passed : %d args but %d expected", Args.size(), CalleeF->arg_size());
  }
  std::vector<Value *> ArgsV;
  for (unsigned i = 0, e = Args.size(); i != e; ++i) {
    Value* temp_val = Args[i]->codegen();
    if (!temp_val){
      return nullptr;
    }
    if (i < FunctionProtos[Callee]->Args.size()){
    if (temp_val->getType() != get_type_llvm(FunctionProtos[Callee]->Args.at(i).second)){
      Log::Info() << "name of arg converting in call expr : " << FunctionProtos[Callee]->Args.at(i).first << "\n";
      //return LogErrorV(this->loc, "Arg %s type is wrong in the call of %s\n Expected type : %s, got type : %s\n", FunctionProtos[Callee]->Args.at(i).second, Callee, create_pretty_name_for_type(get_cpoint_type_from_llvm(temp_val->getType())), create_pretty_name_for_type(FunctionProtos[Callee]->Args.at(i).second));
        convert_to_type(get_cpoint_type_from_llvm(temp_val->getType()) , get_type_llvm(FunctionProtos[Callee]->Args.at(i).second), temp_val);
    }
    }
    ArgsV.push_back(temp_val);
    if (!ArgsV.back())
      return nullptr;
  }

  return Builder->CreateCall(CalleeF, ArgsV, "calltmp");
}

Value* AddrExprAST::codegen(){
  AllocaInst *A = NamedValues[Name]->alloca_inst;
  if (!A)
    return LogErrorV(this->loc, "Addr Unknown variable name %s", Name.c_str());
  return Builder->CreateLoad(PointerType::get(A->getAllocatedType(), A->getAddressSpace()), A, Name.c_str());
}

Value* SizeofExprAST::codegen(){
  Log::Info() << "codegen sizeof" << "\n";
  //auto zero = llvm::ConstantInt::get(*TheContext, llvm::APInt(64, 0, true));
  auto one = llvm::ConstantInt::get(*TheContext, llvm::APInt(32, 1, true));
  if (/*is_type(Name)*/!is_variable){
    Log::Info() << "codegen sizeof is type" << "\n";
    //int type = get_type(Name);
    Cpoint_Type cpoint_type = /*Cpoint_Type(type)*/ type;
    if (cpoint_type.is_struct){
        
        //return getSizeOfStruct(temp_alloca);
    }
    Type* llvm_type = get_type_llvm(cpoint_type);
    Value* size = Builder->CreateGEP(llvm_type, Builder->CreateIntToPtr(ConstantInt::get(Builder->getInt64Ty(), 0),llvm_type->getPointerTo()), {one});
    size =  Builder->CreatePtrToInt(size, get_type_llvm(Cpoint_Type(int_type)));
    //size  = Builder->CreateFPToUI(size, get_type_llvm(Cpoint_Type(int_type)), "cast");
    // size found is in bytes but we want the number of bits
    //size = Builder->CreateMul(size, ConstantInt::get(*TheContext, APInt(32, 8, false)), "mul_converting_in_bits");
    return size;
  } else {
    Log::Info() << "codegen sizeof is variable" << "\n";
  AllocaInst *A = NamedValues[Name]->alloca_inst;
  if (!A){
    return LogErrorV(this->loc, "Addr Unknown variable name %s", Name.c_str());
  }
  if (NamedValues[Name]->type.is_struct && !NamedValues[Name]->type.is_ptr){
    Value* size =  getSizeOfStruct(A);
    size = Builder->CreateMul(size, ConstantInt::get(*TheContext, APInt(32, 8, false)), "mul_converting_in_bits");
    return size;
    /*std::vector<Value*> ArgsV;
    ArgsV.push_back(A);
    ArgsV.push_back(ConstantInt::get(*TheContext, llvm::APInt(1, 0, true))); // return -1 if object size is unknown
    ArgsV.push_back(ConstantInt::get(*TheContext, llvm::APInt(1, 1, true))); // return unknown size if null is passed
    ArgsV.push_back(ConstantInt::get(*TheContext, llvm::APInt(1, 0, true))); // is the calculation made at runtime
    std::vector<Type*> OverloadedTypes;
    OverloadedTypes.push_back(get_type_llvm(int_type));
    OverloadedTypes.push_back(get_type_llvm(i64_type));
    Function *CalleeF = Intrinsic::getDeclaration(TheModule.get(), Intrinsic::objectsize, OverloadedTypes);
    return Builder->CreateCall(CalleeF, ArgsV, "sizeof_calltmp");*/
  }
  Type* llvm_type = get_type_llvm(NamedValues[Name]->type);
  Value* size = Builder->CreateGEP(llvm_type, Builder->CreateIntToPtr(ConstantInt::get(Builder->getInt64Ty(), 0),llvm_type->getPointerTo()), {one});
  size = Builder->CreatePtrToInt(size, get_type_llvm(Cpoint_Type(int_type)));
  //size = Builder->CreateFPToUI(size, get_type_llvm(Cpoint_Type(int_type)), "cast");
  // size found is in bytes but we want the number of bits
  size = Builder->CreateMul(size, ConstantInt::get(*TheContext, APInt(32, 8, false)), "mul_converting_in_bits");
  return size;
  } 
}

Value* TypeidExprAST::codegen(){
    Value* valueLLVM = val->codegen();
    return getTypeId(valueLLVM);
}

Value* CastExprAST::codegen(){
    Value* val = ValToCast->codegen();
    convert_to_type(get_cpoint_type_from_llvm(val->getType()), get_type_llvm(type), val);
    return val;
}

Value* NullExprAST::codegen(){
    return ConstantPointerNull::get(PointerType::get(*TheContext, 0));
}

Value* BoolExprAST::codegen(){
  if (val){
    return ConstantInt::get(*TheContext, APInt(8, 1, true));
  } else {
    return ConstantInt::get(*TheContext, APInt(8, 0, true));
  }
}

Value* CharExprAST::codegen(){
  Log::Info() << "Char Codegen : " << c << "\n";
  return ConstantInt::get(*TheContext, APInt(8, c, true));
}

void TestAST::codegen(){
  Log::Info() << "Codegen test " << this->description << "\n";
  
  testASTNodes.push_back(this->clone());
}

void afterAllTests(){
  if (!Comp_context->test_mode /*|| testASTNodes.empty()*/){
    return;
  }
  std::vector<std::pair<std::string,Cpoint_Type>> Args;
  Args.push_back(std::make_pair("argc", Cpoint_Type(double_type, false)));
  Args.push_back(std::make_pair("argv",  Cpoint_Type(argv_type, false)));
  auto Proto = std::make_unique<PrototypeAST>(emptyLoc, "main", Args, Cpoint_Type(double_type));
  std::vector<std::unique_ptr<ExprAST>> Body;
  if (/*std_mode &&*/ Comp_context->gc_mode){
    std::vector<std::unique_ptr<ExprAST>> Args_gc_init;
    auto E_gc_init = std::make_unique<CallExprAST>(emptyLoc, "gc_init", std::move(Args_gc_init), Cpoint_Type(double_type));
    Body.push_back(std::move(E_gc_init));
  }
  Log::Info() << "testASTNodes size : " << testASTNodes.size() << "\n";
  std::unique_ptr<ExprAST> printfCall;
  std::vector<std::unique_ptr<ExprAST>> ArgsPrintf;
  for (int i = 0; i < testASTNodes.size(); i++){
    // create print for test description
    ArgsPrintf.clear();
    std::string temp_description = testASTNodes.at(i)->description;
    std::unique_ptr<ExprAST> strExprAST;
    strExprAST = std::make_unique<StringExprAST>("Test %s running (in %s)\n");
    ArgsPrintf.push_back(std::move(strExprAST));
    strExprAST = std::make_unique<StringExprAST>(temp_description);
    ArgsPrintf.push_back(std::move(strExprAST));
    strExprAST = std::make_unique<StringExprAST>(first_filename);
    ArgsPrintf.push_back(std::move(strExprAST));
    printfCall = std::make_unique<CallExprAST>(emptyLoc, "printf", std::move(ArgsPrintf), Cpoint_Type(double_type));
    Body.push_back(std::move(printfCall));
    for (int j = 0; j < testASTNodes.at(i)->Body.size(); j++){
      Body.push_back(std::move(testASTNodes.at(i)->Body.at(j)));
    }
  }
  Body.push_back(std::make_unique<NumberExprAST>(0));
  
  auto funcAST = std::make_unique<FunctionAST>(std::move(Proto), std::move(Body));
  funcAST->codegen();
}

Function *PrototypeAST::codegen() {
  // Make the function type:  double(double,double) etc.
  //std::vector<Type *> Doubles(Args.size(), Type::getDoubleTy(*TheContext));
  std::vector<Type *> type_args;
  for (int i = 0; i < Args.size(); i++){
    //if (Args.at(i).first != "..."){
    type_args.push_back(get_type_llvm(Args.at(i).second));
    //}
  }
  FunctionType *FT;
  //FunctionType *FT = FunctionType::get(Type::getDoubleTy(*TheContext), Doubles, false);
  if (Name == "main"){
  std::vector<Type*> args_type_main;
  args_type_main.push_back(get_type_llvm(Cpoint_Type(-2)));
  args_type_main.push_back(get_type_llvm(Cpoint_Type(-4, true))->getPointerTo());
  FT = FunctionType::get(/*get_type_llvm(cpoint_type)*/ get_type_llvm(Cpoint_Type(int_type)), args_type_main, false);
  } else {
  FT = FunctionType::get(get_type_llvm(cpoint_type), type_args, is_variable_number_args);
  }
  Function *F =
      Function::Create(FT, Function::ExternalLinkage, Name, TheModule.get());

  // Set names for all arguments.
  unsigned Idx = 0;
  for (auto &Arg : F->args()){
    //if (Args[Idx++].first != "..."){
    Arg.setName(Args[Idx++].first);
    //}
  }
  FunctionProtos[this->getName()] = this->clone();
  GeneratedFunctions[this->getName()] = F;
  return F;
}

Function *FunctionAST::codegen() {
  auto &P = *Proto;
  Log::Info() << "FunctionAST Codegen : " << Proto->getName() << "\n";
  FunctionProtos[Proto->getName()] = Proto->clone();
  // First, check for an existing function from a previous 'extern' declaration.
  std::string name = P.getName();
  Log::Info() << "Name " << name << "\n";
  Function *TheFunction = getFunction(P.getName());
  /*if (!TheFunction)
    TheFunction = Proto->codegen();*/

  if (!TheFunction)
    return nullptr;
  if (P.isBinaryOp()){
    std::string op = "";
    op += P.getOperatorName();
    BinopPrecedence[op] = P.getBinaryPrecedence();
  }

  // Create a new basic block to start insertion into.
  BasicBlock *BB = BasicBlock::Create(*TheContext, "entry", TheFunction);
  Builder->SetInsertPoint(BB);
  DIFile *Unit = nullptr;
  DIScope *FContext;
  DISubprogram *SP = nullptr;
  unsigned LineNo = 0;
  LineNo = P.getLine();
  Log::Info() << "LineNo after P.getLine : " << LineNo << "\n";
  if (debug_info_mode){
  Unit = DBuilder->createFile(CpointDebugInfo.TheCU->getFilename(),
                                      CpointDebugInfo.TheCU->getDirectory());
  FContext = Unit;
  unsigned ScopeLine = 0;
  SP = DBuilder->createFunction(
    FContext, P.getName(), StringRef(), Unit, LineNo,
    DebugInfoCreateFunctionType(P.cpoint_type, P.Args),
    ScopeLine,
    DINode::FlagPrototyped,
    DISubprogram::SPFlagDefinition);
  TheFunction->setSubprogram(SP);
  CpointDebugInfo.LexicalBlocks.push_back(SP);
  CpointDebugInfo.emitLocation(nullptr);
  }

  // Record the function arguments in the NamedValues map.
  NamedValues.clear();
  // clean the map for Template types
  unsigned ArgIdx = 0;
  if (P.getName() == "main"){
    int i = 0;
    for (auto &Arg : TheFunction->args()){
      Cpoint_Type type = Cpoint_Type(double_type, false);
      if (i == 1){
        //type = argv_type;
        type = Cpoint_Type(i8_type, true, 2);
      } else {
        //type = int_type;
        type = Cpoint_Type(int_type);
      }
      AllocaInst *Alloca = CreateEntryBlockAlloca(TheFunction, Arg.getName(), /*Cpoint_Type(type, false)*/ type);
      if (debug_info_mode){
      debugInfoCreateParameterVariable(SP, Unit, Alloca, /*Cpoint_Type(type, false)*/ type, Arg, ArgIdx, LineNo);
      }
      /*DILocalVariable *D = DBuilder->createParameterVariable(
      SP, Arg.getName(), ++ArgIdx, Unit, LineNo, CpointDebugInfo.getDoubleTy(),
      true);

      DBuilder->insertDeclare(Alloca, D, DBuilder->createExpression(),
                          DILocation::get(SP->getContext(), LineNo, 0, SP),
                          Builder->GetInsertBlock());*/
      Builder->CreateStore(&Arg, Alloca);
      NamedValues[std::string(Arg.getName())] = std::make_unique<NamedValue>(Alloca, /*Cpoint_Type(type, false)*/ type);
      i++;
    }
  } else {
  int i = 0;
  unsigned ArgIdx = 0;
  for (auto &Arg : TheFunction->args()){
    Cpoint_Type cpoint_type_arg = P.Args.at(i).second;
    Log::Info() << "cpoint_type_arg : " << cpoint_type_arg << "\n";
    AllocaInst *Alloca = CreateEntryBlockAlloca(TheFunction, Arg.getName(), cpoint_type_arg);
    debugInfoCreateParameterVariable(SP, Unit, Alloca, cpoint_type_arg, Arg, ArgIdx, LineNo);
    Builder->CreateStore(&Arg, Alloca);
    Log::Info() << "added arg " << (std::string)Arg.getName() << " to NamedValues" << "\n";
    NamedValues[std::string(Arg.getName())] = std::make_unique<NamedValue>(Alloca, cpoint_type_arg);
    i++;
  }
  }
  if (debug_info_mode){
  for (int i = 0; i < Body.size(); i++){
  CpointDebugInfo.emitLocation(Body.at(i).get());
  }
  }
  codegenStructTemplates();
  Value *RetVal = nullptr;
  //std::cout << "BODY SIZE : " << Body.size() << std::endl;
  for (int i = 0; i < Body.size(); i++){
    RetVal = Body.at(i)->codegen();
  }
  // 
  if (RetVal) {
    // Finish off the function.
    if (RetVal->getType() == get_type_llvm(void_type) && TheFunction->getReturnType() != get_type_llvm(void_type)){
        if (P.getName() == "main"){
            RetVal = ConstantInt::get(*TheContext, APInt(32, 0, true));
            // TODO : maybe do an error if main function doesn't return an int and verify it after converting instead of createing the return 0
        } else {
            Log::Warning(emptyLoc) << "Missing return value in function (the return value is void)" << "\n";
        }
    }

    if (RetVal->getType() != get_type_llvm(void_type) && RetVal->getType() != TheFunction->getReturnType()){
      convert_to_type(get_cpoint_type_from_llvm(RetVal->getType()), TheFunction->getReturnType(), RetVal);
    }
    if (RetVal->getType() != TheFunction->getReturnType()){
        //return LogErrorF(emptyLoc, "Return type is wrong in the %s function", P.getName().c_str());
        Log::Warning(emptyLoc) << "Return type is wrong in the " << P.getName() << " function" << "\n" << "Expected type : " << create_pretty_name_for_type(get_cpoint_type_from_llvm(TheFunction->getReturnType())) << ", got type : " << create_pretty_name_for_type(get_cpoint_type_from_llvm(RetVal->getType())) << "\n";
    }

    Builder->CreateRet(RetVal);
    CpointDebugInfo.LexicalBlocks.pop_back();
    // Validate the generated code, checking for consistency.
    verifyFunction(*TheFunction);
    return TheFunction;
  }

  // Error reading body, remove function.
  TheFunction->eraseFromParent();
  if (P.isBinaryOp()){
    std::string op = "";
    op += P.getOperatorName();
    BinopPrecedence.erase(op);
  }
  if (debug_info_mode){
  CpointDebugInfo.LexicalBlocks.pop_back();
  }
  return LogErrorF(emptyLoc, "Return Value failed for the function");
}

void TypeDefAST::codegen(){
  return;
}

void ModAST::codegen(){
  return;
}

Value* CommentExprAST::codegen(){
    return Constant::getNullValue(Type::getDoubleTy(*TheContext));
}

GlobalVariable* GlobalVariableAST::codegen(){
  Constant* InitVal = nullptr;
  if (is_extern && Init){
    return LogErrorGLLVM("Init Value found even though the global variable is extern");
  }
  if (!is_extern) {
  InitVal = get_default_constant(cpoint_type);
  if (Init){
    //InitVal = dyn_cast<ConstantFP>(Init->codegen());
    //InitVal = from_val_to_constant_infer(Init->codegen());
    InitVal = from_val_to_constant(Init->codegen(), cpoint_type);
    if (!InitVal){
        return LogErrorGLLVM("The constant initialization of the global variable wasn't a double so it couldn't be converted to a constant");
    }
    //InitVal = nullptr;
  }
  }
  GlobalValue::LinkageTypes linkage = GlobalValue::ExternalLinkage;
  GlobalVariable* globalVar = new GlobalVariable(*TheModule, get_type_llvm(cpoint_type), is_const, linkage, InitVal, varName);
  //GlobalVariable* globalVar = new GlobalVariable(*TheModule, get_type_llvm(cpoint_type), is_const, linkage, InitVal, varName, nullptr, llvm::GlobalValue::NotThreadLocal, llvm::None, is_extern);
  //globalVar->setExternallyInitialized(is_extern); // TODO : implement extern global variables
  GlobalVariables[varName] = std::make_unique<GlobalVariableValue>(cpoint_type, globalVar);
  return globalVar;
}

Value *IfExprAST::codegen() {
  CpointDebugInfo.emitLocation(this);
  Value *CondV = Cond->codegen();
  if (!CondV)
    return nullptr;

  // Convert condition to a bool by comparing non-equal to 0.0.
  CondV = Builder->CreateFCmpONE(
      CondV, ConstantFP::get(*TheContext, APFloat(0.0)), "ifcond");

  Function *TheFunction = Builder->GetInsertBlock()->getParent();

  Type* phiType = Type::getDoubleTy(*TheContext);
  // TODO : replace the phitype from the function return type to the type of the ThenV
  phiType = TheFunction->getReturnType();

  // Create blocks for the then and else cases.  Insert the 'then' block at the
  // end of the function.
  BasicBlock *ThenBB = BasicBlock::Create(*TheContext, "then", TheFunction);
  BasicBlock *ElseBB = BasicBlock::Create(*TheContext, "else");
  BasicBlock *MergeBB = BasicBlock::Create(*TheContext, "ifcont");
  Builder->CreateCondBr(CondV, ThenBB, ElseBB);

  // Emit then value.
  Builder->SetInsertPoint(ThenBB);
  Value *ThenV = nullptr;
  for (int i = 0; i < Then.size(); i++){
    ThenV = Then.at(i)->codegen();
    if (!ThenV)
      return nullptr;
  }
  if (ThenV->getType() != Type::getVoidTy(*TheContext) && ThenV->getType() != phiType){
    convert_to_type(get_cpoint_type_from_llvm(ThenV->getType()), phiType, ThenV);
  }

  Builder->CreateBr(MergeBB);
  // Codegen of 'Then' can change the current block, update ThenBB for the PHI.
  ThenBB = Builder->GetInsertBlock();
  Value *ElseV = nullptr;
  // Emit else block.
  //TheFunction->getBasicBlockList().push_back(ElseBB);
  TheFunction->insert(TheFunction->end(), ElseBB);
  Builder->SetInsertPoint(ElseBB);
  if (!Else.empty()){
  for (int i = 0; i < Else.size(); i++){
    ElseV = Else.at(i)->codegen();
    if (!ElseV)
      return nullptr;
  }
  if (ElseV->getType() != Type::getVoidTy(*TheContext) && ElseV->getType() != phiType){
    convert_to_type(get_cpoint_type_from_llvm(ElseV->getType()), phiType, ElseV);
  }

  } /*else {
    Log::Info() << "Else empty" << "\n";
    ElseV = ConstantFP::get(*TheContext, APFloat(0.0));
  }*/
 

  Builder->CreateBr(MergeBB);
  // Codegen of 'Else' can change the current block, update ElseBB for the PHI.
  ElseBB = Builder->GetInsertBlock();

  // Emit merge block.
  //TheFunction->getBasicBlockList().push_back(MergeBB);
  TheFunction->insert(TheFunction->end(), MergeBB);
  Builder->SetInsertPoint(MergeBB);

  PHINode *PN = Builder->CreatePHI(phiType, 2, "iftmp");

  if (ThenV->getType() == Type::getVoidTy(*TheContext) && phiType != Type::getVoidTy(*TheContext)){
    //ThenV = ConstantFP::get(*TheContext, APFloat(0.0));
    ThenV = get_default_constant(get_cpoint_type_from_llvm(phiType));
  }

  if (ElseV == nullptr || ElseV->getType() == Type::getVoidTy(*TheContext)){
    if (phiType != Type::getVoidTy(*TheContext)){
    //ElseV = ConstantFP::get(*TheContext, APFloat(0.0));
    ElseV = get_default_constant(get_cpoint_type_from_llvm(phiType));
    }
  }

  /*if (ThenV->getType() != Type::getDoubleTy(*TheContext)){
    ThenV = ConstantFP::get(*TheContext, APFloat(0.0));
  }*/
  /*if (ThenV->getType() != phiType){
    convert_to_type(get_cpoint_type_from_llvm(ThenV->getType()), phiType, ThenV);
  }*/
   /*if (ElseV->getType() != phiType){
    convert_to_type(get_cpoint_type_from_llvm(ElseV->getType()), phiType, ElseV);
  }*/

  PN->addIncoming(ThenV, ThenBB);
  PN->addIncoming(ElseV, ElseBB);
  return PN;
}

Value* ReturnAST::codegen(){
  Value* value_returned = (returned_expr) ? returned_expr->codegen() : nullptr;
  //return Builder->CreateRet(value_returned);
  Builder->CreateRet(value_returned);
  return value_returned;
}

Value* GotoExprAST::codegen(){
  Function *TheFunction = Builder->GetInsertBlock()->getParent();
  BasicBlock* bb = get_basic_block(TheFunction, label_name);
  if (bb == nullptr){
    Log::Warning(this->loc) << "Basic block couldn't be found in goto" << "\n";
    return nullptr;
  }
  Builder->CreateBr(bb);
  return Constant::getNullValue(Type::getDoubleTy(*TheContext));
}

Value* LabelExprAST::codegen(){
  Function *TheFunction = Builder->GetInsertBlock()->getParent();
  BasicBlock* labelBB = BasicBlock::Create(*TheContext, label_name, TheFunction);
  Builder->CreateBr(labelBB);
  Builder->SetInsertPoint(labelBB);
  return Constant::getNullValue(Type::getDoubleTy(*TheContext));
}

Value* RedeclarationExprAST::codegen(){
  // TODO move this from a AST node to an operator 
  Log::Info() << "REDECLARATION CODEGEN" << "\n";
  Log::Info() << "VariableName " << VariableName << "\n";
  //bool is_object = false;
  bool is_array = false;
  if (index != nullptr){
    is_array = true;
  }
  std::string ArrayName = "";
  //int pos_array = -1;
  bool is_global = false;
  if (GlobalVariables[VariableName] != nullptr){
    is_global = true;
  }
  Log::Info() << "is_global : " << is_global << "\n";
  Log::Info() << "VariableName : " << VariableName << "\n";
  Function *TheFunction = Builder->GetInsertBlock()->getParent();
  //Log::Info() << "TEST\n";
  if (Val == nullptr){
    return LogErrorV(this->loc, "Val is Nullptr\n");
  }
  Value* ValDeclared = Val->codegen();
  Cpoint_Type type = Cpoint_Type(0);
  if (is_global && GlobalVariables[VariableName] != nullptr){
    type = GlobalVariables[VariableName]->type;
  } else if (NamedValues[VariableName] != nullptr){
    type = NamedValues[VariableName]->type;
  } else {
    std::string nearest_variable;
    int lowest_distance = 20;
    int temp_distance = 30;
    for ( const auto &p : NamedValues ){
      //Log::Info() << "temp variable : " << p.first << "\n";
      if (p.second != nullptr){
      if ((temp_distance = stringDistance(VariableName, p.first)) < lowest_distance){
        nearest_variable = p.first;
        lowest_distance = temp_distance;
      }
      }
      //Log::Info() << "string distance : " << temp_distance << "\n";
    }
    return LogErrorV(this->loc, "couldn't find variable \"%s\", maybe you meant \"%s\"", VariableName.c_str(), nearest_variable.c_str());
  }
  if (!type.is_template_type && type.type != 0 && member == ""){ 
  if (ValDeclared->getType() != get_type_llvm(type)){
    convert_to_type(get_cpoint_type_from_llvm(ValDeclared->getType()), get_type_llvm(type), ValDeclared);
  }
  }
  bool is_struct = false;
  bool is_union = false;
  //bool is_class = false;
  if (member != ""){
  Log::Info() << "objectName : " << VariableName << "\n";
  if (NamedValues[VariableName] != nullptr){
    if (UnionDeclarations[NamedValues[VariableName]->type.union_name] != nullptr){
      Log::Info() << "IS_UNION" << "\n";
      is_union = true;
    } else if (StructDeclarations[NamedValues[VariableName]->type.struct_name] != nullptr){
      Log::Info() << "IS_STRUCT" << "\n";
      is_struct = true;
    } else {
        return LogErrorV(this->loc, "The variable %s in redeclaration which is used with a member %s is neither a struct or an union", VariableName.c_str(), member.c_str());
    }
  }
  }
  if (is_union){
    Cpoint_Type cpoint_type =  is_global ? GlobalVariables[VariableName]->type : NamedValues[VariableName]->type;
    AllocaInst *Alloca = CreateEntryBlockAlloca(TheFunction, VariableName, cpoint_type);
    auto members = UnionDeclarations[NamedValues[VariableName]->type.union_name]->members;
    int pos_union = -1;
    Log::Info() << "members.size() : " << members.size() << "\n";
    for (int i = 0; i < members.size(); i++){
      if (members.at(i).first == member){
        pos_union = i;
        break;
      }
    }
    if (ValDeclared->getType() != get_type_llvm(members.at(pos_union).second)){
        convert_to_type(get_cpoint_type_from_llvm(ValDeclared->getType()), get_type_llvm(members.at(pos_union).second), ValDeclared);
    }
    Builder->CreateStore(ValDeclared, Alloca);
    NamedValues[VariableName] = std::make_unique<NamedValue>(Alloca, cpoint_type);
  } else if (is_struct){
    Log::Info() << "StructName : " << VariableName << "\n";
    auto members = StructDeclarations[NamedValues[VariableName]->type.struct_name]->members;
    int pos_struct = -1;
    Log::Info() << "members.size() : " << members.size() << "\n";
    for (int i = 0; i < members.size(); i++){
      if (members.at(i).first == member){
        pos_struct = i;
        break;
      }
    }
    Log::Info() << "Pos for GEP struct member redeclaration : " << pos_struct << "\n";
    auto zero = llvm::ConstantInt::get(*TheContext, llvm::APInt(64, 0, true));
    auto index = llvm::ConstantInt::get(*TheContext, llvm::APInt(32, pos_struct, true));
    auto structPtr = NamedValues[VariableName]->alloca_inst;
    Cpoint_Type cpoint_type = NamedValues[VariableName]->type;
    if (cpoint_type.is_ptr){
      cpoint_type.is_ptr = false;
    }
    auto ptr = Builder->CreateGEP(get_type_llvm(cpoint_type), structPtr, {zero, index}, "get_struct");
    if (ValDeclared->getType() != get_type_llvm(members.at(pos_struct).second)){
        convert_to_type(get_cpoint_type_from_llvm(ValDeclared->getType()), get_type_llvm(members.at(pos_struct).second), ValDeclared);
    }
    Builder->CreateStore(ValDeclared, ptr);
    NamedValues[VariableName] = std::make_unique<NamedValue>(structPtr, cpoint_type);
  } else if (is_array) {
    Log::Info() << "array redeclaration" << "\n";
    Cpoint_Type cpoint_type = NamedValues[VariableName]->type;
    Cpoint_Type member_type = cpoint_type;
    Log::Info() << "member type : " << member_type << "\n";
    if (member_type.is_ptr && !member_type.is_array){
        Log::Info() << "is member" << "\n";
        member_type.is_ptr = false;
        member_type.nb_ptr = 0;
        member_type.nb_element = 0;
    } else {
        member_type.is_array = false;
        member_type.nb_element = 0;
    }
    if (ValDeclared->getType()->isArrayTy()){
      AllocaInst *Alloca = CreateEntryBlockAlloca(TheFunction, VariableName, cpoint_type);
      Builder->CreateStore(ValDeclared, Alloca);
      NamedValues[VariableName] = std::make_unique<NamedValue>(Alloca, cpoint_type);
      return Constant::getNullValue(Type::getDoubleTy(*TheContext));
    }
    //Log::Info() << "Pos for GEP : " << pos_array << "\n";
    Log::Info() << "ArrayName : " << VariableName << "\n";
    auto zero = llvm::ConstantInt::get(*TheContext, llvm::APInt(64, 0, true));
    //auto one = llvm::ConstantInt::get(*TheContext, llvm::APInt(32, 1, true));
    //auto index = llvm::ConstantInt::get(*TheContext, llvm::APInt(32, pos_array, true)); 
    auto indexVal = index->codegen();
    if (indexVal->getType() != get_type_llvm(int_type)){
        convert_to_type(get_cpoint_type_from_llvm(indexVal->getType()), get_type_llvm(int_type), indexVal) ;
    }
    //indexVal = Builder->CreateFPToUI(indexVal, Type::getInt32Ty(*TheContext), "cast_gep_index");
    if (!index){
      return LogErrorV(this->loc, "couldn't find index for array %s", VariableName.c_str());
    }
    auto arrayPtr = NamedValues[VariableName]->alloca_inst;
    Log::Info() << "Number of member in array : " << cpoint_type.nb_element << "\n";
    std::vector<Value*> indexes = { zero, indexVal};
    if (cpoint_type.is_ptr && !cpoint_type.is_array){
        Log::Info() << "array for array member is ptr" << "\n";
        cpoint_type.is_ptr = false;
        indexes = {indexVal};
    }
    Type* llvm_type = get_type_llvm(cpoint_type);
    Log::Info() << "Get LLVM TYPE" << "\n";
    auto ptr = Builder->CreateGEP(llvm_type, arrayPtr, indexes, "get_array");
    Log::Info() << "Create GEP" << "\n";
    if (ValDeclared->getType() != get_type_llvm(member_type)){
      convert_to_type(get_cpoint_type_from_llvm(ValDeclared->getType()), get_type_llvm(member_type), ValDeclared);
    }
    Builder->CreateStore(ValDeclared, ptr);
    NamedValues[VariableName] = std::make_unique<NamedValue>(arrayPtr, cpoint_type);
  } else {
  Cpoint_Type cpoint_type =  is_global ? GlobalVariables[VariableName]->type : NamedValues[VariableName]->type;
  if (is_global){
    Builder->CreateStore(ValDeclared, GlobalVariables[VariableName]->globalVar);
  } else {
  AllocaInst *Alloca = CreateEntryBlockAlloca(TheFunction, VariableName, cpoint_type);
  if (ValDeclared->getType() == get_type_llvm(Cpoint_Type(void_type))){
    return LogErrorV(this->loc, "Assigning to a variable a void value");
  }
  Builder->CreateStore(ValDeclared, Alloca);
  NamedValues[VariableName] = std::make_unique<NamedValue>(Alloca, cpoint_type);
  }
  }
  return Constant::getNullValue(Type::getDoubleTy(*TheContext));
}

Value* BreakExprAST::codegen(){
  Builder->CreateBr(blocksForBreak.top());
  Builder->SetInsertPoint(blocksForBreak.top());
  return Constant::getNullValue(Type::getDoubleTy(*TheContext));
}

Value* LoopExprAST::codegen(){
  Function *TheFunction = Builder->GetInsertBlock()->getParent();
  if (is_infinite_loop || Array == nullptr){
    BasicBlock* loopBB = BasicBlock::Create(*TheContext, "loop_infinite", TheFunction);
    Builder->CreateBr(loopBB);
    Builder->SetInsertPoint(loopBB);
    BasicBlock* afterBB = BasicBlock::Create(*TheContext, "after_loop_infinite", TheFunction);
    blocksForBreak.push(afterBB);
    for (int i = 0; i < Body.size(); i++){
      if (!Body.at(i)->codegen())
        return nullptr;
    }
    blocksForBreak.pop();
    Builder->CreateBr(loopBB);

    return Constant::getNullValue(Type::getDoubleTy(*TheContext));
  } else {
    auto double_cpoint_type = Cpoint_Type(double_type, false);
    AllocaInst *PosArrayAlloca = CreateEntryBlockAlloca(TheFunction, "pos_loop_in", double_cpoint_type);
    BasicBlock* CmpLoop = BasicBlock::Create(*TheContext, "cmp_loop_in", TheFunction);
    BasicBlock* InLoop = BasicBlock::Create(*TheContext, "body_loop_in", TheFunction);
    BasicBlock* AfterLoop = BasicBlock::Create(*TheContext, "after_loop_in", TheFunction);;
    if (NamedValues[VarName] != nullptr){
      return LogErrorV(this->loc, "variable for loop already exists in the context");
    }
    Value *StartVal = ConstantFP::get(*TheContext, APFloat(0.0));
    Builder->CreateStore(StartVal, PosArrayAlloca);
 
    //if (auto ArrayVarPtr = dynamic_cast<VariableExprAST*>(Array.get())){
    auto ArrayVarPtr = static_cast<VariableExprAST*>(Array.get());
    std::unique_ptr<VariableExprAST> ArrayVar;
    Array.release();
    ArrayVar.reset(ArrayVarPtr);
    //Value* ArrayPtr = ArrayVar->codegen();
    Cpoint_Type cpoint_type = NamedValues[ArrayVar->getName()]->type;
    //} else {
      //return LogErrorV(this->loc, "Expected a Variable Expression in loop in");
    //}
    Cpoint_Type tempValueArrayType = Cpoint_Type(cpoint_type);
    tempValueArrayType.is_array = false;
    tempValueArrayType.nb_element = 0;
    AllocaInst* TempValueArray = CreateEntryBlockAlloca(TheFunction, VarName, tempValueArrayType);
    NamedValues[VarName] = std::make_unique<NamedValue>(TempValueArray, tempValueArrayType, nullptr, ""); // change values NULL and "" to have structs and classes supported
    Value* SizeArrayVal = ConstantFP::get(*TheContext, APFloat((double)cpoint_type.nb_element));
    Builder->CreateBr(CmpLoop);
    Builder->SetInsertPoint(CmpLoop);
    Value* PosVal = Builder->CreateLoad(PosArrayAlloca->getAllocatedType(), PosArrayAlloca, "load_pos_loop_in");
    Value* isLoopFinishedVal = Builder->CreateFCmpOLT(PosVal, SizeArrayVal,"cmptmp_loop_in");  // if is less than
    Value* isLoopFinishedValFP = Builder->CreateUIToFP(isLoopFinishedVal, Type::getDoubleTy(*TheContext), "booltmp_loop_in");
    Value* isLoopFinishedBoolVal = Builder->CreateFCmpONE(isLoopFinishedValFP, ConstantFP::get(*TheContext, APFloat(0.0)), "loopcond_loop_in");
    Builder->CreateCondBr(isLoopFinishedBoolVal, InLoop, AfterLoop);
    
    Builder->SetInsertPoint(InLoop);
    // initalize here the temp variable to have the value of the array
    Value* PosValInner = Builder->CreateLoad(PosArrayAlloca->getAllocatedType(), PosArrayAlloca, "load_pos_loop_in");
    auto zero = llvm::ConstantInt::get(*TheContext, llvm::APInt(64, 0, true));
    Value* index = Builder->CreateFPToUI(PosValInner, Type::getInt32Ty(*TheContext), "cast_gep_index");
    Value* ptr = Builder->CreateGEP(get_type_llvm(cpoint_type), /*TempValueArray*/ NamedValues[ArrayVarPtr->getName()]->alloca_inst, { zero, index}, "gep_loop_in");
    Value* value = Builder->CreateLoad(get_type_llvm(tempValueArrayType), ptr, VarName);
    Builder->CreateStore(value /*ptr*/, TempValueArray);
    blocksForBreak.push(AfterLoop);
    for (int i = 0; i < Body.size(); i++){
    if (!Body.at(i)->codegen())
      return nullptr;
    }
    blocksForBreak.pop();
    Value* StepVal = ConstantFP::get(*TheContext, APFloat(1.0));
    Value *CurrentPos = Builder->CreateLoad(PosArrayAlloca->getAllocatedType(), PosArrayAlloca, "current_pos_loop_in");
    Value* NextPos = Builder->CreateFAdd(CurrentPos, StepVal, "nextpos_loop_in");
    Builder->CreateStore(NextPos, PosArrayAlloca);
    Builder->CreateBr(CmpLoop);

    Builder->SetInsertPoint(AfterLoop);

    return Constant::getNullValue(Type::getDoubleTy(*TheContext));
  }
}

Value* WhileExprAST::codegen(){
  Function *TheFunction = Builder->GetInsertBlock()->getParent();
  BasicBlock* whileBB = BasicBlock::Create(*TheContext, "while", TheFunction);
  Builder->CreateBr(whileBB);
  Builder->SetInsertPoint(whileBB);
  BasicBlock *LoopBB = BasicBlock::Create(*TheContext, "loop_while", TheFunction);
  Value *CondV = Cond->codegen();
  if (!CondV)
    return nullptr;
  CondV = Builder->CreateFCmpONE(
    CondV, ConstantFP::get(*TheContext, APFloat(0.0)), "loopcond_while");
  BasicBlock *AfterBB =
      BasicBlock::Create(*TheContext, "afterloop_while", TheFunction);

  blocksForBreak.push(AfterBB);
  Builder->CreateCondBr(CondV, LoopBB, AfterBB);
  Builder->SetInsertPoint(LoopBB);
  for (int i = 0; i < Body.size(); i++){
    if (!Body.at(i)->codegen())
      return nullptr;
  }
  blocksForBreak.pop();
  Builder->CreateBr(whileBB);
  Builder->SetInsertPoint(AfterBB);
  return Constant::getNullValue(Type::getDoubleTy(*TheContext));
}

Value *ForExprAST::codegen(){
  Function *TheFunction = Builder->GetInsertBlock()->getParent();
  AllocaInst *Alloca = CreateEntryBlockAlloca(TheFunction, VarName, Cpoint_Type(double_type, false));
  CpointDebugInfo.emitLocation(this);
  Value *StartVal = Start->codegen();
  if (!StartVal)
    return nullptr;
  if (StartVal->getType() != Alloca->getType()){
      convert_to_type(get_cpoint_type_from_llvm(StartVal->getType()), Alloca->getAllocatedType(), StartVal);
  }
  Builder->CreateStore(StartVal, Alloca);

  BasicBlock *LoopBB = BasicBlock::Create(*TheContext, "loop_for", TheFunction);
  Builder->CreateBr(LoopBB);
  Builder->SetInsertPoint(LoopBB);
  AllocaInst *OldVal;
  if (NamedValues[VarName] == nullptr){
    OldVal = nullptr;
  } else {
    OldVal = NamedValues[VarName]->alloca_inst;
  }
  NamedValues[VarName] = std::make_unique<NamedValue>(Alloca, Cpoint_Type(double_type));
  BasicBlock *AfterBB =
      BasicBlock::Create(*TheContext, "afterloop", TheFunction);
  blocksForBreak.push(AfterBB);
  /*PHINode *Variable = Builder->CreatePHI(Type::getDoubleTy(*TheContext), 2, VarName);
  Variable->addIncoming(StartVal, PreheaderBB);
  Value *OldVal = NamedValues[VarName];
  NamedValues[VarName] = Variable;*/
  for (int i = 0; i < Body.size(); i++){
    if (!Body.at(i)->codegen())
      return nullptr;
  }
  blocksForBreak.pop();
  Value *StepVal = nullptr;
  if (Step) {
    StepVal = Step->codegen();
    if (!StepVal)
      return nullptr;
  } else {
    // If not specified, use 1.0.
    StepVal = ConstantFP::get(*TheContext, APFloat(1.0));
  }

  // Compute the end condition.
  Value *EndCond = End->codegen();
  if (!EndCond)
    return nullptr;

  Value *CurVar =
      Builder->CreateLoad(Alloca->getAllocatedType(), Alloca, VarName.c_str());
  Value *NextVar = Builder->CreateFAdd(CurVar, StepVal, "nextvar_for");
  Builder->CreateStore(NextVar, Alloca);

  // Convert condition to a bool by comparing non-equal to 0.0.
  EndCond = Builder->CreateFCmpONE(
      EndCond, ConstantFP::get(*TheContext, APFloat(0.0)), "loopcond_for");

  // Create the "after loop" block and insert it.
  //BasicBlock *LoopEndBB = Builder->GetInsertBlock();

  // Insert the conditional branch into the end of LoopEndBB.
  Builder->CreateCondBr(EndCond, LoopBB, AfterBB);

  // Any new code will be inserted in AfterBB.
  Builder->SetInsertPoint(AfterBB);

  // Add a new entry to the PHI node for the backedge.
  //Variable->addIncoming(NextVar, LoopEndBB);

  // Restore the unshadowed variable.
  if (OldVal)
    NamedValues[VarName] = std::make_unique<NamedValue>(OldVal, Cpoint_Type(double_type));
  else
    NamedValues.erase(VarName);

  // for expr always returns 0.0.
  return Constant::getNullValue(Type::getDoubleTy(*TheContext));
}

Value *UnaryExprAST::codegen() {
  Value *OperandV = Operand->codegen();
  if (!OperandV)
    return nullptr;
  if (Opcode == '-'){
    return operators::LLVMCreateSub(ConstantFP::get(*TheContext, APFloat(0.0)), OperandV);
  }
  Function *F = getFunction(std::string("unary") + Opcode);
  if (!F){
    Log::Info() << "UnaryExprAST : " << Opcode << "\n";
    return LogErrorV(this->loc, "Unknown unary operator %c", Opcode);
  }

  return Builder->CreateCall(F, OperandV, "unop");
}

Value *VarExprAST::codegen() {
  Log::Info() << "VAR CODEGEN " << VarNames.at(0).first << "\n";
  std::vector<AllocaInst *> OldBindings;
  Function *TheFunction = Builder->GetInsertBlock()->getParent();

  // Register all variables and emit their initializer.
  for (unsigned i = 0, e = VarNames.size(); i != e; ++i) {
    const std::string &VarName = VarNames[i].first;
    
    ExprAST *Init = VarNames[i].second.get();

    // Emit the initializer before adding the variable to scope, this prevents
    // the initializer from referencing the variable itself, and permits stuff
    // like this:
    //  var a = 1 in
    //    var a = a in ...   # refers to outer 'a'.
    Value *InitVal;
    bool no_explicit_init_val = false;
    if (Init) {
      if (dynamic_cast<EnumCreation*>(Init)){
        Log::Info() << "init val EnumCreation" << "\n";
        AllocaInst* Alloca = CreateEntryBlockAlloca(TheFunction, VarName, cpoint_type);
        NamedValues[VarName] = std::make_unique<NamedValue>(Alloca, cpoint_type);
        Cpoint_Type cpoint_type = NamedValues[VarName]->type;
        Cpoint_Type tag_type = Cpoint_Type(int_type);
        auto* enumCreation = dynamic_cast<EnumCreation*>(Init);
        auto zero = llvm::ConstantInt::get(*TheContext, llvm::APInt(64, 0, true));
        auto index_tag = llvm::ConstantInt::get(*TheContext, llvm::APInt(32, 0, true));
        auto index_val = llvm::ConstantInt::get(*TheContext, llvm::APInt(32, 1, true));
        auto ptr_tag = Builder->CreateGEP(get_type_llvm(cpoint_type), Alloca, {zero, index_tag}, "get_struct");
        auto value_tag = Builder->CreateGEP(get_type_llvm(cpoint_type), Alloca, {zero, index_val}, "get_struct");
        auto EnumMembers = std::move(EnumDeclarations[enumCreation->EnumVarName]->EnumDeclar->clone()->EnumMembers);
        std::unique_ptr<EnumMember> enumMember = nullptr;
        int pos_member = -1;
        
        for (int i = 0; i < EnumMembers.size(); i++){
            if (EnumMembers.at(i)->Name == enumCreation->EnumMemberName){
                pos_member = i;
                enumMember = EnumMembers.at(i)->clone();
            }
        }
        if (!enumMember){
            return LogErrorV(this->loc, "Couldn't find enum member %s", enumCreation->EnumMemberName.c_str());
        }
        Builder->CreateStore(llvm::ConstantInt::get(*TheContext, llvm::APInt(32, pos_member, true)), ptr_tag);
        NamedValues[VarName] = std::make_unique<NamedValue>(Alloca, cpoint_type);
        if (enumCreation->value){
            Value* val = enumCreation->value->clone()->codegen();
            if (val->getType() != get_type_llvm(*enumMember->Type)){
                convert_to_type(get_cpoint_type_from_llvm(val->getType()), get_type_llvm(*enumMember->Type), val);
            }
            Builder->CreateStore(val, value_tag);
            NamedValues[VarName] = std::make_unique<NamedValue>(Alloca, cpoint_type);
        }
        goto after_storing; 
        // TODO : write code here to initalize the enum without codegen
      }
      InitVal = Init->codegen();
      if (!InitVal)
        return nullptr;
      if (infer_type){
        cpoint_type = Cpoint_Type(get_cpoint_type_from_llvm(InitVal->getType()));
      }
    } else { // If not specified, use 0.0.
      no_explicit_init_val = true;
      InitVal = get_default_value(cpoint_type);
      //InitVal = get_default_constant(*cpoint_type);
    }
    llvm::Value* indexVal = nullptr;
    //double indexD = -1;
    if (index != nullptr){
    indexVal = index->codegen();
    auto constFP = dyn_cast<ConstantFP>(indexVal);
    double indexD = constFP->getValueAPF().convertToDouble();
    Log::Info() << "index for varexpr array : " << indexD << "\n";
    cpoint_type.nb_element = indexD;
    }
    AllocaInst *Alloca = CreateEntryBlockAlloca(TheFunction, VarName, cpoint_type);
    /*if (!get_type_llvm(cpoint_type)->isPointerTy()){
    Log::Warning() << "cpoint_type in var " << VarNames[i].first << " is not ptr" << "\n";
    }*/
    if (index == nullptr){
    if (InitVal->getType() != get_type_llvm(cpoint_type)){
      convert_to_type(get_cpoint_type_from_llvm(InitVal->getType()), get_type_llvm(cpoint_type), InitVal);
    }
    }
    if (InitVal->getType() == get_type_llvm(Cpoint_Type(void_type))){
       return LogErrorV(this->loc, "Assigning to a variable as default value a void value");
    }
    if (!cpoint_type.is_struct || !no_explicit_init_val){
        Builder->CreateStore(InitVal, Alloca);
    }

    // Remember the old variable binding so that we can restore the binding when
    // we unrecurse.
    if (NamedValues[VarName] == nullptr){

      OldBindings.push_back(nullptr);
    } else {
    OldBindings.push_back(NamedValues[VarName]->alloca_inst);
    }

    // Remember this binding.
    Type* struct_type_temp = get_type_llvm(Cpoint_Type(double_type));
    std::string struct_declaration_name_temp = "";
    if (cpoint_type.is_struct){
      std::string struct_name = cpoint_type.struct_name;
      if (cpoint_type.is_struct_template){
        struct_name = get_struct_template_name(cpoint_type.struct_name, *cpoint_type.struct_template_type_passed);
      }
      if (StructDeclarations[struct_name] == nullptr){
        return LogErrorV(this->loc, "Couldn't find struct declaration %s for created variable", cpoint_type.struct_name.c_str());
      }
      struct_type_temp = StructDeclarations[struct_name]->struct_type;
      struct_declaration_name_temp = struct_name;
    }
    Log::Info() << "VarName " << VarName << "\n";
    Log::Info() << "struct_declaration_name_temp " << struct_declaration_name_temp << "\n";
    NamedValues[VarName] = std::make_unique<NamedValue>(Alloca, cpoint_type, struct_type_temp, struct_declaration_name_temp);
    Log::Info() << "NamedValues[VarName]->struct_declaration_name : " <<  NamedValues[VarName]->struct_declaration_name << "\n";
    if (cpoint_type.is_struct && !cpoint_type.is_ptr){
      if (Function* constructorF = getFunction(cpoint_type.struct_name + "__Constructor__Default")){
      std::vector<Value *> ArgsV;
      
      auto A = NamedValues[VarName]->alloca_inst;
      Value* thisClass = Builder->CreateLoad(PointerType::get(A->getAllocatedType(), A->getAddressSpace()), A, VarName.c_str());
      ArgsV.push_back(thisClass);
      Builder->CreateCall(constructorF, ArgsV, "calltmp");
      }
    }
  }
after_storing:
  CpointDebugInfo.emitLocation(this);
  // Pop all our variables from scope.
  /*for (unsigned i = 0, e = VarNames.size(); i != e; ++i)
    NamedValues[VarNames[i].first] = OldBindings[i];
  */
  // Return the body computation.
  //return BodyVal;
  // for expr always returns 0.0.
  return Constant::getNullValue(Type::getDoubleTy(*TheContext));
}

void InitializeModule(std::string filename) {
  // Open a new context and module.
  TheContext = std::make_unique<LLVMContext>();
  TheModule = std::make_unique<Module>(filename, *TheContext);

  // Create a new builder for the module.
  Builder = std::make_unique<IRBuilder<>>(*TheContext);
}
