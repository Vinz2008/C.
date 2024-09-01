#include "llvm.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetOptions.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Transforms/InstCombine/InstCombine.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Scalar/GVN.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/StandardInstrumentations.h"
#include "llvm/Analysis/TargetTransformInfo.h"
#include "llvm/Analysis/AliasAnalysis.h"
#include "llvm/Support/Process.h"
#include <llvm/Transforms/Instrumentation/ThreadSanitizer.h>
#include "gettext.h"
#include "lto.h"
#include "errors.h"

using namespace llvm;
using namespace llvm::sys;

// TODO : move this in the backends/llvm.cpp file

// also stolen from zig
struct TimeTracerRAII {
  // Granularity in ms
  unsigned TimeTraceGranularity;
  StringRef TimeTraceFile, OutputFilename;
  bool EnableTimeTrace;

  TimeTracerRAII(StringRef ProgramName, StringRef OF, bool is_time_trace_on)
    : TimeTraceGranularity(500U),
      TimeTraceFile("trace.json"),
      OutputFilename(OF),
      EnableTimeTrace(!TimeTraceFile.empty() && is_time_trace_on) {
    if (EnableTimeTrace) {
      if (const char *G = std::getenv("CPOINT_TIME_TRACE_GRANULARITY"))
        TimeTraceGranularity = (unsigned)std::atoi(G);

      llvm::timeTraceProfilerInitialize(TimeTraceGranularity, ProgramName);
    }
  }

  ~TimeTracerRAII() {
    if (EnableTimeTrace) {
      if (auto E = llvm::timeTraceProfilerWrite(TimeTraceFile, OutputFilename)) {
        handleAllErrors(std::move(E), [&](const StringError &SE) {
          errs() << SE.getMessage() << "\n";
        });
        return;
      }
      timeTraceProfilerCleanup();
    }
  }
};

//extern std::unique_ptr<Module> TheModule;
extern std::unique_ptr<Compiler_context> Comp_context;

