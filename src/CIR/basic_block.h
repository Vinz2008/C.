#include <string>
#include <vector>
#include <memory>
#include <cassert>

//#include "instructions.h"

namespace CIR {
    class Instruction;
    class BasicBlockRef {
        bool empty_ref;
        int basic_block_pos;
        std::string label;
    public:
        BasicBlockRef() : empty_ref(true), basic_block_pos(-1) {}
        BasicBlockRef(int basic_block_pos, std::string label) : empty_ref(false), basic_block_pos(basic_block_pos), label(label) {}
        int get_pos(){
            assert(!empty_ref);
            return basic_block_pos;
        }
        std::string get_label(){
            return label;
        }
        bool is_empty(){
            return empty_ref;
        }
        std::string to_string(){
            if (basic_block_pos == -1) {
                return "(invalid basic block)";
            }
            return label + ":"; 
        }
    };
    class BasicBlock {
    public:
        std::string name;
        std::vector<std::unique_ptr<Instruction>> instructions;
        std::vector<BasicBlockRef> predecessors;
        BasicBlock(std::string name, std::vector<std::unique_ptr<Instruction>> instructions = std::vector<std::unique_ptr<Instruction>>()) : name(name), instructions(std::move(instructions)), predecessors() {}
        //std::string to_string();
        std::string to_string(int& InstructionIndex);
    };

}