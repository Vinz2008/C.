#ifndef LLVM_EXECUTIONENGINE_ORC_KALEIDOSCOPEJIT_H
#define LLVM_EXECUTIONENGINE_ORC_KALEIDOSCOPEJIT_H

#include "llvm/ADT/StringRef.h"
#include "llvm/ExecutionEngine/Orc/CompileOnDemandLayer.h"
#include "llvm/ExecutionEngine/Orc/CompileUtils.h"
#include "llvm/ExecutionEngine/Orc/Core.h"
#include "llvm/ExecutionEngine/Orc/EPCIndirectionUtils.h"
#include "llvm/ExecutionEngine/Orc/ExecutionUtils.h"
#include "llvm/ExecutionEngine/Orc/ExecutorProcessControl.h"
#include "llvm/ExecutionEngine/Orc/IRCompileLayer.h"
#include "llvm/ExecutionEngine/Orc/IRTransformLayer.h"
#include "llvm/ExecutionEngine/Orc/JITTargetMachineBuilder.h"
#include "llvm/ExecutionEngine/Orc/RTDyldObjectLinkingLayer.h"
#include "llvm/ExecutionEngine/Orc/Shared/ExecutorSymbolDef.h"
#include "llvm/ExecutionEngine/SectionMemoryManager.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Transforms/InstCombine/InstCombine.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Scalar/GVN.h"
//#include "llvm/ExecutionEngine/JITSymbol.h"
//#include "llvm/IR/Module.h"
#include <memory>
#include <iostream>

extern std::unique_ptr<llvm::Module> TheModule;

// TODO : clean up this code

namespace llvm {
namespace orc {

class CpointJIT {
private:
  std::unique_ptr<ExecutionSession> ES;
  std::unique_ptr<EPCIndirectionUtils> EPCIU;

  DataLayout DL;
  MangleAndInterner Mangle;

  RTDyldObjectLinkingLayer ObjectLayer;
  IRCompileLayer CompileLayer;
  IRTransformLayer OptimizeLayer;
  CompileOnDemandLayer CODLayer;

  JITDylib &MainJD;

  static void handleLazyCallThroughError() {
    errs() << "LazyCallThrough error: Could not find function body";
    exit(1);
  }

public:
  CpointJIT(std::unique_ptr<ExecutionSession> ES, 
  std::unique_ptr<EPCIndirectionUtils> EPCIU, JITTargetMachineBuilder JTMB, DataLayout DL)
      : ES(std::move(ES)), EPCIU(std::move(EPCIU)), DL(std::move(DL)), Mangle(*this->ES, this->DL),
        ObjectLayer(*this->ES,
                    []() { return std::make_unique<SectionMemoryManager>(); }),
        CompileLayer(*this->ES, ObjectLayer,
                     std::make_unique<ConcurrentIRCompiler>(std::move(JTMB))),
        OptimizeLayer(*this->ES, CompileLayer, optimizeModule),
        CODLayer(*this->ES, OptimizeLayer,
                 this->EPCIU->getLazyCallThroughManager(),
                 [this] { return this->EPCIU->createIndirectStubsManager(); }),
        MainJD(this->ES->createBareJITDylib("<main>")) {
    MainJD.addGenerator(cantFail(DynamicLibrarySearchGenerator::GetForCurrentProcess(DL.getGlobalPrefix())));
  }

  ~CpointJIT() {
    if (auto Err = ES->endSession()){
      ES->reportError(std::move(Err));
    }
    if (auto Err = EPCIU->cleanup()){
      ES->reportError(std::move(Err));
    }
  }

  static Expected<std::unique_ptr<CpointJIT>> Create() {
    auto EPC = SelfExecutorProcessControl::Create();
    if (!EPC){
      return EPC.takeError();
    }
    auto ES = std::make_unique<ExecutionSession>(std::move(*EPC));

    auto EPCIU = EPCIndirectionUtils::Create(*ES);
    if (!EPCIU){
      return EPCIU.takeError();
    }
    (*EPCIU)->createLazyCallThroughManager(*ES, ExecutorAddr::fromPtr(&handleLazyCallThroughError));
    
    if (auto Err = setUpInProcessLCTMReentryViaEPCIU(**EPCIU)){
      return std::move(Err);
    }
    
    JITTargetMachineBuilder JTMB(ES->getExecutorProcessControl().getTargetTriple());

    auto DL = JTMB.getDefaultDataLayoutForTarget();
    if (!DL)
      return DL.takeError();

    return std::make_unique<CpointJIT>(std::move(ES), std::move(*EPCIU), std::move(JTMB),
                                             std::move(*DL));
  }

  const DataLayout &getDataLayout() const { return DL; }

  JITDylib &getMainJITDylib() { return MainJD; }

  Error addModule(ThreadSafeModule TSM, ResourceTrackerSP RT = nullptr) {
    if (!RT)
      RT = MainJD.getDefaultResourceTracker();
    return CompileLayer.add(RT, std::move(TSM));
  }

  Expected<ExecutorSymbolDef> lookup(StringRef Name) {
    return ES->lookup({&MainJD}, Mangle(Name.str()));
  }
  // FOR DEBUG
  void getLLVMIR(){
    std::error_code ec;
    TheModule->print(*new raw_fd_ostream("out-jit.ll", ec), nullptr);
  }
private:
    static Expected<ThreadSafeModule> optimizeModule(ThreadSafeModule TSM, const MaterializationResponsibility &R) {
        TSM.withModuleDo([](Module &M) {
            auto FPM = std::make_unique<legacy::FunctionPassManager>(&M);
            FPM->add(createInstructionCombiningPass());
            FPM->add(createReassociatePass());
            FPM->add(createGVNPass());
            FPM->add(createCFGSimplificationPass());
            FPM->doInitialization();
            for (auto &F : M){
                FPM->run(F);
            }
        });
        return std::move(TSM);
  }
};

} // end namespace orc
} // end namespace llvm

#endif // LLVM_EXECUTIONENGINE_ORC_KALEIDOSCOPEJIT_H

#include "config.h"

#if ENABLE_JIT
void launchJIT();
void HandleTopLevelExpression();
#endif