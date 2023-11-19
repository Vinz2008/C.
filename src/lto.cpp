//#include "llvm/Transforms/IPO/ThinLTOBitcodeWriter.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/LTO/legacy/ThinLTOCodeGenerator.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Transforms/IPO.h"
#include "llvm/Bitcode/BitcodeWriterPass.h"


#include "log.h"

using namespace llvm;
/*

void getBitcodeData(Module mod){
    std::string str;
    legacy::PassManager pass;
    raw_string_ostream OS(str);
    
    pass.add(ThinLTOBitcodeWriterPass(OS, nullptr));
    pass.run(Mod);
}
*/

extern std::unique_ptr<Module> TheModule;

void writeBitcodeLTO(std::string filename, bool is_thin){
    std::error_code EC;
    //llvm::raw_fd_ostream bc(filename.c_str(), EC, llvm::sys::fs::OF_None);
    legacy::PassManager pass;
    std::string str;
    raw_string_ostream OS(str);
    if (EC) {
        // TODO : create an error here
        std::cout << "ERROR : couldn't write LTO bitcode" << std::endl;
        exit(1);
    }
    if (is_thin){
        // It will need the new pass manager to work (createWriteThinLTOBitcodePass was removed) (TODO)
        std::cout << "ERROR : for now thin LTO doesn't work" << std::endl;
        exit(1);
    } else {
        pass.add(createBitcodeWriterPass(OS));
        pass.run(*TheModule);
    }
    //pass->run(*TheModule);
    // Log::Info() << "bitcode : " << str << "\n";
}