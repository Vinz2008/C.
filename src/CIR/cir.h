#include "../config.h"

#if ENABLE_CIR

#include <vector>
#include <memory>
#include "../types.h"

namespace CIR {
    class Instruction {
    public:
        std::string label;
        Instruction(std::string label = "") : label(label) {}
        virtual ~Instruction() = default;
        virtual std::string to_string() = 0;
    };
    class ConstNumber : public CIR::Instruction {
    public:
        bool is_float;
        union nb_val_ty {
            double float_nb;
            int int_nb;
        } nb_val;
        ConstNumber(bool is_float, union nb_val_ty nb_val) : is_float(is_float), nb_val(nb_val) {}
        std::string to_string() override;
    };
    class BasicBlock {
    public:
        std::string name;
        std::vector<std::unique_ptr<Instruction>> instructions;
        BasicBlock(std::string name, std::vector<std::unique_ptr<Instruction>> instructions = std::vector<std::unique_ptr<Instruction>>()) : name(name), instructions(std::move(instructions)) {}
        std::string to_string();
    };
    class FunctionProto {
    public:
        std::string name;
        std::vector<std::pair<std::string, Cpoint_Type>> args;
        Cpoint_Type return_type;
        FunctionProto(std::string name, Cpoint_Type return_type, std::vector<std::pair<std::string, Cpoint_Type>> args) : name(name), return_type(return_type), args(std::move(args)) {}
    };
    class Function {
    public:
        //std::string name;
        std::unique_ptr<FunctionProto> proto;
        std::vector<std::unique_ptr<CIR::BasicBlock>> basicBlocks;
        Function(std::unique_ptr<FunctionProto> proto, std::vector<std::unique_ptr<CIR::BasicBlock>> basicBlocks = {}) : proto(std::move(proto)), basicBlocks(std::move(basicBlocks)) {}
        std::string to_string();
    };
    // TODO : make it a base class
    class Value {
    public:
        Value() {}
    };
}

class FileCIR {
public:
    std::vector<std::unique_ptr<CIR::Function>> functions;
    // TODO : add function to reset all these ptrs and indices
    CIR::Function* CurrentFunction;
    CIR::BasicBlock* CurrentBasicBlock;
    //int InsertPoint;
    FileCIR(std::vector<std::unique_ptr<CIR::Function>> functions) : functions(std::move(functions)), CurrentFunction(nullptr), CurrentBasicBlock(nullptr) /*, InsertPoint(0)*/  {} 
    void add_function(std::unique_ptr<CIR::Function> func){
        functions.push_back(std::move(func));
        CurrentFunction = functions.back().get();
    }

    void add_basic_block(std::unique_ptr<CIR::BasicBlock> basic_block){
        assert(CurrentFunction != nullptr);
        CurrentFunction->basicBlocks.push_back(std::make_unique<CIR::BasicBlock>("entry"));
        CurrentBasicBlock = CurrentFunction->basicBlocks.back().get();
    }

    void add_instruction(std::unique_ptr<CIR::Instruction> instruction){
        assert(CurrentBasicBlock != nullptr);
        CurrentBasicBlock->instructions.push_back(std::move(instruction));
        /*if (InsertPoint == CurrentBasicBlock->instructions.size()){
            CurrentBasicBlock->instructions.push_back(std::move(instruction));
            InsertPoint++;
        } else {
            CurrentBasicBlock->instructions.insert(CurrentBasicBlock->instructions.begin() + InsertPoint, std::move(instruction));
        }*/
    }
    
    std::string to_string();
};

#endif