// TODO : just pass a TargetInfo insted of CPU, LLVMTargetTriple and cpu_features
// TODO : replace is_optimised and optimize level with an enum with level 0, 1, 2 and 3 
int generate_llvm_object_file(std::unique_ptr<Module> TheModule, std::string object_filename, Triple TripleLLVM, std::string LLVMTargetTriple, llvm::raw_ostream* file_out_ostream, bool PICmode, bool asm_mode, bool time_report, bool is_optimised, bool thread_sanitizer, int optimize_level, std::string CPU, std::string cpu_features){
    InitializeAllTargetInfos();
    InitializeAllTargets();
    InitializeAllTargetMCs();
    std::string Error;
    auto Target = TargetRegistry::lookupTarget(LLVMTargetTriple, Error);
    if (!Target) {
        errs() << Error << "\n";
        return 1;
    }

    //std::string CPU = "generic";
    //std::string Features = "";
    TargetOptions opt;
    std::optional<llvm::Reloc::Model> RM;
    if (PICmode){
      RM = std::optional<Reloc::Model>(Reloc::Model::PIC_);
    } else {
      RM = std::optional<Reloc::Model>(Reloc::Model::DynamicNoPIC);
    }
    TargetMachine* TheTargetMachine = Target->createTargetMachine(LLVMTargetTriple, CPU, /*Features*/ cpu_features, opt, RM);
    TheModule->setDataLayout(TheTargetMachine->createDataLayout());
    InitializeAllAsmParsers();
    InitializeAllAsmPrinters();
    TheModule->setTargetTriple(LLVMTargetTriple);
    std::error_code EC;
    raw_fd_ostream dest(llvm::StringRef(object_filename), EC, sys::fs::OF_None);
    if (EC) {
        errs() << _("Could not open file: ") << EC.message();
        return 1;
    }
    auto PID = sys::Process::getProcessId();
    std::string ProcName = "cpoint-";
    ProcName += std::to_string(PID);
    TimeTracerRAII TimeTracer(ProcName,
                              object_filename, time_report);

#if LLVM_VERSION_MAJOR >= 18 
  llvm::CodeGenFileType FileType = CodeGenFileType::ObjectFile;
  if (asm_mode){
    FileType = CodeGenFileType::AssemblyFile;
  }
#else
    llvm::CodeGenFileType FileType = CGFT_ObjectFile;
    if (asm_mode){
      FileType = CGFT_AssemblyFile;
    }
#endif    

    // this is stolen from zig
    // maybe try to remove some parts to see what is necessary (TODO ?)
    PipelineTuningOptions pipeline_opts;
    pipeline_opts.LoopUnrolling = is_optimised;
    pipeline_opts.SLPVectorization = is_optimised;
    pipeline_opts.LoopVectorization = is_optimised;
    pipeline_opts.LoopInterleaving = is_optimised;
    pipeline_opts.MergeFunctions = is_optimised;
    
    PassInstrumentationCallbacks instr_callbacks;
    StandardInstrumentations std_instrumentations(TheModule->getContext(), false);
    std_instrumentations.registerCallbacks(instr_callbacks);

    std::optional<PGOOptions> opt_pgo_options = {};

    PassBuilder pass_builder(TheTargetMachine, pipeline_opts,
                             opt_pgo_options, &instr_callbacks);
    LoopAnalysisManager loop_am;
    FunctionAnalysisManager function_am;
    CGSCCAnalysisManager cgscc_am;
    ModuleAnalysisManager module_am;

    function_am.registerPass([&] {
      return pass_builder.buildDefaultAAPipeline();
    });

    //Triple target_triple(TargetTriple);
    auto tlii = std::make_unique<TargetLibraryInfoImpl>(TripleLLVM);
    function_am.registerPass([&] { return TargetLibraryAnalysis(*tlii); });

    pass_builder.registerModuleAnalyses(module_am);
    pass_builder.registerCGSCCAnalyses(cgscc_am);
    pass_builder.registerFunctionAnalyses(function_am);
    pass_builder.registerLoopAnalyses(loop_am);
    pass_builder.crossRegisterProxies(loop_am, function_am,
                                      cgscc_am, module_am);


    if (thread_sanitizer){
      pass_builder.registerOptimizerLastEPCallback([](ModulePassManager &module_pm, OptimizationLevel level) {
            module_pm.addPass(ModuleThreadSanitizerPass());
            module_pm.addPass(createModuleToFunctionPassAdaptor(ThreadSanitizerPass()));
        });
    }
    ModulePassManager module_pm;
    OptimizationLevel opt_level;
    if (optimize_level == 0){
        opt_level = OptimizationLevel::O0;
    } else if (optimize_level == 1) {
        opt_level = OptimizationLevel::O1;
    } else if (optimize_level == 2) {
        opt_level = OptimizationLevel::O2;
    } else if (optimize_level == 3) {
        opt_level = OptimizationLevel::O3;
    } // TODO add optimization for size (Os) and 0z ?

    if (opt_level == OptimizationLevel::O0){
        module_pm = pass_builder.buildO0DefaultPipeline(opt_level, Comp_context->lto_mode);
    } else if (Comp_context->lto_mode){
        module_pm = pass_builder.buildLTOPreLinkDefaultPipeline(opt_level);
    } else {
        module_pm = pass_builder.buildPerModuleDefaultPipeline(opt_level);
    }
    legacy::PassManager codegen_pass;
    codegen_pass.add(createTargetTransformInfoWrapperPass(TheTargetMachine->getTargetIRAnalysis()));
    
    if (!Comp_context->lto_mode){
    if (TheTargetMachine->addPassesToEmitFile(codegen_pass, dest, nullptr, FileType)) {
    errs() << _("TheTargetMachine can't emit a file of this type");
    return 1;
    }
    }
    
    module_pm.run(*TheModule, module_am);

    TheModule->print(*file_out_ostream, nullptr);

    codegen_pass.run(*TheModule);


    if (Comp_context->lto_mode){
        writeBitcodeLTO(object_filename, false);
    }
    if (time_report) {
        TimerGroup::printAll(errs());
    }

    dest.flush();
    delete TheTargetMachine;
    return 0;
}