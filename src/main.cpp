#include <iostream>
#include <fstream>
#include <unordered_map>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <libintl.h>
#include <locale.h>
#include "llvm/IR/Module.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetOptions.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/Host.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/IR/DIBuilder.h"
#include "llvm/Transforms/InstCombine/InstCombine.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Scalar/GVN.h"
#include "config.h"
#include "lexer.h"
#include "ast.h"
#include "codegen.h"
#include "preprocessor.h"
#include "errors.h"
#include "debuginfo.h"
#include "linker.h"
#include "target-triplet.h"
#include "cli.h"
#include "imports.h"
#include "log.h"
#include "utils.h"
#include "color.h"
#include "gettext.h"
#include "c_translator.h"
#include "templates.h"
#include "tests.h"
#include "lto.h"
#include "jit.h"

using namespace std;
using namespace llvm;
using namespace llvm::sys;

#ifdef _WIN32
#define DLLEXPORT __declspec(dllexport)
#else
#define DLLEXPORT
#endif

#ifdef _WIN32
#include "windows.h"
#endif

int return_status = 0;

extern std::unique_ptr<DIBuilder> DBuilder;
extern std::vector<std::string> PackagesAdded;
extern bool is_template_parsing_definition;
extern bool is_template_parsing_struct;
struct DebugInfo CpointDebugInfo;
std::unique_ptr<Compiler_context> Comp_context;
string std_path = DEFAULT_STD_PATH;
string filename = "";
/*bool std_mode = true;
bool gc_mode = true;*/
extern std::string IdentifierStr;
//bool debug_mode = false;
bool debug_info_mode = false;
//bool test_mode = false;
bool silent_mode = false;

bool link_files_mode = true;

//bool is_release_mode = false;

bool errors_found = false;
int error_count = 0;

std::unordered_map<std::string, int> BinopPrecedence;
extern std::unique_ptr<Module> TheModule;

std::string first_filename = "";

extern int CurTok;
extern std::unordered_map<std::string, std::unique_ptr<PrototypeAST>> FunctionProtos;
//std::unique_ptr<llvm::raw_fd_ostream> file_out_ostream;
llvm::raw_ostream* file_out_ostream;
ifstream file_in;
ofstream file_log;
bool last_line = false;

extern int modifier_for_line_count;

extern std::vector<std::string> modulesNamesContext;

extern Source_location emptyLoc;

extern bool is_in_extern;

void add_manually_extern(std::string fnName, Cpoint_Type cpoint_type, std::vector<std::pair<std::string, Cpoint_Type>> ArgNames, unsigned Kind, unsigned BinaryPrecedence, bool is_variable_number_args, bool has_template, std::string TemplateName){
  if (FunctionProtos[fnName]){
    return;
  }
  is_in_extern = true;
  auto FnAST =  std::make_unique<PrototypeAST>(emptyLoc, fnName, std::move(ArgNames), cpoint_type, Kind != 0, BinaryPrecedence, is_variable_number_args);
  FunctionProtos[fnName] = FnAST->clone();
  Log::Info() << "add extern name " << fnName << "\n";
  FnAST->codegen();
  is_in_extern = false;
}

string TargetTriple;

Triple TripleLLVM;

void add_externs_for_gc(){
  std::vector<std::pair<std::string, Cpoint_Type>> args_gc_init;
  add_manually_extern("gc_init", Cpoint_Type(void_type), std::move(args_gc_init), 0, 30, false, false, "");
  std::vector<std::pair<std::string, Cpoint_Type>> args_gc_malloc;
  args_gc_malloc.push_back(make_pair("size", Cpoint_Type(i64_type)));
  add_manually_extern("gc_malloc", Cpoint_Type(void_type, true), std::move(args_gc_malloc), 0, 30, false, false, "");
  std::vector<std::pair<std::string, Cpoint_Type>> args_gc_realloc;
  args_gc_realloc.push_back(make_pair("ptr", Cpoint_Type(void_type, true)));
  args_gc_realloc.push_back(make_pair("size", Cpoint_Type(i64_type)));
  add_manually_extern("gc_realloc", Cpoint_Type(void_type, true), std::move(args_gc_realloc), 0, 30, false, false, "");
}

