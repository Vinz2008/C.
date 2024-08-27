#include "debuginfo.h"
#include "llvm/IR/DIBuilder.h"
//#include "errors.h"
#include "codegen.h"
//#include "ast.h"
//#include "log.h"

using namespace llvm;


std::unique_ptr<DIBuilder> DBuilder;

extern std::unique_ptr<IRBuilder<>> Builder;;

extern struct DebugInfo CpointDebugInfo;

extern bool debug_info_mode;

DIType* DebugInfo::getVoidTy() {
  if (VoidTy)
    return VoidTy;
  VoidTy = nullptr;
  return VoidTy;
}

DIType* DebugInfo::getDoubleTy() {
  if (DoubleTy)
    return DoubleTy;
  DoubleTy = DBuilder->createBasicType("double", 64, dwarf::DW_ATE_float);
  return DoubleTy;
}

DIType* DebugInfo::getFloatTy() {
  if (FloatTy)
    return FloatTy;
  FloatTy = DBuilder->createBasicType("float", 32, dwarf::DW_ATE_float);
  return FloatTy;
}

// size is in bits
DIType* DebugInfo::getIntTy(int size) {
  DIType* IntTy = DBuilder->createBasicType("i" + std::to_string(size), size, dwarf::DW_ATE_signed);
  return IntTy;
}

DIType* DebugInfo::getPtrTy(DIType* baseDiType){
    if (!baseDiType){
        return DBuilder->createBasicType("void*", pointer_width, dwarf::DW_TAG_pointer_type);
    }
    DIType* PtrTy = DBuilder->createPointerType(baseDiType, pointer_width);
    return PtrTy;
}

extern std::unordered_map<std::string, std::unique_ptr<StructDeclaration>> StructDeclarations;

static DIType* get_debuginfo_type(Cpoint_Type type){
  DIType* ret_type = nullptr;
  if (type.is_struct){
    std::string structName = type.struct_name;
    ret_type = StructDeclarations[structName]->struct_debuginfos_type;
  } else {
    if (type.is_signed() || type.is_unsigned()){
    ret_type = CpointDebugInfo.getIntTy(type.get_number_of_bits());
    } else {
    switch (type.type){
    case void_type:
        ret_type = CpointDebugInfo.getVoidTy();
        break;
    case float_type:
        ret_type = CpointDebugInfo.getFloatTy();
        break;
    default:
    case double_type:
        ret_type = CpointDebugInfo.getDoubleTy();
        break;
    }
    }
  }
  if (type.is_ptr){
    /*if (type.type == void_type){
        return CpointDebugInfo.getPtrTy(nullptr);
    }*/
    //Cpoint_Type base_type = type.deref_type();
    return CpointDebugInfo.getPtrTy(ret_type);
  }
  return ret_type;
}

DISubroutineType *DebugInfoCreateFunctionType(Cpoint_Type type, std::vector<std::pair<std::string, Cpoint_Type>> Args) {
  SmallVector<Metadata *, 8> EltTys;
  DIType* return_type = get_debuginfo_type(type);

  // Add the result type.
  EltTys.push_back(return_type);

  for (unsigned i = 0; i < Args.size(); ++i){
    EltTys.push_back(get_debuginfo_type(Args.at(i).second));
  }

  return DBuilder->createSubroutineType(DBuilder->getOrCreateTypeArray(EltTys));
}

DICompositeType* DebugInfoCreateStructType(Cpoint_Type struct_type, std::vector<std::pair<std::string, Cpoint_Type>> Members, int LineNo) {
    // TODO : move this in a get_scope function
    DIScope *Scope;
    if (CpointDebugInfo.LexicalBlocks.empty()){
        Scope = CpointDebugInfo.TheCU;
    } else {
        Scope = CpointDebugInfo.LexicalBlocks.back();
    }
    DIFile* File = CpointDebugInfo.TheCU->getFile();
    SmallVector<Metadata *, 8> MemberTys;
    for (int i = 0; i < Members.size(); i++){
        std::string struct_member_name = Members.at(i).first;
        Cpoint_Type member_cpoint_type = Members.at(i).second;
        DIType* member_type = get_debuginfo_type(member_cpoint_type);
        int size_in_bits = get_type_size(member_cpoint_type)*8;
        MemberTys.push_back(DBuilder->createMemberType(Scope, struct_member_name, File, LineNo,  size_in_bits, 0, 0, DINode::DIFlags::FlagPublic, member_type));
    }
    DITypeRefArray all_member_types = DBuilder->getOrCreateTypeArray(MemberTys);
    return DBuilder->createStructType(Scope, struct_type.struct_name, File, LineNo, get_type_size(struct_type)*8, get_type_size(struct_type)*8, DINode::DIFlags::FlagPublic, nullptr, all_member_types.get());
}


