#include <iostream>
#include <fstream>
#include <map>
#include <cstdio>
#include <cstdlib>
#include "llvm/IR/Module.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetOptions.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/Host.h"
#include "llvm/Support/FileSystem.h"
#include "lexer.h"
#include "ast.h"
#include "codegen.h"
using namespace std;
using namespace llvm;
using namespace llvm::sys;

#ifdef _WIN32
#define DLLEXPORT __declspec(dllexport)
#else
#define DLLEXPORT
#endif

/// putchard - putchar that takes a double and returns 0.
extern "C" DLLEXPORT double putchard(double X) {
  fputc((char)X, stderr);
  return 0;
}

/// printd - printf that takes a double prints it as "%f\n", returning 0.
extern "C" DLLEXPORT double printd(double X) {
  fprintf(stderr, "%f\n", X);
  return 0;
}

std::map<char, int> BinopPrecedence;
extern std::unique_ptr<Module> TheModule;

extern int CurTok;
//std::unique_ptr<llvm::raw_fd_ostream> file_out_ostream;
llvm::raw_ostream* file_out_ostream;
ifstream file_in;
ofstream file_log;
bool last_line = false;

static void HandleDefinition() {
  if (auto FnAST = ParseDefinition()) {
    if (auto *FnIR = FnAST->codegen()) {
    fprintf(stderr, "Parsed a function definition.\n");
    FnIR->print(*file_out_ostream);
    fprintf(stderr, "\n");
    }
  } else {
    // Skip token for error recovery.
    getNextToken();
  }
}

static void HandleExtern() {
  if (auto ProtoAST = ParseExtern()) {
    if (auto *FnIR = ProtoAST->codegen()) {
      fprintf(stderr, "Read extern: ");
      FnIR->print(*file_out_ostream);
      fprintf(stderr, "\n");
    }
  } else {
    // Skip token for error recovery.
    getNextToken();
  }
}


static void HandleTopLevelExpression() {
  // Evaluate a top-level expression into an anonymous function.
  if (auto FnAST = ParseTopLevelExpr()) {
    if (auto *FnIR = FnAST->codegen()) {
      fprintf(stderr, "Read top-level expression:");
      FnIR->print(*file_out_ostream);
      fprintf(stderr, "\n");

      // Remove the anonymous expression.
      FnIR->eraseFromParent();
    }
  } else {
    // Skip token for error recovery.
    getNextToken();
  }
}


static void MainLoop() {
  while (1) {
    //fprintf(stderr, "ready> ");
    if (last_line == true){
      return;
    }
    switch (CurTok) {
    case tok_eof:
      return;
    case ';': // ignore top-level semicolons.
      getNextToken();
      break;
    case tok_func:
      HandleDefinition();
      break;
    case tok_extern:
      HandleExtern();
      break;
    default:
      HandleTopLevelExpression();
      break;
    }
  }
}


int main(int argc, char **argv){
    string filename ="";
    string object_filename = "out.o";
    bool debug_mode = false;
    for (int i = 1; i < argc; i++){
        string arg = argv[i];
        if (arg.compare("-d") == 0){
            cout << "debug mode" << endl;
            debug_mode = true;
        } else if (arg.compare("-o") == 0){
          i++;
          object_filename = argv[i];
          cout << "object_filename " << object_filename << endl;
        } else {
            cout << "filename : " << arg << endl;
            filename = arg;
        }
    }
    std::error_code ec;
    file_log.open("cpoint.log");
    file_out_ostream = new raw_fd_ostream(llvm::StringRef("out.ll"), ec);
    file_in.open(filename);
    //file_out_ostream = std::make_unique<llvm::raw_fd_ostream>(llvm::StringRef(filename), &ec);
    //file_out_ostream = raw_fd_ostream(llvm::StringRef(filename), &ec);
    // Install standard binary operators.
    // 1 is lowest precedence.
    BinopPrecedence['<'] = 10;
    BinopPrecedence['+'] = 20;
    BinopPrecedence['-'] = 20;
    BinopPrecedence['*'] = 40;  // highest.
    //fprintf(stderr, "ready> ");
    getNextToken();
    InitializeModule();
    TheModule->print(*file_out_ostream, nullptr);
    MainLoop();
    file_in.close();
    file_log.close();
    auto TargetTriple = sys::getDefaultTargetTriple();
    InitializeAllTargetInfos();
    InitializeAllTargets();
    InitializeAllTargetMCs();
    InitializeAllAsmParsers();
    InitializeAllAsmPrinters();
    TheModule->setTargetTriple(TargetTriple);
    std::string Error;
    auto Target = TargetRegistry::lookupTarget(TargetTriple, Error);
    auto CPU = "generic";
    auto Features = "";
    TargetOptions opt;
    auto RM = Optional<Reloc::Model>();
    auto TheTargetMachine = Target->createTargetMachine(TargetTriple, CPU, Features, opt, RM);
    TheModule->setDataLayout(TheTargetMachine->createDataLayout());
    std::error_code EC;
    if (EC) {
    errs() << "Could not open file: " << EC.message();
    return 1;
    }
    raw_fd_ostream dest(llvm::StringRef(object_filename), EC, sys::fs::OF_None);
    legacy::PassManager pass;
    auto FileType = CGFT_ObjectFile;
    if (TheTargetMachine->addPassesToEmitFile(pass, dest, nullptr, FileType)) {
    errs() << "TheTargetMachine can't emit a file of this type";
    return 1;
    }
    pass.run(*TheModule);
    dest.flush();
    outs() << "Wrote " << object_filename << "\n";
    return 0;

}