void add_externs_for_test(){
  std::vector<std::pair<std::string, Cpoint_Type>> args_printf;
  args_printf.push_back(make_pair("format", Cpoint_Type(i8_type, true)));
  add_manually_extern("printf", Cpoint_Type(int_type), std::move(args_printf), 0, 30, true, false, "");
}

void print_help(){
    std::cout << "Usage : cpoint [options] file" << std::endl;
    std::cout << "Options : " << std::endl;
    std::cout << "  -std : Select the path where is the std which will be builded" << std::endl;
    std::cout << "  -no-std : Make the compiler to not link to the std. It is the equivalent of -freestanding in gcc" << std::endl;
    std::cout << "  -c : Create an object file instead of an executable" << std::endl;
    std::cout << "  -target-triplet : Select the target triplet to select the target to compile to" << std::endl;
    std::cout << "  -verbose-std-build : Make the build of the standard library verbose. It is advised to activate this if the std doesn't build" << std::endl;
    std::cout << "  -no-delete-import-file : " << std::endl;
    std::cout << "  -no-gc : Make the compiler not add functions for garbage collection" << std::endl;
    std::cout << "  -with-gc : Activate the garbage collector explicitally (it is by default activated)" << std::endl;
    std::cout << "  -no-imports : Deactivate imports in the compiler" << std::endl;
    std::cout << "  -rebuild-gc : Force rebuilding the garbage collector" << std::endl;
    std::cout << "  -no-rebuild-std : Make the compiler not rebuild the standard library. You probably only need to rebuild it when you change the target" << std::endl;
    std::cout << "  -linker-flags=[flags] : Select additional linker flags" << std::endl;
    std::cout << "  -d : Activate debug mode to see debug logs" << std::endl;
    std::cout << "  -o : Select the output file name" << std::endl;
    std::cout << "  -g : Enable debuginfos" << std::endl;
    std::cout << "  -silent : Make the compiler silent" << std::endl;
    std::cout << "  -build-mode : Select the build mode (release or debug)" << std::endl;
    std::cout << "  -fPIC : Make the compiler produce position-independent code" << std::endl;
    std::cout << "  -compile-time-sizeof : The compiler gets the size of types at compile time" << std::endl;
    std::cout << "  -test : Compile tests" << std::endl;
    std::cout << "  -run-test : Run tests" << std::endl;
    std::cout << "others : TODO" << std::endl;
}


// TODO : move this in operators.cpp ?
void installPrecendenceOperators(){
    // Install standard binary operators.
    // 1 is lowest precedence.
    BinopPrecedence["="] = 5;

    BinopPrecedence["||"] = 10;
    BinopPrecedence["&&"] = 11;
    BinopPrecedence["|"] = 12;
    BinopPrecedence["^"] = 13;
    BinopPrecedence["&"] = 14;

    BinopPrecedence["!="] = 15;
    BinopPrecedence["=="] = 15;

    BinopPrecedence["<"] = 16;
    BinopPrecedence["<="] = 16;
    BinopPrecedence[">"] = 16;
    BinopPrecedence[">="] = 16;

    BinopPrecedence["<<"] = 20;
    BinopPrecedence[">>"] = 20;

    BinopPrecedence["+"] = 25;
    BinopPrecedence["-"] = 25;

    BinopPrecedence["*"] = 30;
    BinopPrecedence["%"] = 30;
    BinopPrecedence["/"] = 30;

    BinopPrecedence["["] = 35;

    BinopPrecedence["."] = 35;

    BinopPrecedence["("] = 35;
    BinopPrecedence["~"] = 35;

}

