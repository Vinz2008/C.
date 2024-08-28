#include "config.h"
#include <vector>
#include <memory>

#if ENABLE_CIR

namespace CIR {
    class Instruction {
    public:
        virtual ~Instruction() = default;
        virtual std::string to_string() = 0;
    };
    class BasicBlock {
    public:
        std::vector<std::unique_ptr<Instruction>> instructions;
    };
    // TODO : make it a base class
    class Value {
    public:
        Value() {}
    };
}

class FileCIR {
public:
    std::vector<std::unique_ptr<CIR::BasicBlock>> basicBlocks;
    FileCIR(std::vector<std::unique_ptr<CIR::BasicBlock>> basicBlocks) : basicBlocks(std::move(basicBlocks)) {} 
};

#endif