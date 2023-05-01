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
#include <cstdarg>
#include "ast.h"
#include "lexer.h"
#include "types.h"
#include "log.h"
#include "debuginfo.h"

using namespace llvm;

std::unique_ptr<LLVMContext> TheContext;
std::unique_ptr<Module> TheModule;
std::unique_ptr<IRBuilder<>> Builder;
std::map<std::string, std::unique_ptr<NamedValue>> NamedValues;
std::map<std::string, std::unique_ptr<GlobalVariableValue>> GlobalVariables;
std::map<std::string, std::unique_ptr<PrototypeAST>> FunctionProtos;
std::map<std::string, std::unique_ptr<StructDeclaration>> StructDeclarations;
std::map<std::string, std::unique_ptr<ClassDeclaration>> ClassDeclarations;

std::map<std::string, std::unique_ptr<TemplateType>> TemplateTypes; // the second is temporary I will see what I will put (a AST node ? a class ?)
//std::map<std::string, std::unique_ptr<Struct>> StructsDeclared;

extern std::map<std::string, int> BinopPrecedence;
extern bool isInStruct;
extern bool gc_mode;
extern std::unique_ptr<DIBuilder> DBuilder;
extern struct DebugInfo CpointDebugInfo;

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

static void convertToType(Cpoint_Type typeFrom, Type* typeTo, Value* &val){
  Log::Info() << "Creating cast" << "\n";
  switch (typeFrom.type)
  {
  case int_type:
    if (typeTo == Type::getDoubleTy(*TheContext) || typeTo == Type::getFloatTy(*TheContext)){
      Log::Info() << "From int to float/double" << "\n";
      Log::Info() << "typeFrom " << typeFrom.type << "\n";
      val = Builder->CreateSIToFP(val, typeTo, "cast");
    } else if (typeTo == Type::getInt32Ty(*TheContext)){
      break;
    }
    break;
  case float_type:
  case double_type:
    if (typeTo == Type::getInt32Ty(*TheContext)){
      Log::Info() << "From float/double to int" << "\n";
      val = Builder->CreateFPToUI(val, typeTo, "cast");
    } else if (typeTo == Type::getDoubleTy(*TheContext) || typeTo == Type::getFloatTy(*TheContext)){
      break;
    }
    break;
  default:
    break;
  }
}

static void AllocateMemory(Function* function, std::string VarName, Cpoint_Type type){
    // TODO return alloca ? because of different types, probably no return and add in function to table og alloca inst or pass pointer to variable instead of returning the value
    if (gc_mode){
    Function *CalleeF = getFunction("gc_malloc");
    Type* llvm_type = get_type_llvm(type);
    auto size = llvm::ConstantInt::get(llvm::Type::getInt64Ty(*TheContext), TheModule->getDataLayout().getTypeAllocSize(llvm_type));
    std::vector<Value *> ArgsV;
    ArgsV.push_back(size);
    Builder->CreateCall(CalleeF, ArgsV, "calltmp");
    } else {
    CreateEntryBlockAlloca(function, VarName, type);
    }
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
  string->print(outs());
  return string;
}

