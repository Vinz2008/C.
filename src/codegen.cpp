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

using namespace llvm;

std::unique_ptr<LLVMContext> TheContext;
std::unique_ptr<Module> TheModule;
std::unique_ptr<IRBuilder<>> Builder;
std::map<std::string, std::unique_ptr<NamedValue>> NamedValues;
std::map<std::string, std::unique_ptr<GlobalVariableValue>> GlobalVariables;
std::map<std::string, std::unique_ptr<PrototypeAST>> FunctionProtos;
std::map<std::string, std::unique_ptr<StructDeclaration>> StructDeclarations;

std::map<std::string, std::unique_ptr<TemplateProto>> TemplateProtos;
std::vector<std::string> modulesNamesContext;

std::map<std::string, Function*> GeneratedFunctions;

std::stack<BasicBlock*> blocksForBreak;

std::vector<std::unique_ptr<TemplateCall>> TemplatesToGenerate;

std::pair<std::string, std::string> TypeTemplateCallCodegen;



extern std::map<std::string, int> BinopPrecedence;
extern bool gc_mode;
extern std::unique_ptr<DIBuilder> DBuilder;
extern struct DebugInfo CpointDebugInfo;

extern Source_location emptyLoc;

extern bool debug_info_mode;

Value *LogErrorV(const char *Str, ...);

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
            return LogErrorV("wrong number of arguments for reflection function");
        }
        return getTypeId(Args.at(0)->codegen());
    } else if (instruction == "getstructname"){
        if (Args.size() > 1){
            return LogErrorV("wrong number of arguments for reflection function");
        }
        Value* val = Args.at(0)->codegen();
        if (!val->getType()->isStructTy()){
            return LogErrorV("argument is not a struct in getmembername");
        }
        auto structType = static_cast<StructType*>(val->getType());
        std::string structName = (std::string)structType->getStructName();
        Log::Info() << "get struct name : " << structName << "\n";
        return Builder->CreateGlobalStringPtr(StringRef(structName.c_str()));
    } else if (instruction == "getmembernb"){
        if (Args.size() > 1){
            return LogErrorV("wrong number of arguments for reflection function");
        }
        Value* val = Args.at(0)->codegen();
        if (!val->getType()->isStructTy()){
            return LogErrorV("argument is not a struct in getmembername");
        }
        auto structType = static_cast<StructType*>(val->getType());
        int nb_member = -1;
        nb_member = structType->getNumElements();
        Log::Info() << "getmembernbname : " << nb_member << "\n";
        return ConstantFP::get(*TheContext, APFloat((double)nb_member));
    }
    return LogErrorV("Unknown Reflection Instruction");
}

