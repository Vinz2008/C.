#include <string>
#include <vector>
#include <memory>
#include <cassert>

//#include "instructions.h"

class FileCIR;

namespace CIR {
    class Instruction;
    /*class BasicBlockRef {
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
    };*/
    class BasicBlock {
    public:
        std::string name;
        std::vector<std::unique_ptr<Instruction>> instructions; // TODO : remove unique_ptr
        //std::vector<BasicBlockRef> predecessors;
        std::vector<BasicBlock*> predecessors;
        BasicBlock(FileCIR* fileCIR, std::string name, std::vector<std::unique_ptr<Instruction>> instructions = std::vector<std::unique_ptr<Instruction>>()); /*: name(name), instructions(std::move(instructions)), predecessors() {
            if (fileCIR->get_basic_block_from_name(name) != nullptr){
                name += std::to_string(fileCIR->already_named_index);
                already_named_index
            }
        }*/
        //std::string to_string();
        std::string to_string(int& InstructionIndex);
    };

}


std::string BasicBlock_ptr_to_string(CIR::BasicBlock* bb);