// Add to each class a location attribute for line and column position to create debug infos like in the Kaleidoscope tutorial 

void DebugInfo::emitLocation(Compiler_context context, bool pop_the_scope = false){
  if (pop_the_scope){
    return Builder->SetCurrentDebugLocation(DebugLoc());
  }
  DIScope *Scope;
  if (LexicalBlocks.empty())
    Scope = TheCU;
  else
    Scope = LexicalBlocks.back();
  Builder->SetCurrentDebugLocation(
      DILocation::get(Scope->getContext(), context.lexloc.line_nb, context.lexloc.col_nb, Scope));
}

void DebugInfo::emitLocation(ExprAST* AST){
  if (debug_info_mode){
  if (!AST){
    return Builder->SetCurrentDebugLocation(DebugLoc());
  }
  DIScope *Scope;
  if (LexicalBlocks.empty())
    Scope = TheCU;
  else
    Scope = LexicalBlocks.back();
  Builder->SetCurrentDebugLocation(
      DILocation::get(Scope->getContext(), AST->getLine(), AST->getCol(), Scope));
  }
}

// will probably not use it because it is used not often and we need most of these values
void debugInfoCreateFunction(PrototypeAST &P, Function *TheFunction){
  DIFile *Unit = DBuilder->createFile(CpointDebugInfo.TheCU->getFilename(),
                                      CpointDebugInfo.TheCU->getDirectory());
  DIScope *FContext = Unit;
  unsigned ScopeLine = 0; // TODO : add way to get file line from Prototype
  unsigned LineNo = 0;
  DISubprogram *SP = DBuilder->createFunction(
    FContext, P.getName(), StringRef(), Unit, LineNo,
    DebugInfoCreateFunctionType(P.cpoint_type, P.Args),
    ScopeLine,
    DINode::FlagPrototyped,
    DISubprogram::SPFlagDefinition);
  TheFunction->setSubprogram(SP);
}



void debugInfoCreateParameterVariable(DISubprogram *SP, DIFile *Unit, AllocaInst *Alloca, Cpoint_Type type, Argument& Arg, unsigned& ArgIdx, unsigned LineNo){
  DILocalVariable *D = DBuilder->createParameterVariable(
      SP, Arg.getName(), ++ArgIdx, Unit, LineNo, get_debuginfo_type(type),
      true);
  Log::Info() << "LineNo Var : " << LineNo << "\n";
  DBuilder->insertDeclare(Alloca, D, DBuilder->createExpression(),
                        DILocation::get(SP->getContext(), LineNo, 0, SP),
                        Builder->GetInsertBlock());
}

static BasicBlock* getEntryBlock(BasicBlock* insertBlock){
    if (insertBlock->isEntryBlock()){
        return insertBlock;
    } else {
    Function* TheFunction = insertBlock->getParent();
    for (auto b = TheFunction->begin(), be = TheFunction->end(); b != be; ++b){
        BasicBlock* bb = dyn_cast<BasicBlock>(&*b);
        if (bb->isEntryBlock()){
            return bb;
        }
    }
    }
    return nullptr;
}

void debugInfoCreateLocalVariable(DIScope *SP, DIFile *Unit, AllocaInst *Alloca, Cpoint_Type type, unsigned LineNo){
    DILocalVariable *D = DBuilder->createAutoVariable(SP, Alloca->getName(), Unit, LineNo, get_debuginfo_type(type));
    BasicBlock* insertBlock = Builder->GetInsertBlock();
    BasicBlock* firstBasicBlock = getEntryBlock(insertBlock);
    DBuilder->insertDeclare(Alloca, D, DBuilder->createExpression(), DILocation::get(SP->getContext(), LineNo, 0, SP), firstBasicBlock);
}

void debugInfoCreateNamespace(std::string name){
    DIFile *Unit = DBuilder->createFile(CpointDebugInfo.TheCU->getFilename(), CpointDebugInfo.TheCU->getDirectory());
    DINamespace* namespace_scope = DBuilder->createNameSpace(Unit, name, false);
    // TODO : add this to a queue so we can set the scope to the namespace of functions 
}