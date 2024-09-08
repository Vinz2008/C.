#include <iostream>
#include <fstream>
#include <unordered_map>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <locale.h>
#include "llvm/Config/llvm-config.h"
#if LLVM_VERSION_MAJOR <= 17
#include "llvm/Support/Host.h"
#else 
#include "llvm/TargetParser/Host.h"
#endif
#include "config.h"
#include "lexer.h"
#include "ast.h"
#include "codegen.h"
#include "preprocessor.h"
#include "errors.h"
#include "debuginfo.h"
#include "linker.h"
#include "targets/target-triplet.h"
#include "cli.h"
#include "imports.h"
#include "log.h"
#include "utils.h"
#include "color.h"
#include "gettext.h"
#include "c_translator.h"
#include "templates.h"
#include "tests.h"
#include "abi.h"
#include "jit.h"
#include "operators.h"
#include "cli_infos.h"
#include "llvm.h"
#include "targets/targets.h"

#ifdef ENABLE_LLVM_TOOLS_EMBEDDED_COMPILER
#include "clang.h"
#include "ar.h"
#include "lld.h"
#endif

#if ENABLE_CIR
#include "CIR/cir.h"
#include "backends/backends.h"
#endif

using namespace std;

/*using namespace llvm;
using namespace llvm::sys;*/

#ifdef _WIN32
#define DLLEXPORT __declspec(dllexport)
#else
#define DLLEXPORT
#endif

#ifdef _WIN32
#include "windows.h"
#endif

#ifndef _WIN32
#define STDOUT_PATH "/dev/stdout"
#else
#define STDOUT_PATH ""
#endif

int return_status = 0;

extern std::unique_ptr<DIBuilder> DBuilder;
extern std::vector<std::string> PackagesAdded;
extern bool is_template_parsing_definition;
extern bool is_template_parsing_struct;
extern bool is_template_parsing_enum;
struct DebugInfo CpointDebugInfo;

// TODO : replace the unique_ptr by just the class
std::unique_ptr<Compiler_context> Comp_context;
string std_path = DEFAULT_STD_PATH;
string filename = "";
extern std::string IdentifierStr;
// TODO : move debug_info_mode, silent_mode, etc to Comp_Context
bool debug_info_mode = false;
bool silent_mode = false;

bool link_files_mode = true;

bool errors_found = false;
int error_count = 0;

std::unordered_map<std::string, int> BinopPrecedence;
extern std::unique_ptr<Module> TheModule;

std::string first_filename = "";

// TODO : add this to Comp_Context
extern int CurTok;
extern std::unordered_map<std::string, std::unique_ptr<PrototypeAST>> FunctionProtos;
llvm::raw_ostream* file_out_ostream;
ifstream file_in;
ofstream file_log;
bool last_line = false;

extern int modifier_for_line_count;

//extern std::vector<std::string> modulesNamesContext;

extern Source_location emptyLoc;

extern bool is_in_extern;

extern std::vector<std::string> types_list;

extern std::vector<Cpoint_Type> typeDefTable;


// TODO : refactor this code (move it in other files, for example move the generation by llvm of an object file in a separate function in another file)

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

//string TargetTriple;

TargetInfo targetInfos;

// put this in comp_context or target infos ?
Triple TripleLLVM;