Value *VariableExprAST::codegen() {
  // Look this variable up in the function.
  if (GlobalVariables[Name] != nullptr){
    return Builder->CreateLoad(get_type_llvm(type), GlobalVariables[Name]->globalVar, Name.c_str());
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
    AllocaInst* Alloca = NamedValues[StructName]->alloca_inst;
    if (!NamedValues[StructName]->type.is_struct){
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
    if (StructDeclarations[NamedValues[StructName]->type.struct_name] == nullptr){
      Log::Info() << "NULLPTR" << "\n";
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
    Log::Info() << "Pos for GEP struct member " << pos << "\n";
    auto zero = llvm::ConstantInt::get(*TheContext, llvm::APInt(64, 0, true));
    auto index = llvm::ConstantInt::get(*TheContext, llvm::APInt(32, pos, true));
    Cpoint_Type cpoint_type = NamedValues[StructName]->type;
    if (cpoint_type.is_ptr){
      cpoint_type.is_ptr = false;
    }
    Value* ptr = Builder->CreateGEP(get_type_llvm(cpoint_type), Alloca, { zero, index});
    Value* value = Builder->CreateLoad(get_type_llvm(cpoint_type), ptr, StructName);
    return value;
}

Value* ClassMemberExprAST::codegen(){
  AllocaInst* Alloca = NamedValues[ClassName]->alloca_inst;
  if (!NamedValues[ClassName]->type.is_class){
    return LogErrorV("Using a member of variable even though it is not a class");
  }
  auto zero = llvm::ConstantInt::get(*TheContext, llvm::APInt(64, 0, true));
  auto members = std::move(ClassDeclarations[NamedValues[ClassName]->struct_declaration_name]->members);
  int pos = -1;
  for (int i = 0; i < members.size(); i++){
    if (members.at(i).first == MemberName){
      pos = i;
      break;
    }
  }
  auto index = llvm::ConstantInt::get(*TheContext, llvm::APInt(32, pos, true));
  Type* type_llvm = NamedValues[ClassName]->struct_type;
  Value* ptr = Builder->CreateGEP(type_llvm, Alloca, { zero, index});
  Value* value = Builder->CreateLoad(type_llvm, ptr, ClassName);
  return value;
}

Value* ArrayMemberExprAST::codegen() {
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
  //auto index = llvm::ConstantInt::get(*TheContext, llvm::APInt(32, pos, true));
  //auto ptr = GetElementPtrInst::Create(Alloca, { zero, index}, "", );
  Type* type_llvm = get_type_llvm(cpoint_type);
  Value* ptr = Builder->CreateGEP(type_llvm, Alloca, { zero, index});
  Value* value = Builder->CreateLoad(type_llvm, ptr, ArrayName);
  //Value* ptr = Builder->CreateGEP(double_type, ); //  TODO : make the code find the type of the array
  return value;
}

Type* ClassDeclarAST::codegen(){
  Log::Info() << "CODEGEN CLASS DECLAR" << "\n";
  StructType* classType = StructType::create(*TheContext);
  classType->setName(Name);
  std::vector<Type*> dataTypes;
  std::vector<std::pair<std::string,Cpoint_Type>> members;
  for (int i = 0; i < Vars.size(); i++){
    std::unique_ptr<VarExprAST> VarExpr = std::move(Vars.at(i));
    Type* var_type = get_type_llvm(*(VarExpr->cpoint_type));
    dataTypes.push_back(var_type);
    std::string VarName = VarExpr->VarNames.at(0).first;
    members.push_back(std::make_pair(VarName, *(VarExpr->cpoint_type)));
  }
  classType->setBody(dataTypes);
  ClassDeclarations[Name] = std::make_unique<ClassDeclaration>(classType, std::move(members));
  Log::Info() << "added class declaration name " << Name << " to ClassDeclarations" << "\n";
  for (int i = 0; i < Functions.size(); i++){
    std::unique_ptr<FunctionAST> FunctionExpr = std::move(Functions.at(i));
    std::string newNameFunc = "";
    newNameFunc = Name;
    newNameFunc.append("__");
    Log::Info() << "FunctionExpr->Proto->Name : " << FunctionExpr->Proto->Name << "\n";
    if (FunctionExpr->Proto->Name == Name){
    // Constructor
    newNameFunc.append("Constructor__Default");
    } else {
    newNameFunc.append(FunctionExpr->Proto->Name);
    }
    FunctionExpr->Proto->Args.insert(FunctionExpr->Proto->Args.begin(), std::make_pair("this", *get_cpoint_type_from_llvm(classType->getPointerTo()) /* TODO fix this by passing class type pointer as first arg */));
    FunctionExpr->Proto->Name = newNameFunc;
    FunctionExpr->codegen();
    
  }
  return classType;
}

Type* StructDeclarAST::codegen(){
  Log::Info() << "codegen struct" << "\n";
  StructType* structType = StructType::create(*TheContext);
  structType->setName(Name);
  std::vector<Type*> dataTypes;
  std::vector<std::pair<std::string,Cpoint_Type>> members;
  members.clear();
  for (int i = 0; i < Vars.size(); i++){
    std::unique_ptr<VarExprAST> VarExpr = std::move(Vars.at(i));
    Type* var_type = get_type_llvm(*(VarExpr->cpoint_type));
    std::string VarName = VarExpr->VarNames.at(0).first;
    dataTypes.push_back(var_type);
    members.push_back(std::make_pair(VarName, *(VarExpr->cpoint_type)));
    //dataTypes.push_back(Type::getDoubleTy(*TheContext));
  }
  structType->setBody(dataTypes);
  std::unique_ptr<StructDeclarAST> structDeclarASTTemp = std::make_unique<StructDeclarAST>(this->Name, std::move(this->Vars));
  Log::Info() << "members size before : " << members.size() << "\n";
  Log::Info() << "Name struct : " << Name << "\n";
  Log::Info() << "Name struct length : " << Name.length() << "\n";
  StructDeclarations[Name] = std::make_unique<StructDeclaration>(std::move(structDeclarASTTemp), structType, std::move(members));
  return structType;
}

Value *BinaryExprAST::codegen() {
  Value *L = LHS->codegen();
  Value *R = RHS->codegen();
  if (!L || !R)
    return nullptr;
  if (L->getType() != R->getType()){
    convertToType(*get_cpoint_type_from_llvm(R->getType()), L->getType(), R);
  }
  
  if (Op == "=="){
    Log::Info() << "Codegen ==" << "\n";
    L = Builder->CreateFCmpUEQ(L, R, "cmptmp");
    return Builder->CreateUIToFP(L, Type::getDoubleTy(*TheContext), "booltmp");
  }
  if (Op == "||"){
    L = Builder->CreateOr(L, R, "ortmp");
    return Builder->CreateUIToFP(L, Type::getDoubleTy(*TheContext), "booltmp");
  }
  if (Op == "&&"){
    L = Builder->CreateAnd(L, R, "andtmp");
    return Builder->CreateUIToFP(L, Type::getDoubleTy(*TheContext), "booltmp");
  }
  if (Op == "!="){
    L = Builder->CreateFCmpUNE(L, R, "cmptmp");
    return Builder->CreateUIToFP(L, Type::getDoubleTy(*TheContext), "booltmp");
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
    return Builder->CreateFAdd(L, R, "addtmp");
  case '-':
    return Builder->CreateFSub(L, R, "subtmp");
  case '*':
    return Builder->CreateFMul(L, R, "multmp");
  case '%':
    return Builder->CreateFRem(L, R, "remtmp");
  case '<':
    L = Builder->CreateFCmpOLT(L, R, "cmptmp");
    // Convert bool 0/1 to double 0.0 or 1.0
    return Builder->CreateUIToFP(L, Type::getDoubleTy(*TheContext), "booltmp");
  case '>':
    L = Builder->CreateFCmpOGT(R, L, "cmptmp");
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
  if (Callee.rfind(internal_func_prefix, 0) == 0){
    is_internal = true;
    Callee = Callee.substr(internal_func_prefix.size(), Callee.size());
    Log::Info() << "internal function called " << Callee << "\n";
    if (Callee.rfind("llvm_", 0) == 0){
      Log::Info() << "llvm intrisic called" << "\n";
      Callee = Callee.substr(4, Callee.size());
      llvm::Intrinsic::IndependentIntrinsics intrisicId;
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
  }
  Function *CalleeF = getFunction(Callee);
  if (!CalleeF)
    return LogErrorV("Unknown function referenced %s", Callee.c_str());
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
      convertToType(*get_cpoint_type_from_llvm(temp_val->getType()) , get_type_llvm(FunctionProtos[Callee]->Args.at(i).second), temp_val);
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
  auto zero = llvm::ConstantInt::get(*TheContext, llvm::APInt(64, 0, true));
  auto one = llvm::ConstantInt::get(*TheContext, llvm::APInt(32, 1, true));
  if (is_type(Name)){
    int type = get_type(Name);
    Cpoint_Type cpoint_type = Cpoint_Type(type);
    Type* llvm_type = get_type_llvm(cpoint_type);
    Value* size = Builder->CreateGEP(llvm_type->getPointerTo(), Builder->CreateIntToPtr(ConstantInt::get(Builder->getInt64Ty(), 0),llvm_type->getPointerTo()), {one});
    size =  Builder->CreatePtrToInt(size, get_type_llvm(Cpoint_Type(int_type)));
    size  = Builder->CreateFPToUI(size, get_type_llvm(Cpoint_Type(int_type)), "cast");
    return size;
    //Type* llvm_type_ptr = llvm_type->getPointerTo();
    //Value* size = Builder->CreateGEP(llvm_type_ptr->getContainedType(0UL), Builder->CreateIntToPtr(ConstantInt::get(Builder->getInt8Ty(), 0), llvm_type_ptr), ConstantInt::get(Builder->getInt8Ty(), 1));
    //return Builder->CreatePtrToInt(size, get_type_llvm(Cpoint_Type(double_type)));
  } else {
  Function *TheFunction = Builder->GetInsertBlock()->getParent();
  AllocaInst *A = NamedValues[Name]->alloca_inst;
  if (!A){
    return LogErrorV("Addr Unknown variable name %s", Name.c_str());
  }
  Type* llvm_type = A->getAllocatedType();
  Value* size = Builder->CreateGEP(llvm_type->getPointerTo(), Builder->CreateIntToPtr(ConstantInt::get(Builder->getInt64Ty(), 0),llvm_type->getPointerTo()), {one});
  size =  Builder->CreatePtrToInt(size, get_type_llvm(Cpoint_Type(int_type)));
  size  = Builder->CreateFPToUI(size, get_type_llvm(Cpoint_Type(int_type)), "cast");
  return size;
  /*auto zero = llvm::ConstantInt::get(*TheContext, llvm::APInt(64, 0, true));
  auto one = llvm::ConstantInt::get(*TheContext, llvm::APInt(32, 1, true));
  AllocaInst* temp_ptr = CreateEntryBlockAlloca(TheFunction, "temp_sizeof", *get_cpoint_type_from_llvm(llvm_type->getPointerTo()));
  Value* Val = Builder->CreateLoad(llvm_type->getPointerTo(), A, "temp_load_sizeof");
  Builder->CreateStore(Val, temp_ptr);
  Value* size = Builder->CreateGEP(Val->getType(), temp_ptr, {zero, one}, "sizeof");
  return Builder->CreatePtrToInt(size, get_type_llvm(Cpoint_Type(double_type)));*/
  } 
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
  FunctionProtos[this->getName()] = std::make_unique<PrototypeAST>(this->Name, this->Args, this->cpoint_type, this->IsOperator, this->Precedence, this->is_variable_number_args);
  return F;
}

Function *FunctionAST::codegen() {
  auto &P = *Proto;
  Log::Info() << "FunctionAST Codegen : " << Proto->getName() << "\n";
  FunctionProtos[Proto->getName()] = std::move(Proto);
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

  unsigned LineNo = 0;
  DIFile *Unit = DBuilder->createFile(CpointDebugInfo.TheCU->getFilename(),
                                      CpointDebugInfo.TheCU->getDirectory());
  DIScope *FContext = Unit;
  unsigned ScopeLine = 0;
  DISubprogram *SP = DBuilder->createFunction(
    FContext, P.getName(), StringRef(), Unit, LineNo,
    CreateFunctionType(P.cpoint_type, P.Args),
    ScopeLine,
    DINode::FlagPrototyped,
    DISubprogram::SPFlagDefinition);
  TheFunction->setSubprogram(SP);

  // Record the function arguments in the NamedValues map.
  NamedValues.clear();
  TemplateTypes.clear();
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
      Builder->CreateStore(&Arg, Alloca);
      NamedValues[std::string(Arg.getName())] = std::make_unique<NamedValue>(Alloca, Cpoint_Type(type, false));
      i++;
    }
  } else {
  int i = 0;
  unsigned ArgIdx = 0;
  for (auto &Arg : TheFunction->args()){
    Cpoint_Type cpoint_type_arg = P.Args.at(i).second;
    AllocaInst *Alloca = CreateEntryBlockAlloca(TheFunction, Arg.getName(), cpoint_type_arg);

    DILocalVariable *D = DBuilder->createParameterVariable(
      SP, Arg.getName(), ++ArgIdx, Unit, LineNo, CpointDebugInfo.getDoubleTy(),
      true);
    DBuilder->insertDeclare(Alloca, D, DBuilder->createExpression(),
                          DILocation::get(SP->getContext(), LineNo, 0, SP),
                          Builder->GetInsertBlock());
    
    Builder->CreateStore(&Arg, Alloca);
    NamedValues[std::string(Arg.getName())] = std::make_unique<NamedValue>(Alloca, cpoint_type_arg);
    i++;
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
  return nullptr;
}

void TypeDefAST::codegen(){
  return;
}

GlobalVariable* GlobalVariableAST::codegen(){
  Constant* InitVal = get_default_constant(*cpoint_type);
  if (Init){
    InitVal = dyn_cast<ConstantFP>(Init->codegen());
  }
  GlobalVariable* globalVar = new GlobalVariable(*TheModule, get_type_llvm(*cpoint_type), /*is constant*/ is_const, GlobalValue::ExternalLinkage, InitVal, varName);
  GlobalVariables[varName] = std::make_unique <GlobalVariableValue>(*cpoint_type, globalVar);
  return globalVar;
}

Value *IfExprAST::codegen() {
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
  Log::Info() << "REDECLARATION CODEGEN" << "\n";
  Log::Info() << "VariableName " << VariableName << "\n";
  bool is_object = false;
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
  /*for (int i = 0; i < VariableName.length(); i++){
    if (VariableName.at(i) == '.'){
      Log::Info() << "i struct : " << i << "\n";
      is_object = true;
      ObjectName =  VariableName.substr(0, i);
      MemberName =  VariableName.substr(i+1, VariableName.length()-1);
      break;
    }
    if (VariableName.at(i) == '['){
      is_array = true;
      ArrayName = VariableName.substr(0, i);
      std::string pos_str = "";
      for (int j = i + 1; j < VariableName.length(); j++){
        if (VariableName.at(j) == ']'){
          break;
        } else {
          pos_str += VariableName.at(j);
        }
      }
      pos_array = std::stoi(pos_str);
      break;
    }
  }*/
  Log::Info() << "VariableName : " << VariableName << "\n";
  Function *TheFunction = Builder->GetInsertBlock()->getParent();
  Log::Info() << "TEST\n";
  if (Val == nullptr){
    return LogErrorV("Val is Nullptr\n");
  }
  Value* ValDeclared = Val->codegen();
  Cpoint_Type* type = nullptr;
  if (is_global && GlobalVariables[VariableName] != nullptr){
    type = new Cpoint_Type(GlobalVariables[VariableName]->type);
  } else if (NamedValues[VariableName] != nullptr){
    type = new Cpoint_Type(NamedValues[VariableName]->type);
  } else {
    Log::Info() << "couldn't find variable" << "\n";
  }
  if (type != nullptr){
  if (ValDeclared->getType() != get_type_llvm(*type)){
    convertToType(*get_cpoint_type_from_llvm(ValDeclared->getType()), get_type_llvm(*type), ValDeclared);
  }
  }
  bool is_struct = false;
  bool is_class = false;
  if (member != ""){
  Log::Info() << "objectName : " << VariableName << "\n";
  if (NamedValues[VariableName] != nullptr){
    if (ClassDeclarations[NamedValues[VariableName]->type.class_name] != nullptr){
      Log::Info() << "IS_CLASS" << "\n";
      is_class = true;
    }
    if (StructDeclarations[NamedValues[VariableName]->type.struct_name] != nullptr){
      Log::Info() << "IS_STRUCT" << "\n";
      is_struct = true;
    }
  }
  Log::Info() << "className Declar " << NamedValues[VariableName]->type.class_name << "\n";
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
  } else if (is_class){
    Log::Info() << "class redeclaration" << "\n";
    auto members = ClassDeclarations[NamedValues[VariableName]->type.class_name]->members;
    int pos_class = -1;
    for (int i = 0; i < members.size(); i++){
      Log::Info() << "member nb " << i << " " << members.at(i).first << "\n";
      if (members.at(i).first == member){
        pos_class = i;
        break;
      }
    }
    Log::Info() << "Pos for GEP class member redeclaration : " << pos_class << "\n";
    auto zero = llvm::ConstantInt::get(*TheContext, llvm::APInt(64, 0, true));
    auto index = llvm::ConstantInt::get(*TheContext, llvm::APInt(32, pos_class, true));
    auto classPtr = NamedValues[VariableName]->alloca_inst;
    Cpoint_Type cpoint_type = NamedValues[VariableName]->type;
    auto ptr = Builder->CreateGEP(get_type_llvm(cpoint_type), classPtr, {zero, index}, "get_class");
    Builder->CreateStore(ValDeclared, ptr);
    NamedValues[VariableName] = std::make_unique<NamedValue>(classPtr, cpoint_type);
  } else if (is_array) {
    Log::Info() << "array redeclaration" << "\n";
    //Log::Info() << "Pos for GEP : " << pos_array << "\n";
    Log::Info() << "ArrayName : " << VariableName << "\n";
    auto zero = llvm::ConstantInt::get(*TheContext, llvm::APInt(64, 0, true));
    auto one = llvm::ConstantInt::get(*TheContext, llvm::APInt(32, 1, true));
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
    //auto ptr = llvm::GetElementPtrInst::Create(arrayPtr, { zero, index }, "", block);
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

Value* LoopExprAST::codegen(){
  Function *TheFunction = Builder->GetInsertBlock()->getParent();
  if (is_infinite_loop){
  BasicBlock* loopBB = BasicBlock::Create(*TheContext, "loop_infinite", TheFunction);
  Builder->CreateBr(loopBB);
  Builder->SetInsertPoint(loopBB);
  for (int i = 0; i < Body.size(); i++){
    if (!Body.at(i)->codegen())
      return nullptr;
  }
  Builder->CreateBr(loopBB);
  return Constant::getNullValue(Type::getDoubleTy(*TheContext));
  } else {
    return LogErrorV("Functionnality not finished to be implemented");
    if (NamedValues[VarName] != nullptr){
      return LogErrorV("variable for loop already exists in the context");
    }
    AllocaInst *allocaPos = CreateEntryBlockAlloca(TheFunction, VarName, Cpoint_Type(double_type, false));
    Value *StartVal = ConstantFP::get(*TheContext, APFloat(0.0));
    Builder->CreateStore(StartVal, allocaPos);
    
    BasicBlock *LoopBB = BasicBlock::Create(*TheContext, "loop_loop_in", TheFunction);
    Builder->CreateBr(LoopBB);
    Builder->SetInsertPoint(LoopBB);
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
  Builder->CreateCondBr(CondV, LoopBB, AfterBB);
  //Builder->CreateBr(LoopBB);
  Builder->SetInsertPoint(LoopBB);
  for (int i = 0; i < Body.size(); i++){
    if (!Body.at(i)->codegen())
      return nullptr;
  }
  Builder->CreateBr(whileBB);
  Builder->SetInsertPoint(AfterBB);
  return Constant::getNullValue(Type::getDoubleTy(*TheContext));
}

Value *ForExprAST::codegen(){
  Function *TheFunction = Builder->GetInsertBlock()->getParent();
  AllocaInst *Alloca = CreateEntryBlockAlloca(TheFunction, VarName, Cpoint_Type(double_type, false));
  Value *StartVal = Start->codegen();
  if (!StartVal)
    return nullptr;
  Builder->CreateStore(StartVal, Alloca);

  //BasicBlock *PreheaderBB = Builder->GetInsertBlock();
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
  /*PHINode *Variable = Builder->CreatePHI(Type::getDoubleTy(*TheContext), 2, VarName);
  Variable->addIncoming(StartVal, PreheaderBB);
  Value *OldVal = NamedValues[VarName];
  NamedValues[VarName] = Variable;*/
  for (int i = 0; i < Body.size(); i++){
    if (!Body.at(i)->codegen())
      return nullptr;
  }
  Value *StepVal = nullptr;
  if (Step) {
    StepVal = Step->codegen();
    if (!StepVal)
      return nullptr;
  } else {
    // If not specified, use 1.0.
    StepVal = ConstantFP::get(*TheContext, APFloat(1.0));
  }

  //Value *NextVar = Builder->CreateFAdd(Variable, StepVal, "nextvar");

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
  BasicBlock *AfterBB =
      BasicBlock::Create(*TheContext, "afterloop", TheFunction);

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
        cpoint_type = std::make_unique<Cpoint_Type>(*get_cpoint_type_from_llvm(InitVal->getType()));
      }
    } else { // If not specified, use 0.0.
      InitVal = get_default_value(*cpoint_type);
    }
    llvm::Value* indexVal = nullptr;
    double indexD = -1;
    if (index != nullptr){
    indexVal = index->codegen();
    auto constFP = dyn_cast<ConstantFP>(indexVal);
    double indexD = constFP->getValueAPF().convertToDouble();
    Log::Info() << "index for varexpr array : " << indexD << "\n";
    cpoint_type->nb_element = indexD;
    }
    AllocaInst *Alloca = CreateEntryBlockAlloca(TheFunction, VarName, *cpoint_type);
    if (!get_type_llvm(*cpoint_type)->isPointerTy()){
    Log::Warning() << "cpoint_type in var " << VarNames[i].first << " is not ptr" << "\n";
    }
    if (index != nullptr){
    if (InitVal->getType() != get_type_llvm(*cpoint_type)){
      convertToType(*get_cpoint_type_from_llvm(InitVal->getType()), get_type_llvm(*cpoint_type), InitVal);
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
    if (cpoint_type->is_struct){
      struct_type_temp = StructDeclarations[cpoint_type->struct_name]->struct_type;
      struct_declaration_name_temp = cpoint_type->struct_name;
    }
    if (cpoint_type->is_class){
      // use the same member in NamedValue class for structs and class
      Log::Info() << "IS CLASS" << "\n";
      Log::Info() << "class name : " << cpoint_type->class_name << "\n";
      if (ClassDeclarations[cpoint_type->class_name] == nullptr){
        return LogErrorV("Couldn't find class declaration %s for created variable", cpoint_type->class_name);
      }
      struct_type_temp = ClassDeclarations[cpoint_type->class_name]->class_type;
      struct_declaration_name_temp = cpoint_type->class_name;
      if (!cpoint_type->is_ptr){
      Function* constructorF = getFunction(cpoint_type->class_name + "__Constructor__Default");
      std::vector<Value *> ArgsV;
      /*auto A = NamedValues[VarName]->alloca_inst;
      Value* thisClass = Builder->CreateLoad(PointerType::get(A->getAllocatedType(), A->getAddressSpace()), A, VarName.c_str());
      ArgsV.push_back(thisClass);*/
      Builder->CreateCall(constructorF, ArgsV, "calltmp");
      }
    }
    Log::Info() << "VarName " << VarName << "\n";
    Log::Info() << "struct_declaration_name_temp " << struct_declaration_name_temp << "\n";
    NamedValues[VarName] = std::make_unique<NamedValue>(Alloca, *cpoint_type, struct_type_temp, struct_declaration_name_temp);
    Log::Info() << "NamedValues[VarName]->struct_declaration_name : " <<  NamedValues[VarName]->struct_declaration_name << "\n";
  }

  // Codegen the body, now that all vars are in scope.
  /*Value *BodyVal = Body->codegen();
  if (!BodyVal)
    return nullptr;
  */
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
  //std::cout << "CREATED MODULE" << std::endl;

  // Create a new builder for the module.
  Builder = std::make_unique<IRBuilder<>>(*TheContext);
}