static void HandleDefinition() {
  if (auto FnAST = ParseDefinition()) {
    if (Comp_context->c_translator){
        FnAST->c_codegen();
    } else {
	if (/*auto *FnIR =*/ FnAST->codegen()) {
    Log::Info() << "Parsed a function definition." << "\n";
    //FnIR->print(*file_out_ostream);
    }
    }
  } else {
    if (!is_template_parsing_definition){
    // Skip token for error recovery.
    getNextToken();
    }
  }
}

static void HandleExtern() {
  is_in_extern = true;
  if (auto ProtoAST = ParseExtern()) {
    if (Comp_context->c_translator){
        ProtoAST->c_codegen();
    } else {
    if (/*auto *FnIR =*/ ProtoAST->codegen()) {
      Log::Info() << "Read Extern" << "\n";
      //FnIR->print(*file_out_ostream);
    }
    }
  } else {
    // Skip token for error recovery.
    getNextToken();
  }
  is_in_extern = false;
}

static void HandleTypeDef(){
  if (auto typeDefAST = ParseTypeDef()){
    typeDefAST->codegen();
  } else {
    getNextToken();
  }
}

static void HandleMod(){
  if (auto modAST = ParseMod()){
    modAST->codegen();
  }
}

static void HandleGlobalVariable(){
  if (auto GlobalVarAST = ParseGlobalVariable()){
    GlobalVarAST->codegen();
  } else {
    getNextToken();
  }

}

static void HandleStruct() {
  if (auto structAST = ParseStruct()){
    if (/*auto* structIR =*/ structAST->codegen()){
      //ObjIR->print(*file_out_ostream);
    }
  } else {
    if (!is_template_parsing_struct){
        getNextToken();
    }
  }
}

static void HandleUnion(){
  if (auto unionAST = ParseUnion()){
    unionAST->codegen();
  } else {
    getNextToken();
  }
}

static void HandleEnum(){
    if (auto enumAST = ParseEnum()){
        enumAST->codegen();
    } else {
        getNextToken();
    }
}

static void HandleMembers(){
    if (auto memberAST = ParseMembers()){
        memberAST->codegen();
    } else {
        getNextToken();
    }
}

void HandleComment(){
  Log::Info() << "token bef : " << CurTok << "\n";
  getNextToken(); // pass tok_single_line_comment token
  Log::Info() << "go to next line : " << CurTok << "\n";
  go_to_next_line();
  getNextToken();
  //handlePreprocessor();
  Log::Info() << "token : " << CurTok << "\n";
}

void HandleTest(){
  if (auto testAST = ParseTest()){
      testAST->codegen();
  } else {
    getNextToken();
  }
}

void MainLoop(){
  while (1) {
    if (Comp_context->debug_mode){
    std::flush(file_log);
    }
    if (last_line == true){
      if (Comp_context->debug_mode){
      file_log << "exit" << "\n";
      }
      return;
    }
    switch (CurTok) {
    case tok_eof:
      last_line = true; 
      return;
    case '#':
        // found call macro
        break;
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
    case tok_union:
      HandleUnion();
      break;
    case tok_enum:
      HandleEnum();
      break;
    case tok_class:
      //HandleClass();
      HandleStruct();
      break;
    case tok_var:
      HandleGlobalVariable();
      break;
    case tok_typedef:
      HandleTypeDef();
      break;
    case tok_mod:
      HandleMod();
      break;
    case tok_members:
        HandleMembers();
        break;
    case tok_single_line_comment:
      Log::Info() << "found single-line comment" << "\n";
      Log::Info() << "char found as a '/' : " << CurTok << "\n";
      if (Comp_context->debug_mode){
      file_log << "found single-line comment" << "\n";
      }
      HandleComment();
      break;
    default:
      bool is_in_module_context = CurTok == '}' && !modulesNamesContext.empty();
      if (is_in_module_context){
        modulesNamesContext.pop_back();
        getNextToken();
      } else {
      if (CurTok == tok_identifier && IdentifierStr == "test"){
        HandleTest();
        break;
      }
#if ENABLE_JIT
    if (Comp_context->jit_mode){
      if (CurTok != '\0'){
        HandleTopLevelExpression();
      } else {
        getNextToken();
      }
      break;
    }
#endif
      Log::Info() << "CurTok : " << CurTok << "\n";
      Log::Info() << "identifier : " << IdentifierStr << "\n";
      LogError("TOP LEVEL EXPRESSION FORBIDDEN");
      getNextToken();
      }
      break;
    }
  }
}