static void add_externs_for_gc(){
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

static void add_externs_for_test(){
  std::vector<std::pair<std::string, Cpoint_Type>> args_printf;
  args_printf.push_back(make_pair("format", Cpoint_Type(i8_type, true)));
  add_manually_extern("printf", Cpoint_Type(i32_type), std::move(args_printf), 0, 30, true, false, "");
}

static void add_default_typedefs(){
  types_list.push_back("int");
  typeDefTable.push_back(Cpoint_Type(i32_type));
}

#if ENABLE_FILE_AST == 0

static void HandleDefinition() {
  if (auto FnAST = ParseDefinition()) {
    if (Comp_context->c_translator){
        FnAST->c_codegen();
    } else {
	if (FnAST->codegen()) {
    Log::Info() << "Parsed a function definition." << "\n";
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
    if (ProtoAST->codegen()) {
      Log::Info() << "Read Extern" << "\n";
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
    //typeDefAST->codegen(); // it is not needed
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
    structAST->codegen();
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
        if (!is_template_parsing_enum){
            getNextToken();
        }
    }
}

static void HandleMembers(){
    if (auto memberAST = ParseMembers()){
        memberAST->codegen();
    } else {
        getNextToken();
    }
}

#endif

void HandleComment(){
  Log::Info() << "token bef : " << CurTok << "\n";
  getNextToken(); // pass tok_single_line_comment token
  Log::Info() << "go to next line : " << CurTok << "\n";
  go_to_next_line();
  getNextToken();
  //handlePreprocessor();
  Log::Info() << "token : " << CurTok << "\n";
}

extern std::vector<std::unique_ptr<TestAST>> testASTNodes;;

void HandleTest(){
  if (auto testAST = ParseTest()){
    testASTNodes.push_back(std::move(testAST));
    //testAST->codegen();
  } else {
    getNextToken();
  }
}

#if ENABLE_FILE_AST == 0

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
      //bool is_in_module_context = CurTok == '}' && !modulesNamesContext.empty();
      /*if (is_in_module_context){
        modulesNamesContext.pop_back();
        getNextToken();
      } else {*/
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
      LogErrorE("TOP LEVEL EXPRESSION FORBIDDEN");
      getNextToken();
      //}
      break;
    }
  }
}

#endif


#if ENABLE_JIT
int StartJIT(){
    Comp_context->jit_mode = true;
    launchJIT();
    return 0;
}
#endif

int main(int argc, char **argv){
#ifndef _WIN32 // TODO : make work gettext on windows (removed also because of problems with mingw gettext in the docker ci)
    setlocale(LC_ALL, "");
    bindtextdomain("cpoint", "/usr/share/locale/" /*"./locales/"*/ /*NULL*/);
    textdomain("cpoint");
#endif
    Comp_context = std::make_unique<Compiler_context>("", 1, 0, "<empty line>");
    add_default_typedefs();
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
    int optimize_level = 0;
    bool explicit_with_gc = false; // add gc even with -no-std
    bool PICmode = false;
    bool time_report = false;
    // use native target even if -target is used. Needed for now in windows
    bool use_native_target = false;
    bool thread_sanitizer = false;
    bool only_preprocess = false;
    bool should_use_internal_lld = true; // TODO : set this by default to true
    std::string linker_additional_flags = "";
    std::string run_args = "";
    std::string llvm_default_target_triple = llvm::sys::getDefaultTargetTriple();
    TripleLLVM = Triple(llvm_default_target_triple); // default target triplet only for lld
    std::string TargetTriple = "";
    enum backend_type backend = backend_type::LLVM_TYPE; // add a cli flag to set this
    if (argc < 2){
#if ENABLE_JIT
        return StartJIT();
#else
        fprintf(stderr, "not enough arguments, expected at least 1, got %d\n", argc-1);
        exit(1);
#endif
    }
    bool ignore_invalid_flags = false;
#ifdef ENABLE_LLVM_TOOLS_EMBEDDED_COMPILER
    std::string lld_target_triplet = llvm_default_target_triple;
    for (int i = 1; i < argc; i++){
        std::string arg_temp = (std::string)argv[i];
        if (arg_temp.compare("lld") == 0){
            ignore_invalid_flags = true;
        } else if (arg_temp.compare("-target-triplet") == 0){
            lld_target_triplet = argv[i+1];
            i++;
            Log::Info() << "lld_target_triplet : " << lld_target_triplet << "\n"; 
        }
    }
#endif
    bool filename_found = false;
    // TODO : put all of this in a parse_args function where a compiler context is passed and move it to a args.cpp file
    for (int i = 1; i < argc; i++){
        string arg = argv[i];
#ifdef ENABLE_LLVM_TOOLS_EMBEDDED_COMPILER
        if (arg.compare("cc") == 0 || arg.compare("c++") == 0 || arg.compare("-cc1") == 0 || arg.compare("-cc1as") == 0){
            //printf("argc : %d\n", argc);
            std::vector<char*> clang_args;
            std::vector<std::string> args_vector(argv, argv+argc);
            if (std::find(args_vector.begin(), args_vector.end(), "-cc1") != args_vector.end() || std::find(args_vector.begin(), args_vector.end(), "-cc1ass") != args_vector.end()){
                Log::Info() << "call_from_clang_driver" << "\n";
                //call_from_clang_driver = true;
                //i += 2;
            }
            clang_args.push_back((char*)/*"cc"*/ arg.c_str());
            while (i < argc){
                clang_args.push_back(argv[i]);
                //printf("CLANG ARG added : %s\n", clang_args.back());
                i++;    
            }
            clang_args.push_back(nullptr);
            exit(launch_clang(clang_args.size()-1, clang_args.data()));
        } else if (arg.compare("ar") == 0 || arg.compare("ranlib") == 0 || arg.compare("lib") == 0){
            std::vector<char*> ar_args;
            while (i < argc){
                ar_args.push_back(argv[i]);
                i++;    
            }
            ar_args.push_back(nullptr);
            return launch_ar(ar_args.size()-1, ar_args.data());
        } else if (arg.compare("lld") == 0 || arg.compare("ld.lld") == 0 || arg.compare("lld-link") == 0 || arg.compare("wasm-ld") == 0){
            std::vector<const char*> lld_args;
            i = 1;
            while (i < argc){
                std::string arg_temp = argv[i];
                if (arg_temp.compare("-target-triplet") == 0){
                    i++; // ignore the -target-triplet information for setting the lld flavor
                } else if (arg_temp.compare("lld") != 0 && arg_temp.compare("ld.lld") != 0 && arg_temp.compare("lld-link") != 0 && arg_temp.compare("wasm-ld") != 0){
                    lld_args.push_back((const char*)argv[i]);
                }
                i++;    
            }
            lld_args.push_back(nullptr);
            Log::Info() << "args : ";
            for (int i = 0; i < lld_args.size()-1; i++){
                Log::Info() << lld_args.at(i) << " ";
            }
            Log::Info() << "\n";
            return LLDLink(llvm::Triple(lld_target_triplet), lld_args.size()-1, lld_args.data(), false /*can_exit_early : should it ? (TODO ?)*/, silent_mode);
        } else if (arg.compare("-internal-lld") == 0){
            should_use_internal_lld = true;
        } else if (arg.compare("-disable-internal-lld") == 0) {
            should_use_internal_lld = false;
        } else
#endif
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
        } /*else if (arg.compare("-O") == 0){
          // TODO : change from -O 1 to -O1
          is_optimised = true;
          i++;
          optimize_level = std::stoi((std::string)argv[i]);
        }*/ else if (arg.compare("-g") == 0){
          debug_info_mode = true;
        } else if (arg.compare("-h") == 0 || arg.compare("--help") == 0){
            print_help();
            return 0;
        } else if (arg.compare("-v") == 0 || arg.compare("--version") == 0){
            print_version();
            return 0;
        } else if (arg.compare("--infos") == 0){
            print_infos(llvm_default_target_triple);
            return 0;
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
                Log::Warning(emptyLoc) << "Unknown build mode, defaults to debug mode" << "\n";
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
        } else if(arg.compare("-time-trace") == 0){
          time_report = true;
        } else if (arg.compare("-thread-sanitizer") == 0){
          thread_sanitizer = true;
        } else if (arg.compare("-E") == 0){
          only_preprocess = true;
        } else if (arg.compare(0, 2, "-O") == 0){
          size_t pos = arg.find("O");
          std::string temp = arg.substr(pos+1, arg.size());
          is_optimised = true;
          //i++;
          optimize_level = std::stoi(temp);
        } else if (arg.compare(0, 14,  "-linker-flags=") == 0){
          size_t pos = arg.find('=');
          linker_additional_flags += arg.substr(pos+1, arg.size());
          linker_additional_flags += ' ';
          //cout << "linker flag " << linker_additional_flags << endl; 
        } else if (arg.compare(0, 10, "-run-args=") == 0){
          size_t pos = arg.find('=');
          run_args += arg.substr(pos+1, arg.size());
          run_args += ' ';
        } else if (arg.at(0) == '-'){
            if (!ignore_invalid_flags){
                fprintf(stderr, "Unknown flag : %s\n", arg.c_str());
                return 1;
            }
        } else if (!ignore_invalid_flags) {
          filename_found = true;
          Log::Print() << _("filename : ") << arg << "\n";
          filename = arg;
        }
    }
    operators::installPrecendenceOperators();
    if (!filename_found){
#if ENABLE_JIT
        return StartJIT();
#else
        fprintf(stderr, "didn't find a filename in args\n");
        return 1;
#endif
    }
    Log::Info() << "filename at end : " << filename << "\n";
    if (!filesystem::exists(filename)){
        fprintf(stderr, RED "File %s doesn't exist\n" CRESET, filename.c_str());
        return 1;
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
    if (target_triplet_found_bool){
    TargetTriple = target_triplet_found;
    } else {
    TargetTriple = llvm_default_target_triple;
    } 
    setup_preprocessor(TargetTriple);
    targetInfos = get_target_infos(TargetTriple);
    Comp_context->filename = filename;
    std::string temp_filename = filename;
    temp_filename.append(".temp");
    if (only_preprocess){
        temp_filename = STDOUT_PATH;
        generate_file_with_imports(filename, temp_filename);
        return 0;
    }
    if (import_mode){
    int nb_imports = 0;
    nb_imports = generate_file_with_imports(filename, temp_filename);
    Log::Info() << "before remove line count because of import mode " << Comp_context->lexloc.line_nb << " lines" << "\n";
    Log::Info() << "lines count to remove because of import mode " << modifier_for_line_count << "\n";
    Comp_context->lexloc.line_nb -= modifier_for_line_count-nb_imports /*+1*/ /**2*/; // don't know why there are 2 times too much increment so we need to make the modifier double
    Comp_context->curloc.line_nb -= modifier_for_line_count-nb_imports;
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
    if (Comp_context->c_translator){
        c_translator::init_context();
    }
    
    std::string os_name = get_os(TargetTriple);
    TripleLLVM = llvm::Triple(/*TargetTriple*/ targetInfos.llvm_target_triple);
    Log::Info() << "os from target triplet : " << os_name << "\n";
    Log::Info() << "TEST AFTER PREPROCESSOR" << "\n";
    getNextToken();
    InitializeModule(first_filename);

    if (debug_info_mode){
    TheModule->addModuleFlag(Module::Warning, "Debug Info Version",
                           DEBUG_METADATA_VERSION);
    if (/*Triple(sys::getProcessTriple())*/ TripleLLVM.isOSDarwin()){
      TheModule->addModuleFlag(llvm::Module::Warning, "Dwarf Version", 2);
    }
    DBuilder = std::make_unique<DIBuilder>((*TheModule));
    CpointDebugInfo = DebugInfo(get_pointer_size());
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
#if ENABLE_FILE_AST
  std::unique_ptr<FileAST> file_ast = ParseFile();
#if ENABLE_CIR
  auto file_cir = file_ast->cir_gen(first_filename);
  std::string cir_string = file_cir->to_string();
  ofstream cir_file("out.cir");
  cir_file << cir_string << "\n";
  cir_file.close();
  if (codegenBackend(std::move(file_cir), backend, object_filename, TripleLLVM, PICmode, asm_mode, time_report, is_optimised, thread_sanitizer, optimize_level, targetInfos.cpu, targetInfos.features) == 1){
    return 1;
  }
#endif
  file_ast->codegen();
#else
    MainLoop();
#endif
    codegenTemplates();
    //codegenStructTemplates();
    generateTests();
    generateClosures();
    //generateExterns();
    if (debug_info_mode){
    DBuilder->finalize();
    }
    file_in.close();
    file_log.close();
    if (errors_found){
      fprintf(stderr, _(RED "exited with %d errors\n" CRESET), error_count);
      exit(1);
    }

    if (Comp_context->c_translator){
        c_translator::generate_c_code("out.c");
    } else {
    int ret = generate_llvm_object_file(std::move(TheModule), object_filename, TripleLLVM, /*TargetTriple*/ targetInfos.llvm_target_triple, file_out_ostream, PICmode, asm_mode, time_report, is_optimised, thread_sanitizer, optimize_level, targetInfos.cpu, targetInfos.features);
    if (ret == 1){
        return 1;
    }
    std::string gc_path = DEFAULT_GC_PATH;
    Log::Print() << _("Wrote ") << object_filename << "\n";
    std::string std_static_path = std_path;
    /*if (asm_mode){
        goto after_linking;
    }*/
    if (!asm_mode){
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
      if (thread_sanitizer){
        linker_additional_flags += " -ltsan ";
      }
      link_files(vect_obj_files, exe_filename, TargetTriple, linker_additional_flags, should_use_internal_lld, (std::string)argv[0]);
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
      std::string cmd = actualpath;
      cmd += " " + run_args + " ";
      runCommand(cmd);
    }

    }
    }
//after_linking:
    if (remove_temp_file){
      remove(temp_filename.c_str());
    }
    return return_status;
}
