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
#include "llvm/IR/DIBuilder.h"
#include "config.h"
#include "lexer.h"
#include "ast.h"
#include "codegen.h"
#include "preprocessor.h"
#include "errors.h"
#include "debuginfo.h"
#include "linker.h"
#include "target-triplet.h"
#include "imports.h"
#include "log.h"

using namespace std;
using namespace llvm;
using namespace llvm::sys;

#ifdef _WIN32
#define DLLEXPORT __declspec(dllexport)
#else
#define DLLEXPORT
#endif

int return_status = 0;

extern std::unique_ptr<DIBuilder> DBuilder;
extern std::vector<std::string> PackagesAdded;
struct DebugInfo CpointDebugInfo;
std::unique_ptr<Compiler_context> Comp_context;
string std_path = DEFAULT_STD_PATH;
string filename ="";
bool std_mode = true;

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

void add_manually_extern(std::string fnName, std::unique_ptr<Cpoint_Type> cpoint_type, std::vector<std::pair<std::string, Cpoint_Type>> ArgNames, unsigned Kind, unsigned BinaryPrecedence, bool is_variable_number_args){

  auto FnAST =  std::make_unique<PrototypeAST>(fnName, std::move(ArgNames), std::move(cpoint_type), Kind != 0, BinaryPrecedence, is_variable_number_args);
  auto *FnIR = FnAST->codegen();
}

void add_externs_for_gc(){
  std::vector<std::pair<std::string, Cpoint_Type>> args_gc_init;
  add_manually_extern("gc_init", std::make_unique<Cpoint_Type>(void_type), std::move(args_gc_init), 0, 30, false);
  std::vector<std::pair<std::string, Cpoint_Type>> args_gc_malloc;
  args_gc_malloc.push_back(make_pair("size", Cpoint_Type(int_type)));
  add_manually_extern("gc_malloc", std::make_unique<Cpoint_Type>(void_type, true), std::move(args_gc_malloc), 0, 30, false);
}

static void HandleDefinition() {
  if (auto FnAST = ParseDefinition()) {
    if (auto *FnIR = FnAST->codegen()) {
    Log::Info() << "Parsed a function definition." << "\n";
    //fprintf(stderr, "Parsed a function definition.\n");
    //FnIR->print(*file_out_ostream);
    //fprintf(stderr, "\n");
    }
  } else {
    // Skip token for error recovery.
    getNextToken();
  }
}

static void HandleExtern() {
  if (auto ProtoAST = ParseExtern()) {
    if (auto *FnIR = ProtoAST->codegen()) {
      //fprintf(stderr, "Read extern: ");
      Log::Info() << "Read Extern" << "\n";
      //FnIR->print(*file_out_ostream);
      //fprintf(stderr, "\n");
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
      //fprintf(stderr, "Read top-level expression:");
      Log::Info() << "Read top-level expression" << "\n";
      //FnIR->print(*file_out_ostream);
      //fprintf(stderr, "\n");

      // Remove the anonymous expression.
      //FnIR->eraseFromParent();
    }
  } else {
    // Skip token for error recovery.
    getNextToken();
  }
}

static void HandleStruct() {
  if (auto structAST = ParseStruct()){
    if (auto* structIR = structAST->codegen()){
      //ObjIR->print(*file_out_ostream);
    }
  } else {
    getNextToken();
  }
}

static void MainLoop() {
  while (1) {
    std::flush(file_log);
    //fprintf(stderr, "ready> ");
    if (last_line == true){
      file_log << "exit" << "\n";
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
    case tok_struct:
      HandleStruct();
      break;
    default:
      HandleTopLevelExpression();
      break;
    }
  }
}