#if ENABLE_JIT
int StartJIT(){
    Comp_context->jit_mode = true;
    launchJIT();
    return 0;
}
#endif

int main(int argc, char **argv){
    setlocale(LC_ALL, "");
    bindtextdomain("cpoint", /*"/usr/share/locale/"*/ /*"./locales/"*/ NULL);
    textdomain("cpoint");
    Comp_context = std::make_unique<Compiler_context>("", 1, 0, "<empty line>");
    string object_filename = "out.o";
    string exe_filename = "a.out";
    string temp_output = "";
    bool output_temp_found = false;
    string target_triplet_found;
    bool target_triplet_found_bool = false;
    bool verbose_std_build = false;
    bool remove_temp_file = true;
    bool import_mode = true;
    bool rebuild_gc = false;
    bool rebuild_std = true;
    bool asm_mode = false;
    bool run_mode = false;
    bool is_optimised = false;
    bool explicit_with_gc = false; // add gc even with -no-std
    bool PICmode = false;
    // use native target even if -target is used. Needed for now in windows
    bool use_native_target = false;
    std::string linker_additional_flags = "";
    if (argc < 2){
#if ENABLE_JIT
        return StartJIT();
        /*Comp_context->jit_mode = true;
        launchJIT();
        return 0;*/
#else
        fprintf(stderr, "not enough arguments, expected at least 1, got %d\n", argc-1);
        exit(1);
#endif
    }
    bool filename_found = false;
    for (int i = 1; i < argc; i++){
        string arg = argv[i];
        if (arg.compare("-d") == 0){
            cout << "debug mode" << endl;
            Comp_context->debug_mode = true;
        } else if (arg.compare("-o") == 0){
          i++;
          temp_output = argv[i];
          Log::Info() << "object_filename " << object_filename << "\n";
          output_temp_found = true;
        }  else if (arg.compare("-silent") == 0){
          silent_mode = true;
        } else if (arg.compare("-std") == 0){
          i++;
          std_path = argv[i];
          Log::Print() << _("custom path std : ") << std_path << "\n";
        } else if (arg.compare("-c") == 0){
          link_files_mode = false;
        } else if (arg.compare("-S") == 0){
          asm_mode = true;
        } else if (arg.compare("-O") == 0){
          is_optimised = true;
        } else if (arg.compare("-g") == 0){
          debug_info_mode = true;
        } else if (arg.compare("-h") == 0 || arg.compare("-help") == 0){
            print_help();
            exit(0);
        } else if (arg.compare("-no-std") == 0){
          Comp_context->std_mode = false;
        } else if (arg.compare("-fPIC") == 0){
          PICmode = true;
        } else if(arg.compare("-target-triplet") == 0){
          target_triplet_found_bool = true;
          i++;
          target_triplet_found = argv[i];
          cout << "target triplet : " << target_triplet_found << endl;
        } else if (arg.compare("-build-mode") == 0){
            i++;
            if ((std::string)argv[i] == "release"){
                Comp_context->is_release_mode = true;
            } else if ((std::string)argv[i] == "debug"){
                Comp_context->is_release_mode = false;
            } else {
                Log::Warning(emptyLoc) << "Unkown build mode, defaults to debug mode" << "\n";
                Comp_context->is_release_mode = false;
            }
        } else if (arg.compare("-verbose-std-build") == 0){
          verbose_std_build = true;
        } else if (arg.compare("-no-delete-import-file") == 0){
          remove_temp_file = false;
        } else if (arg.compare("-no-gc") == 0){
          Comp_context->gc_mode = false;
        } else if (arg.compare("-with-gc") == 0){
          explicit_with_gc = true;
        } else if (arg.compare("-no-imports") == 0){
          import_mode = false;
        } else if (arg.compare("-rebuild-gc") == 0) {
	        rebuild_gc = true;
        } else if (arg.compare("-no-rebuild-std") == 0){
          rebuild_std = false;
        } else if (arg.compare("-compile-time-sizeof") == 0){
            Comp_context->compile_time_sizeof = true;
        } else if (arg.compare("-test") == 0){
          Comp_context->test_mode = true;
        } else if (arg.compare("-c-translator") == 0){
            Comp_context->c_translator = true;
        } else if (arg.compare("-use-native-target") == 0){
            use_native_target = true;
        } else if (arg.compare("-run") == 0){
          run_mode = true;
        } else if (arg.compare("-run-test") == 0){
          run_mode = true;
          Comp_context->test_mode = true;
        } else if (arg.compare("-strip") == 0){
          Comp_context->strip_mode = true;
        } else if (arg.compare("-flto") == 0){
          Comp_context->lto_mode = true;
        } else if (arg.compare(0, 14,  "-linker-flags=") == 0){
          size_t pos = arg.find('=');
          linker_additional_flags += arg.substr(pos+1, arg.size());
          linker_additional_flags += ' ';
          cout << "linker flag " << linker_additional_flags << endl; 
        } else {
          filename_found = true;
          Log::Print() << _("filename : ") << arg << "\n";
          filename = arg;
        }
    }
    if (!filename_found){
#if ENABLE_JIT
        return StartJIT();
#else
        fprintf(stderr, "didn't find a filename in args\n");
        exit(1);
#endif
    }
    Log::Info() << "filename at end : " << filename << "\n";
    if (!filesystem::exists(filename)){
        fprintf(stderr, RED "File %s doesn't exist\n" CRESET, filename.c_str());
        exit(1);
    }
    first_filename = filename;
    if (output_temp_found){
      if (link_files_mode){
        exe_filename = temp_output;
      } else {
        object_filename = temp_output;
      }
    }
    if (asm_mode){
      link_files_mode = false;
      if (object_filename == "out.o"){
        object_filename = "out.S";
      }
    }
    init_context_preprocessor();
    Comp_context->filename = filename;
    std::string temp_filename = filename;
    temp_filename.append(".temp");
    if (import_mode){
    int nb_imports = 0;
    nb_imports = generate_file_with_imports(filename, temp_filename);
    Log::Info() << "before remove line count because of import mode " << Comp_context->lexloc.line_nb << " lines" << "\n";
    Log::Info() << "lines count to remove because of import mode " << modifier_for_line_count << "\n";
    Comp_context->lexloc.line_nb -= modifier_for_line_count+nb_imports /*+1*/ /**2*/; // don't know why there are 2 times too much increment so we need to make the modifier double
    Comp_context->curloc.line_nb -= modifier_for_line_count;
    Log::Info() << "after remove line count because of import mode " << Comp_context->lexloc.line_nb << " lines" << "\n";
    filename = temp_filename;
    }
    std::error_code ec;
    if (Comp_context->debug_mode == false){
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
    
    c_translator::init_context();
    
    installPrecendenceOperators();

    legacy::PassManager pass;
    if (target_triplet_found_bool){
    TargetTriple = target_triplet_found;
    } else {
    TargetTriple = sys::getDefaultTargetTriple();
    }
    std::string os_name = get_os(TargetTriple);
    TripleLLVM = Triple(TargetTriple);
    Log::Info() << "os from target triplet : " << os_name << "\n";
    setup_preprocessor(TargetTriple);
    Log::Info() << "TEST AFTER PREPROCESSOR" << "\n";
    getNextToken();
    InitializeModule(first_filename);
    if (is_optimised){
    pass.add(createInstructionCombiningPass());
    pass.add(createReassociatePass());
    pass.add(createGVNPass());
    pass.add(createCFGSimplificationPass());
    }
    if (debug_info_mode){
    TheModule->addModuleFlag(Module::Warning, "Debug Info Version",
                           DEBUG_METADATA_VERSION);
    if (Triple(sys::getProcessTriple()).isOSDarwin()){
      TheModule->addModuleFlag(llvm::Module::Warning, "Dwarf Version", 2);
    }
    DBuilder = std::make_unique<DIBuilder>((*TheModule));
    CpointDebugInfo.TheCU = DBuilder->createCompileUnit(
      dwarf::DW_LANG_C, DBuilder->createFile(first_filename, "."),
      "Cpoint Compiler", is_optimised, "", 0);
    }
    if ((Comp_context->std_mode || explicit_with_gc) && Comp_context->gc_mode){
      add_externs_for_gc();
    }
    if (Comp_context->test_mode){
      add_externs_for_test();
      if (Comp_context->std_mode){
        add_test_imports();
      }
    }
    if (Comp_context->test_mode && !Comp_context->std_mode){
        Log::Warning() << "Using tests without the standard library. You will not be able to use expect and you will need to have linked a libc" << "\n";
    }
    MainLoop();
    codegenTemplates();
    //codegenStructTemplates();
    generateTests();
    generateClosures();
    //generateExterns();
    if (debug_info_mode){
    DBuilder->finalize();
    }
    TheModule->print(*file_out_ostream, nullptr);
    file_in.close();
    file_log.close();
    if (errors_found){
      fprintf(stderr, _(RED "exited with %d errors\n" CRESET), error_count);
      exit(1);
    }

    if (Comp_context->c_translator){
        c_translator::generate_c_code("out.c");
    } else {
    InitializeAllTargetInfos();
    InitializeAllTargets();
    InitializeAllTargetMCs();
    std::string Error;
    auto Target = TargetRegistry::lookupTarget(TargetTriple, Error);
    if (!Target) {
        errs() << Error << "\n";
        return 1;
    }

    auto CPU = "generic";
    auto Features = "";
    TargetOptions opt;
    //llvm::Optional<llvm::Reloc::Model> RM;
    std::optional<llvm::Reloc::Model> RM;
    if (PICmode){
      //RM = Optional<Reloc::Model>(Reloc::Model::PIC_);
      RM = std::optional<Reloc::Model>(Reloc::Model::PIC_);
    } else {
      //RM = Optional<Reloc::Model>(Reloc::Model::DynamicNoPIC);
      RM = std::optional<Reloc::Model>(Reloc::Model::DynamicNoPIC);
    }
    auto TheTargetMachine = Target->createTargetMachine(TargetTriple, CPU, Features, opt, RM);
    TheModule->setDataLayout(TheTargetMachine->createDataLayout());
    if (Comp_context->lto_mode){
        writeBitcodeLTO(object_filename, false);
    } else {
    //InitializeAllTargetInfos();
    //InitializeAllTargets();
    //InitializeAllTargetMCs();
    InitializeAllAsmParsers();
    InitializeAllAsmPrinters();
    TheModule->setTargetTriple(TargetTriple);
    /*std::string Error;
    auto Target = TargetRegistry::lookupTarget(TargetTriple, Error);
    if (!Target) {
        errs() << Error;
        return 1;
    }

    auto CPU = "generic";
    auto Features = "";
    TargetOptions opt;
    //llvm::Optional<llvm::Reloc::Model> RM;
    std::optional<llvm::Reloc::Model> RM;
    if (PICmode){
      //RM = Optional<Reloc::Model>(Reloc::Model::PIC_);
      RM = std::optional<Reloc::Model>(Reloc::Model::PIC_);
    } else {
      //RM = Optional<Reloc::Model>(Reloc::Model::DynamicNoPIC);
      RM = std::optional<Reloc::Model>(Reloc::Model::DynamicNoPIC);
    }
    auto TheTargetMachine = Target->createTargetMachine(TargetTriple, CPU, Features, opt, RM);
    TheModule->setDataLayout(TheTargetMachine->createDataLayout());*/
    std::error_code EC;
    raw_fd_ostream dest(llvm::StringRef(object_filename), EC, sys::fs::OF_None);
    if (EC) {
        errs() << _("Could not open file: ") << EC.message();
        return 1;
    }
    llvm::CodeGenFileType FileType = CGFT_ObjectFile;
    if (asm_mode){
      FileType = CGFT_AssemblyFile;
    }
    if (TheTargetMachine->addPassesToEmitFile(pass, dest, nullptr, FileType)) {
    errs() << _("TheTargetMachine can't emit a file of this type");
    return 1;
    }
    pass.run(*TheModule);
    dest.flush();
    delete TheTargetMachine; // call the TargetMachine destructor to not leak memory
    }
    std::string gc_path = DEFAULT_GC_PATH;
    Log::Print() << _("Wrote ") << object_filename << "\n";
    std::string std_static_path = std_path;
    if (asm_mode){
        goto after_linking;
    }
    if (std_path.back() != '/'){
      std_static_path.append("/");
    }
    std_static_path.append("libstd.a");
    if (Comp_context->std_mode && link_files_mode){
      if (rebuild_std){
      if (build_std(std_path, TargetTriple, verbose_std_build, use_native_target, Comp_context->gc_mode) == -1){
        fprintf(stderr, "Could not build std at path : %s\n", std_path.c_str());
        exit(1);
      }
      Log::Print() << _("Built the standard library") << "\n";
      } else {
        if (!FileExists(std_static_path)){
          fprintf(stderr, "std static library %s has not be builded. You need to at least compile a file one time without the -no-rebuild-std flag\n", std_static_path.c_str());
        }
      }
      if (Comp_context->gc_mode == true){
      gc_path = std_path;
      if (rebuild_gc){
      gc_path.append("/../bdwgc");
      Log::Print() << "Build the garbage collector" << "\n";
      build_gc(gc_path, TargetTriple);
      }
      }
    }
    if (link_files_mode){
      std::vector<string> vect_obj_files;
      if (TargetTriple.find("wasm") == std::string::npos){
      std::string gc_static_path = gc_path;
      if (gc_path.back() != '/'){
      gc_static_path.append("/");
      }
      gc_static_path.append("../bdwgc_prefix/lib/libgc.a");
      /*std::string std_static_path = "-L";
      std_static_path.append(std_path);
      std_static_path.append(" -lstd");*/
      vect_obj_files.push_back(object_filename);
      if (Comp_context->std_mode){
      vect_obj_files.push_back(std_static_path);
      if (Comp_context->gc_mode){
      vect_obj_files.push_back(gc_static_path);
      }
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
      if (Comp_context->lto_mode){
        linker_additional_flags += " -flto ";
      }
      link_files(vect_obj_files, exe_filename, TargetTriple, linker_additional_flags);
      Log::Print() << _("Linked the executable") << "\n";
    }
    if (Comp_context->strip_mode){
        if (!link_files_mode){
            fprintf(stderr, "Can't strip without linking so it is ignored\n");
            exit(1);
        }
        std::string cmd = "llvm-strip " + exe_filename;
        runCommand(cmd);
    }
    if (run_mode){
      if (!link_files_mode){
        fprintf(stderr, "Can't run without linking so it is ignored\n");
        exit(1);
      }
      char actualpath[PATH_MAX+1];
      realpath(exe_filename.c_str(), actualpath);
      Log::Print() << "run executable at " << actualpath << "\n";
      runCommand(actualpath);
    }

    }
after_linking:
    if (remove_temp_file /*&& !asm_mode*/){
      remove(temp_filename.c_str());
    }
    return return_status;
}
