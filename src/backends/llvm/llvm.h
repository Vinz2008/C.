#include <memory>
#include <map>
#include "llvm/TargetParser/Triple.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"

using namespace llvm;


class FileCIR;

namespace CIR {
    class BasicBlock;
}

int codegenLLVM(std::unique_ptr<FileCIR> fileCIR, std::string object_filename, Triple tripleLLVM, bool PICmode, bool asm_mode, bool time_report, bool is_optimised, bool thread_sanitizer, int optimize_level, std::string CPU, std::string cpu_features);

namespace LLVM {
    class Context {
    public:
        std::unique_ptr<LLVMContext> TheContext;
        std::unique_ptr<Module> TheModule;
        std::unique_ptr<IRBuilder<>> Builder;
        std::unordered_map<std::string, Type*> structDeclars;
        std::vector<Constant*> staticStrings;
        std::vector<Value*> GlobalValues;
        std::unordered_map< std::string, GlobalVariable*> GlobalVars;
        int bb_codegen_number;
        std::vector<std::pair<CIR::BasicBlock*, BasicBlock*>> functionBBs;
        //std::map<std::string, BasicBlock*> functionBBs; // ordered map
        std::vector<Value*> functionValues; // Values of each instruction (by pos) for the Basic Block that is codegened
        std::unordered_map<std::string, AllocaInst*> functionVarsAllocas; // TODO : rename to functionArgsAllocas ?
        Context(std::unique_ptr<LLVMContext> TheContext, std::unique_ptr<Module> TheModule, std::unique_ptr<IRBuilder<>> Builder) : TheContext(std::move(TheContext)), TheModule(std::move(TheModule)), Builder(std::move(Builder)), structDeclars(), staticStrings(), GlobalVars(), bb_codegen_number(0), functionBBs(), functionValues(), functionVarsAllocas() {}
        BasicBlock* get_function_BB(CIR::BasicBlock* bb){
            for (int i = 0; i < functionBBs.size(); i++){
                if (functionBBs.at(i).first == bb){
                    return functionBBs.at(i).second;
                }
            }
            return nullptr;
        }
    };
}