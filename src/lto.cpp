/*#include "llvm/Transforms/IPO/ThinLTOBitcodeWriter.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Module.h"

using namespace llvm;


void getBitcodeData(Module mod){
    std::string str;
    legacy::PassManager pass;
    raw_string_ostream OS(str);
    
    pass.add(ThinLTOBitcodeWriterPass(OS, nullptr));
    pass.run(Mod);
}
*/