int main(int argc, char **argv){
    string object_filename = "out.o";
    string exe_filename = "a.out";
    string temp_output = "";
    bool output_temp_found = false;
    string target_triplet_found;
    bool target_triplet_found_bool = false;
    bool debug_mode = false;
    bool link_files_mode = true;
    bool verbose_std_build = false;
    bool remove_temp_file = true;
    for (int i = 1; i < argc; i++){
        string arg = argv[i];
        if (arg.compare("-d") == 0){
            cout << "debug mode" << endl;
            debug_mode = true;
        } else if (arg.compare("-o") == 0){
          i++;
          temp_output = argv[i];
          cout << "object_filename " << object_filename << endl;
          output_temp_found = true;
        } else if (arg.compare("-std") == 0){
          i++;
          std_path = argv[i];
          cout << "custom path std : " << std_path << endl;
        } else if (arg.compare("-c") == 0){
          link_files_mode = false;
        } else if (arg.compare("-nostd") == 0){
          std_mode = false;
        } else if(arg.compare("-target-triplet") == 0){
          target_triplet_found_bool = true;
          i++;
          target_triplet_found = argv[i];
          cout << "target triplet : " << target_triplet_found << endl;
        } else if (arg.compare("-verbose-std-build") == 0){
          verbose_std_build = true;
        } else if (arg.compare("-no-delete-import-file") == 0){
          remove_temp_file = false;
        } else {
          cout << "filename : " << arg << endl;
          filename = arg;
        }
    }
    if (output_temp_found){
      if (link_files_mode){
        exe_filename = temp_output;
      } else {
        object_filename = temp_output;
      }
    }
    init_context_preprocessor();
    Comp_context = std::make_unique<Compiler_context>(filename, 0, 0, "<empty line>");
    std::string temp_filename = filename;
    temp_filename.append(".temp");
    generate_file_with_imports(filename, temp_filename);
    filename = temp_filename;
    std::error_code ec;
    if (debug_mode == false ){
#ifdef _WIN32
      file_log.open("nul");
#else
      file_log.open("/dev/null");
#endif
    } else {
    file_log.open("cpoint.log");
    }
    file_out_ostream = new raw_fd_ostream(llvm::StringRef("out.ll"), ec);
    file_in.open(filename);
    //file_out_ostream = std::make_unique<llvm::raw_fd_ostream>(llvm::StringRef(filename), &ec);
    //file_out_ostream = raw_fd_ostream(llvm::StringRef(filename), &ec);
    // Install standard binary operators.
    // 1 is lowest precedence.
    BinopPrecedence['<'] = 10;
    BinopPrecedence['>'] = 10;
    BinopPrecedence['+'] = 20;
    BinopPrecedence['-'] = 20;
    BinopPrecedence['*'] = 40;  // highest.
    //fprintf(stderr, "ready> ");
    string TargetTriple;
    if (target_triplet_found_bool){
    TargetTriple = target_triplet_found;
    } else {
    TargetTriple = sys::getDefaultTargetTriple();
    }
    std::string os_name = get_os(TargetTriple);
    setup_preprocessor(TargetTriple);
    getNextToken();
    InitializeModule(filename);
    TheModule->addModuleFlag(Module::Warning, "Debug Info Version",
                           DEBUG_METADATA_VERSION);
    if (Triple(sys::getProcessTriple()).isOSDarwin()){
      TheModule->addModuleFlag(llvm::Module::Warning, "Dwarf Version", 2);
    }
    DBuilder = std::make_unique<DIBuilder>((*TheModule));
    CpointDebugInfo.TheCU = DBuilder->createCompileUnit(
      dwarf::DW_LANG_C, DBuilder->createFile(filename, "."),
      "Cpoint Compiler", false, "", 0);
    DBuilder->finalize();
    if (std_mode){
      add_externs_for_gc();
    }
    MainLoop();
    TheModule->print(*file_out_ostream, nullptr);
    file_in.close();
    file_log.close();
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
    auto RM = Optional<Reloc::Model>(Reloc::Model::DynamicNoPIC);
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
    std::string gc_path = DEFAULT_GC_PATH;
    outs() << "Wrote " << object_filename << "\n";
    if (std_mode && link_files_mode){
      if (build_std(std_path, TargetTriple, verbose_std_build) == -1){
        cout << "Could not build std at path : " << std_path << endl;
        exit(1);
      }
      gc_path = std_path;
      gc_path.append("/../bdwgc");
      build_gc(gc_path, TargetTriple);
      Log::Info() << "TEST" << "\n";
    }
    if (link_files_mode){
      std::vector<string> vect_obj_files;
      if (TargetTriple.find("wasm") == std::string::npos){
      std::string std_static_path = std_path;
      if (std_path.back() != '/'){
        std_static_path.append("/");
      }
      std_static_path.append("libstd.a");
      std::string gc_static_path = gc_path;
      if (gc_path.back() != '/'){
      gc_static_path.append("/");
      }
      gc_static_path.append("../bdwgc_prefix/lib/libgc.a");
      /*std::string std_static_path = "-L";
      std_static_path.append(std_path);
      std_static_path.append(" -lstd");*/
      vect_obj_files.push_back(object_filename);
      if (std_mode){
      vect_obj_files.push_back(std_static_path);
      vect_obj_files.push_back(gc_static_path);
      }
      } else {
        vect_obj_files.push_back(object_filename);
      }
      if (PackagesAdded.empty() == false){
        std::string package_archive_path;
        for (int i = 0; i < PackagesAdded.size(); i++){
          package_archive_path = DEFAULT_PACKAGE_PATH;
          package_archive_path.append("/");
          package_archive_path.append(PackagesAdded.at(i));
          package_archive_path.append("/lib.a");
          vect_obj_files.push_back(package_archive_path);
        }
      }
      link_files(vect_obj_files, exe_filename, TargetTriple);
    }
    if (remove_temp_file){
      remove(temp_filename.c_str());
    }
    return return_status;
}
