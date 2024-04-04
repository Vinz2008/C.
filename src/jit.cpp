#include "config.h"

#if ENABLE_JIT

#include "jit.h"
#include "ast.h"
#include "lexer.h"
#include "codegen.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetOptions.h"

extern std::unique_ptr<Module> TheModule;
extern std::unique_ptr<LLVMContext> TheContext;

std::unique_ptr<llvm::orc::CpointJIT> TheJIT;

void MainLoop();

void initModuleJIT(){
    InitializeModule("jit");
    TheModule->setDataLayout(TheJIT->getDataLayout());
}

void launchJIT(){
    InitializeNativeTarget();
    InitializeNativeTargetAsmPrinter();
    InitializeNativeTargetAsmParser();
    auto JITResult = llvm::orc::CpointJIT::Create();
    if (auto E = JITResult.takeError()){
        errs() << "Problem with creating the JIT " << toString(std::move(E)) << "\n";
        exit(1);
    }
    TheJIT = std::move(*JITResult);
    initModuleJIT();
    fprintf(stderr, "> ");
    MainLoop();
}

std::unique_ptr<FunctionAST> ParseTopLevelExpr() {
  if (auto E = ParseExpression()) {
    // Make an anonymous proto.
    auto Proto = std::make_unique<PrototypeAST>(emptyLoc, "__anon_expr", std::vector<std::pair<std::string, Cpoint_Type>>(), Cpoint_Type(double_type));
    std::vector<std::unique_ptr<ExprAST>> Body;
    Body.push_back(std::move(E));
    return std::make_unique<FunctionAST>(std::move(Proto), std::move(Body));
  }
  return nullptr;
}


extern bool is_last_char_next_line_jit;

void HandleTopLevelExpression(){
  // Evaluate a top-level expression into an anonymous function.
  fprintf(stderr, "TOP LEVEL EXPRESSION\n");
  if (!TheModule || !TheContext){
    initModuleJIT();
  }
  if (auto FnAST = ParseTopLevelExpr()) {
    if (FnAST->codegen()) {
        TheJIT->getLLVMIR();
        auto H = TheJIT->addModule(llvm::orc::ThreadSafeModule(std::move(TheModule), std::move(TheContext)));
        auto ExprSymbol = TheJIT->lookup("__anon_expr");
        assert(ExprSymbol && "Function not found");
        double (*FP)() = (double (*)())(intptr_t)ExprSymbol->getAddress().getValue();
        //void (*FP)() = (void (*)())(intptr_t)ExprSymbol.getAddress();
        fprintf(stderr, "Evaluated to %f\n", FP());
        //FP();
    } else {
        fprintf(stderr, "Couldn't codegen top level expression");
    }
    is_last_char_next_line_jit = false;
  } else {
    // Skip token for error recovery.
    getNextToken();
  }
}

#endif