Function *getFunction(std::string Name) {
  // First, see if the function has already been added to the current module.
  if (auto *F = TheModule->getFunction(Name))
    return F;

  // If not, check whether we can codegen the declaration from some existing
  // prototype.
  auto FI = FunctionProtos.find(Name);
  if (FI != FunctionProtos.end())
    return FI->second->codegen();

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

void codegenTemplates(){
  for (int i = 0; i < TemplatesToGenerate.size(); i++){
    TypeTemplateCallCodegen = std::make_pair(TemplatesToGenerate.at(i)->functionAST->Proto->template_name, TemplatesToGenerate.at(i)->typeName);
    TemplatesToGenerate.at(i)->functionAST->codegen();
  }
}

void callTemplate(std::string& Callee, std::string template_passed_type){
  Log::Info() << "Callee : " << Callee << "\n";
  auto templateProto = std::make_unique<TemplateProto>(TemplateProtos[Callee]->functionAST->clone(), TemplateProtos[Callee]->template_type_name); 
  std::string typeName = "____type";
  typeName = "____" + template_passed_type;
  std::string function_temp_name = templateProto->functionAST->Proto->Name + typeName; // add something to specify type
  templateProto->functionAST->Proto->Name = function_temp_name;
  //templateProto->functionAST->codegen();
  add_manually_extern(templateProto->functionAST->Proto->Name, templateProto->functionAST->Proto->cpoint_type, templateProto->functionAST->Proto->Args, (templateProto->functionAST->Proto->IsOperator) ? 1 : 0, templateProto->functionAST->Proto->getBinaryPrecedence(), templateProto->functionAST->Proto->is_variable_number_args, templateProto->functionAST->Proto->has_template, templateProto->functionAST->Proto->template_name);
  Callee = function_temp_name;
  TemplatesToGenerate.push_back(std::make_unique<TemplateCall>(template_passed_type, std::move(templateProto->functionAST)));
}

Value* callLLVMIntrisic(std::string Callee, std::vector<std::unique_ptr<ExprAST>>& Args){
  Callee = Callee.substr(5, Callee.size());
  Log::Info() << "llvm intrisic called " << Callee << "\n";
  llvm::Intrinsic::IndependentIntrinsics intrisicId = llvm::Intrinsic::not_intrinsic;
  if (Callee == "va_start"){
    intrisicId = Intrinsic::vastart;
  } else if (Callee == "va_end"){
    intrisicId = Intrinsic::vaend;
  }
  Function *CalleeF = Intrinsic::getDeclaration(TheModule.get(), intrisicId);
  std::vector<Value *> ArgsV;
  for (unsigned i = 0, e = Args.size(); i != e; ++i) {
    Value* temp_val = Args[i]->codegen();
    ArgsV.push_back(temp_val);
    if (!ArgsV.back())
      return nullptr;
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

Value *LogErrorV(const char *Str, ...) {
  va_list args;
  va_start(args, Str);
  vLogError(Str, args);
  va_end(args);
  return nullptr;
}

Value *NumberExprAST::codegen() {
  return ConstantFP::get(*TheContext, APFloat(Val));
}

Value* StringExprAST::codegen() {
  Log::Info() << "Before Codegen " << (std::string)StringRef(str.c_str()) << '\n';
  Value* string = Builder->CreateGlobalStringPtr(StringRef(str.c_str()));
  Log::Info() << "Codegen String" << "\n";
  //string->print(outs());
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
  return LogErrorV("Unknown variable name %s", Name.c_str());
  }
  AllocaInst *A = NamedValues[Name]->alloca_inst;
  if (!A)
    return LogErrorV("Unknown variable name %s", Name.c_str());
  return Builder->CreateLoad(A->getAllocatedType(), A, Name.c_str());
}

Value* StructMemberExprAST::codegen() {
    Log::Info() << "STRUCT MEMBER CODEGEN" << "\n";
    AllocaInst* Alloca = nullptr;
    if (StructName == "reflection"){
        goto if_reflection;
    }
    if (NamedValues[StructName] == nullptr){
        return LogErrorV("Can't find struct that is used for a member");
    }
    Alloca = NamedValues[StructName]->alloca_inst;
    Log::Info() << "struct type : " <<  NamedValues[StructName]->type << "\n";
    if (!NamedValues[StructName]->type.is_struct ){ // TODO : verify if is is really  struct (it didn't work for example with the self of structs function members)
      return LogErrorV("Using a member of variable even though it is not a struct");
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
        return LogErrorV("Unknown struct function member called : %s\n", MemberName.c_str());
      }
      std::vector<llvm::Value*> CallArgs;
      CallArgs.push_back(NamedValues[StructName]->alloca_inst);
      for (int i = 0; i < Args.size(); i++){
        CallArgs.push_back(Args.at(i)->codegen());
      }
      Function *F = getFunction(struct_function_mangling(NamedValues[StructName]->type.struct_name, MemberName));
      if (!F){
        Log::Info() << "struct_function_mangling(StructName, MemberName) : " << struct_function_mangling(NamedValues[StructName]->type.struct_name, MemberName) << "\n";
        return LogErrorV("The function member %s called doesn't exist mangled in the scope", MemberName.c_str());
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

Value* ArrayMemberExprAST::codegen() {
  // TODO : fix type for array member after getelementptr
  Log::Info() << "ARRAY MEMBER CODEGEN" << "\n";
  auto index = posAST->codegen();
  if (!index){
    return LogErrorV("error in array index");
  }
  index = Builder->CreateFPToUI(index, Type::getInt32Ty(*TheContext), "cast_gep_index");
  if (!is_llvm_type_number(index->getType())){
    return LogErrorV("index for array is not a number\n");
  }
  Cpoint_Type cpoint_type = NamedValues[ArrayName]->type;
  AllocaInst* Alloca = NamedValues[ArrayName]->alloca_inst;
  auto zero = llvm::ConstantInt::get(*TheContext, llvm::APInt(64, 0, true));
  std::vector<Value*> indexes = { zero, index};
  if (cpoint_type.is_ptr && !cpoint_type.is_array){
    Log::Info() << "array for array member is ptr" << "\n";
    cpoint_type.is_ptr = false;
    indexes = {index};
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
    self_pointer_type = Cpoint_Type(double_type, true, false, 0, true, Name);
    FunctionExpr->Proto->Args.insert(FunctionExpr->Proto->Args.begin(), std::make_pair("self", self_pointer_type)); // TODO fix this by passing struct type pointer as first arg
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
    Cpoint_Type self_pointer_type = Cpoint_Type(double_type, true, false, 0, true, Name);
    ProtoExpr->Args.insert(ProtoExpr->Args.begin(), std::make_pair("self", self_pointer_type));
    ProtoExpr->Name = mangled_name_function;
    ProtoExpr->codegen();
  }

  StructDeclarations[Name] = std::make_unique<StructDeclaration>(structType, std::move(members), std::move(functions));
  return structType;
}

Value *BinaryExprAST::codegen() {
  Value *L = LHS->codegen();
  Value *R = RHS->codegen();
  if (!L || !R)
    return nullptr;
  if (L->getType() != R->getType()){
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
    L = Builder->CreateFCmpULE(L, R, "cmptmp");
    return Builder->CreateUIToFP(L, Type::getDoubleTy(*TheContext), "booltmp");
  }
  if (Op == ">="){
    L = Builder->CreateFCmpUGE(L, R, "cmptmp");
    return Builder->CreateUIToFP(L, Type::getDoubleTy(*TheContext), "booltmp");
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
    if (get_cpoint_type_from_llvm(R->getType()).type == int_type){
      L = Builder->CreateICmpSLT(L, R, "cmptmp");
    } else {
      L = Builder->CreateFCmpOLT(L, R, "cmptmp");
    }
    // Convert bool 0/1 to double 0.0 or 1.0
    return Builder->CreateUIToFP(L, Type::getDoubleTy(*TheContext), "booltmp");
  case '/':
    return operators::LLVMCreateDiv(L, R);
  case '>':
    if (get_cpoint_type_from_llvm(R->getType()).type == int_type){
      L = Builder->CreateICmpSGT(R, L, "cmptmp");
    } else {
      L = Builder->CreateFCmpOGT(R, L, "cmptmp");
    }
    return Builder->CreateUIToFP(L, Type::getDoubleTy(*TheContext), "booltmp");
  case '^':
    L = Builder->CreateXor(L, R, "xortmp");
    return Builder->CreateUIToFP(L, Type::getDoubleTy(*TheContext), "booltmp");
  case '|':
    L = Builder->CreateOr(L, R, "ortmp");
    return Builder->CreateUIToFP(L, Type::getDoubleTy(*TheContext), "booltmp");
  case '=': {
    VariableExprAST *LHSE = static_cast<VariableExprAST *>(LHS.get());
    if (!LHSE)
      return LogErrorV("destination of '=' must be a variable");
    Value *Val = RHS->codegen();
    if (!Val)
      return nullptr;
    Value *Variable = NamedValues[LHSE->getName()]->alloca_inst;
    if (!Variable)
      return LogErrorV("Unknown variable name");
    Builder->CreateStore(Val, Variable);
    return Val;
  } break;
  default:
    //return LogErrorV("invalid binary operator");
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
  if (Callee.rfind(internal_func_prefix, 0) == 0){
    is_internal = true;
    Callee = Callee.substr(internal_func_prefix.size(), Callee.size());
    Log::Info() << "internal function called " << Callee << "\n";
    if (Callee.rfind("llvm_", 0) == 0){
      return callLLVMIntrisic(Callee, Args);
    }
  }
  bool is_function_template = TemplateProtos[Callee] != nullptr;
  if (is_function_template){
    callTemplate(Callee, template_passed_type);
  }
  Function *CalleeF = getFunction(Callee);
  Log::Info() << "is_function_template : " << is_function_template << "\n";
  if (!CalleeF && !is_function_template)
    return LogErrorV("Unknown function referenced %s", Callee.c_str());
  /*if (TemplateProtos[Callee] != nullptr){
    callTemplate(Callee);
  }*/
  Log::Info() << "CalleeF->arg_size : " << CalleeF->arg_size() << "\n";
  Log::Info() << "Args.size : " << Args.size() << "\n";
  if (FunctionProtos[Callee] == nullptr){
    return LogErrorV("Incorrect Function %s", Callee.c_str());
  }
  if (FunctionProtos[Callee]->is_variable_number_args){
    Log::Info() << "Variable number of args" << "\n";
    if (!(Args.size() >= CalleeF->arg_size())){
      return LogErrorV("Incorrect number of arguments passed : %d args but %d expected", Args.size(), CalleeF->arg_size());
    }
  } else {
    // If argument mismatch error.
  if (CalleeF->arg_size() != Args.size())
    return LogErrorV("Incorrect number of arguments passed : %d args but %d expected", Args.size(), CalleeF->arg_size());
  }
  std::vector<Value *> ArgsV;
  for (unsigned i = 0, e = Args.size(); i != e; ++i) {
    Value* temp_val = Args[i]->codegen();
    if (i < FunctionProtos[Callee]->Args.size()){
    if (temp_val->getType() != get_type_llvm(FunctionProtos[Callee]->Args.at(i).second)){
      Log::Info() << "name of arg converting in call expr : " << FunctionProtos[Callee]->Args.at(i).first << "\n";
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
    return LogErrorV("Addr Unknown variable name %s", Name.c_str());
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
    size = Builder->CreateMul(size, ConstantInt::get(*TheContext, APInt(32, 8, false)), "mul_converting_in_bits");
    return size;
  } else {
    Log::Info() << "codegen sizeof is variable" << "\n";
  AllocaInst *A = NamedValues[Name]->alloca_inst;
  if (!A){
    return LogErrorV("Addr Unknown variable name %s", Name.c_str());
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
  FT = FunctionType::get(get_type_llvm(cpoint_type), args_type_main, false);
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
      int type;
      if (i == 1){
        type = argv_type;
      } else {
        type = int_type;
      }
      AllocaInst *Alloca = CreateEntryBlockAlloca(TheFunction, Arg.getName(), Cpoint_Type(type, false));
      if (debug_info_mode){
      debugInfoCreateParameterVariable(SP, Unit, Alloca, Cpoint_Type(type, false), Arg, ArgIdx, LineNo);
      }
      /*DILocalVariable *D = DBuilder->createParameterVariable(
      SP, Arg.getName(), ++ArgIdx, Unit, LineNo, CpointDebugInfo.getDoubleTy(),
      true);

      DBuilder->insertDeclare(Alloca, D, DBuilder->createExpression(),
                          DILocation::get(SP->getContext(), LineNo, 0, SP),
                          Builder->GetInsertBlock());*/
      Builder->CreateStore(&Arg, Alloca);
      NamedValues[std::string(Arg.getName())] = std::make_unique<NamedValue>(Alloca, Cpoint_Type(type, false));
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

  Value *RetVal = nullptr;
  //std::cout << "BODY SIZE : " << Body.size() << std::endl;
  for (int i = 0; i < Body.size(); i++){
    RetVal = Body.at(i)->codegen();
  }
  if (RetVal) {
    // Finish off the function.
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
  return nullptr;
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
  Constant* InitVal = get_default_constant(cpoint_type);
  if (Init){
    InitVal = dyn_cast<ConstantFP>(Init->codegen());
  }
  GlobalVariable* globalVar = new GlobalVariable(*TheModule, get_type_llvm(cpoint_type), /*is constant*/ is_const, GlobalValue::ExternalLinkage, InitVal, varName);
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

  Builder->CreateBr(MergeBB);
  // Codegen of 'Then' can change the current block, update ThenBB for the PHI.
  ThenBB = Builder->GetInsertBlock();
  Value *ElseV = nullptr;
  // Emit else block.
  TheFunction->getBasicBlockList().push_back(ElseBB);
  Builder->SetInsertPoint(ElseBB);
  if (!Else.empty()){
  for (int i = 0; i < Else.size(); i++){
    ElseV = Else.at(i)->codegen();
    if (!ElseV)
      return nullptr;
  }
  } /*else {
    Log::Info() << "Else empty" << "\n";
    ElseV = ConstantFP::get(*TheContext, APFloat(0.0));
  }*/
  Builder->CreateBr(MergeBB);
  // Codegen of 'Else' can change the current block, update ElseBB for the PHI.
  ElseBB = Builder->GetInsertBlock();

  // Emit merge block.
  TheFunction->getBasicBlockList().push_back(MergeBB);
  Builder->SetInsertPoint(MergeBB);
  PHINode *PN = Builder->CreatePHI(Type::getDoubleTy(*TheContext), 2, "iftmp");

  if (ThenV->getType() != Type::getDoubleTy(*TheContext)){
    ThenV = ConstantFP::get(*TheContext, APFloat(0.0));
  }
  if (ElseV == nullptr){
    ElseV = ConstantFP::get(*TheContext, APFloat(0.0));
  } else if (ElseV->getType() != Type::getDoubleTy(*TheContext)){
    ElseV = ConstantFP::get(*TheContext, APFloat(0.0));
  }
  PN->addIncoming(ThenV, ThenBB);
  PN->addIncoming(ElseV, ElseBB);
  return PN;
}

Value* ReturnAST::codegen(){
  Value* value_returned = returned_expr->codegen();
  return value_returned;
  //return ConstantFP::get(*TheContext, APFloat(Val));
}

Value* GotoExprAST::codegen(){
  Function *TheFunction = Builder->GetInsertBlock()->getParent();
  BasicBlock* bb = get_basic_block(TheFunction, label_name);
  if (bb == nullptr){
    Log::Warning() << "Basic block couldn't be found in goto" << "\n";
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
  Log::Info() << "TEST\n";
  if (Val == nullptr){
    return LogErrorV("Val is Nullptr\n");
  }
  Value* ValDeclared = Val->codegen();
  Cpoint_Type type = Cpoint_Type(0);
  if (is_global && GlobalVariables[VariableName] != nullptr){
    type = GlobalVariables[VariableName]->type;
  } else if (NamedValues[VariableName] != nullptr){
    type = NamedValues[VariableName]->type;
  } else {
    Log::Info() << "couldn't find variable" << "\n";
  }
  // TODO convert type for struct members. Either find the type and the member before and store the pos for after to not do loop 2 times, either move this code at the end to have the type
  if (type.type != 0 && member == ""){ 
  if (ValDeclared->getType() != get_type_llvm(type)){
    convert_to_type(get_cpoint_type_from_llvm(ValDeclared->getType()), get_type_llvm(type), ValDeclared);
  }
  }
  bool is_struct = false;
  //bool is_class = false;
  if (member != ""){
  Log::Info() << "objectName : " << VariableName << "\n";
  if (NamedValues[VariableName] != nullptr){
    /*if (ClassDeclarations[NamedValues[VariableName]->type.class_name] != nullptr){
      Log::Info() << "IS_CLASS" << "\n";
      is_class = true;
    }*/
    if (StructDeclarations[NamedValues[VariableName]->type.struct_name] != nullptr){
      Log::Info() << "IS_STRUCT" << "\n";
      is_struct = true;
    }
  }
  //Log::Info() << "className Declar " << NamedValues[VariableName]->type.class_name << "\n";
  /*if (StructDeclarations[ObjectName] != nullptr){
    is_struct = true;
  }
  if (ClassDeclarations[ObjectName] != nullptr){
    is_class = true;
  }*/
  }
  if (is_struct){
    Log::Info() << "StructName : " << VariableName << "\n";
    Log::Info() << "StructName len : " << VariableName.length() << "\n";
    Log::Info() << "StructDeclarations len : " << StructDeclarations.size() << "\n"; 
    auto members = StructDeclarations[NamedValues[VariableName]->type.struct_name]->members;
    Log::Info() << "TEST2" << "\n";
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
    Builder->CreateStore(ValDeclared, ptr);
    NamedValues[VariableName] = std::make_unique<NamedValue>(structPtr, cpoint_type);
  } else if (is_array) {
    Log::Info() << "array redeclaration" << "\n";
    //Log::Info() << "Pos for GEP : " << pos_array << "\n";
    Log::Info() << "ArrayName : " << VariableName << "\n";
    auto zero = llvm::ConstantInt::get(*TheContext, llvm::APInt(64, 0, true));
    //auto one = llvm::ConstantInt::get(*TheContext, llvm::APInt(32, 1, true));
    //auto index = llvm::ConstantInt::get(*TheContext, llvm::APInt(32, pos_array, true)); 
    auto indexVal = index->codegen();
    indexVal = Builder->CreateFPToUI(indexVal, Type::getInt32Ty(*TheContext), "cast_gep_index");
    if (!index){
      return LogErrorV("couldn't find index for array %s", VariableName.c_str());
    }
    auto arrayPtr = NamedValues[VariableName]->alloca_inst;
    Cpoint_Type cpoint_type = NamedValues[VariableName]->type;
    Log::Info() << "Number of member in array : " << cpoint_type.nb_element << "\n";
    Type* llvm_type = get_type_llvm(cpoint_type);
    Log::Info() << "Get LLVM TYPE" << "\n";
    auto ptr = Builder->CreateGEP(llvm_type, arrayPtr, {zero, indexVal}, "get_array");
    Log::Info() << "Create GEP" << "\n";
    Builder->CreateStore(ValDeclared, ptr);
    NamedValues[VariableName] = std::make_unique<NamedValue>(arrayPtr, cpoint_type);
  } else {
  Cpoint_Type cpoint_type =  is_global ? GlobalVariables[VariableName]->type : NamedValues[VariableName]->type;
  if (is_global){
    Builder->CreateStore(ValDeclared, GlobalVariables[VariableName]->globalVar);
  } else {
  AllocaInst *Alloca = CreateEntryBlockAlloca(TheFunction, VariableName, cpoint_type);
  if (ValDeclared->getType() == get_type_llvm(Cpoint_Type(void_type))){
    return LogErrorV("Assigning to a variable a void value");
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
      return LogErrorV("variable for loop already exists in the context");
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
      //return LogErrorV("Expected a Variable Expression in loop in");
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
    Value* ptr = Builder->CreateGEP(get_type_llvm(cpoint_type), TempValueArray, { zero, index}, "gep_loop_in");
    Value* value = Builder->CreateLoad(get_type_llvm(cpoint_type), ptr, VarName);
    Builder->CreateStore(value, TempValueArray);
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

  Function *F = getFunction(std::string("unary") + Opcode);
  if (!F){
    Log::Info() << "UnaryExprAST : " << Opcode << "\n";
    return LogErrorV("Unknown unary operator %c", Opcode);
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
    if (Init) {
      InitVal = Init->codegen();
      if (!InitVal)
        return nullptr;
      if (infer_type){
        cpoint_type = Cpoint_Type(get_cpoint_type_from_llvm(InitVal->getType()));
      }
    } else { // If not specified, use 0.0.
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
    if (!get_type_llvm(cpoint_type)->isPointerTy()){
    Log::Warning() << "cpoint_type in var " << VarNames[i].first << " is not ptr" << "\n";
    }
    if (index == nullptr){
    if (InitVal->getType() != get_type_llvm(cpoint_type)){
      convert_to_type(get_cpoint_type_from_llvm(InitVal->getType()), get_type_llvm(cpoint_type), InitVal);
    }
    }
    if (InitVal->getType() == get_type_llvm(Cpoint_Type(void_type))){
       return LogErrorV("Assigning to a variable as default value a void value");
    }
    Builder->CreateStore(InitVal, Alloca);

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
      if (StructDeclarations[cpoint_type.struct_name] == nullptr){
        return LogErrorV("Couldn't find struct declaration %s for created variable", cpoint_type.struct_name.c_str());
      }
      struct_type_temp = StructDeclarations[cpoint_type.struct_name]->struct_type;
      struct_declaration_name_temp = cpoint_type.struct_name;
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
