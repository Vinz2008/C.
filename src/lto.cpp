//#include "llvm/Transforms/IPO/ThinLTOBitcodeWriter.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/LTO/legacy/ThinLTOCodeGenerator.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Transforms/IPO.h"
#include "llvm/Bitcode/BitcodeWriterPass.h"
#include <fstream>


#include "log.h"

using namespace llvm;

extern std::unique_ptr<Module> TheModule;

void writeBitcodeLTO(std::string filename, bool is_thin){
    legacy::PassManager pass;
    std::string bitcode;
    raw_string_ostream OS(bitcode);
    if (is_thin){
        std::cout << "ERROR : for now thin LTO doesn't work" << std::endl;
        exit(1);
    } else {
        pass.add(createBitcodeWriterPass(OS));
        pass.run(*TheModule);
    }
    std::ofstream outfile;
    outfile.open(filename);
    outfile << bitcode;
    outfile.close();
}