#ifndef _CIR_HEADER_
#define _CIR_HEADER_
#include "../config.h"

#if ENABLE_CIR

#include <vector>
#include <memory>
#include "../types.h"

#include "instructions.h"


namespace CIR {
    /*class Instruction {
    public:
        // TODO : add a type to each instruction
        std::string label;
        Instruction(std::string label = "") : label(label) {}
        virtual ~Instruction() = default;
        virtual std::string to_string() = 0;
    };
    class InstructionRef {
      bool empty_ref;
      int instruction_pos;
    public:
        InstructionRef() : empty_ref(true), instruction_pos(-1) {}
        InstructionRef(int instruction_pos) : empty_ref(false), instruction_pos(instruction_pos) {}
        int get_pos(){
            assert(!empty_ref);
            return instruction_pos;
        }
        bool is_empty(){
            return empty_ref;
        }
        std::string to_string(){
            return "%" + std::to_string(instruction_pos); 
        }
    };
    class LoadInstruction : public CIR::Instruction {
    public:
        LoadInstruction(){}
    };
    class VarInit : public CIR::Instruction {
    public:
        // TODO : add multiple vars support
        std::pair<std::string, InstructionRef> VarName;
        VarInit(std::pair<std::string, InstructionRef> VarName) : VarName(VarName) {}
        std::string to_string() override;
    };
    class ConstNumber : public CIR::Instruction {
    public:
        bool is_float;
        Cpoint_Type type;
        union nb_val_ty {
            double float_nb;
            int int_nb;
        } nb_val;
        ConstNumber(bool is_float, Cpoint_Type type, union nb_val_ty nb_val) : is_float(is_float), type(type), nb_val(nb_val) {}
        std::string to_string() override;
    };*/
    /*class BasicBlock {
    public:
        std::string name;
        std::vector<std::unique_ptr<Instruction>> instructions;
        BasicBlock(std::string name, std::vector<std::unique_ptr<Instruction>> instructions = std::vector<std::unique_ptr<Instruction>>()) : name(name), instructions(std::move(instructions)) {}
        //std::string to_string();
        std::string to_string(int& InstructionIndex);
    };
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
    class FunctionProto {
    public:
        std::string name;
        Cpoint_Type return_type;
        std::vector<std::pair<std::string, Cpoint_Type>> args;
        bool is_variable_number_args;
        bool is_private_func;
        FunctionProto() : name(), return_type(), args(), is_variable_number_args(false), is_private_func(false) {}
        FunctionProto(std::string name, Cpoint_Type return_type, std::vector<std::pair<std::string, Cpoint_Type>> args, bool is_variable_number_args, bool is_private_func) : name(name), return_type(return_type), args(std::move(args)), is_variable_number_args(is_variable_number_args), is_private_func(is_private_func) {}
    };
    class GlobalVar {
    public:
        std::string name;
        Cpoint_Type type;
        bool is_const;
        bool is_extern;
        CIR::InstructionRef Init;
        std::string section_name; // TODO : have a better way to have attributes to global vars (an Attributes class ?)
        GlobalVar(std::string name, Cpoint_Type type, bool is_const, bool is_extern, CIR::InstructionRef Init, std::string section_name) : name(name), type(type), is_const(is_const), is_extern(is_extern), Init(Init), section_name(section_name) {}
    };
    class Var {
    public:
        InstructionRef var_ref;
        Cpoint_Type type;
        Var() : var_ref(), type() {}
        Var(InstructionRef var_ref, Cpoint_Type type) : var_ref(var_ref), type(type) {}
    };
    class Function {
    public:
        //std::string name;
        std::unique_ptr<FunctionProto> proto;
        std::vector<std::unique_ptr<CIR::BasicBlock>> basicBlocks;
        std::unordered_map<std::string, Var> vars;
        Function() : proto(), basicBlocks(), vars() {}
        Function(std::unique_ptr<FunctionProto> proto, std::vector<std::unique_ptr<CIR::BasicBlock>> basicBlocks = {}, std::unordered_map<std::string, Var> vars = {}) : proto(std::move(proto)), basicBlocks(std::move(basicBlocks)), vars(vars) {}
        std::string to_string();
        Instruction* get_instruction(int pos){
            int iter_pos = 0;
            for (int i = 0; i < basicBlocks.size(); i++){
                for (int j = 0; j < basicBlocks.at(i)->instructions.size(); j++){
                    if (iter_pos == pos){
                        return basicBlocks.at(i)->instructions.at(j).get();
                    }
                    iter_pos++;
                }
            }
            return nullptr;
        }
    };
    // TODO : make it a base class
    class Value {
    public:
        Value() {}
    };
}

class FileCIR {
public:
    std::string filename; // not the path of the path of the real file compiled (it is a .temp file), the filename is the name of the file passed to the cli
    std::vector<std::unique_ptr<CIR::Function>> functions;
    std::unordered_map<std::string, CIR::FunctionProto> protos;
    std::unordered_map<std::string, std::unique_ptr<CIR::GlobalVar>> global_vars;
    std::vector<std::string> strings;
    // TODO : add function to reset all these ptrs and indices
    CIR::Function global_context; // is a (false) function with global vars (is used to have const instructions for global vars init)
    CIR::Function* CurrentFunction;
    //CIR::BasicBlock* CurrentBasicBlock;
    CIR::BasicBlockRef CurrentBasicBlock;
    //int InsertPoint;
    FileCIR(std::string filename, std::vector<std::unique_ptr<CIR::Function>> functions) : filename(filename), functions(std::move(functions)), protos(), global_vars(), strings(), global_context(), CurrentFunction(nullptr), CurrentBasicBlock() /*, InsertPoint(0)*/  {} 
    
    void add_function(std::unique_ptr<CIR::Function> func){
        functions.push_back(std::move(func));
        CurrentFunction = functions.back().get();

        // reset refs that are no longer valid
        CurrentBasicBlock = CIR::BasicBlockRef();
    }

    CIR::BasicBlockRef add_basic_block(std::unique_ptr<CIR::BasicBlock> basic_block){
        assert(CurrentFunction != nullptr);
        std::string bb_name = basic_block->name; 
        CurrentFunction->basicBlocks.push_back(std::move(basic_block) /*std::make_unique<CIR::BasicBlock>("entry")*/);
        //CurrentBasicBlock = CurrentFunction->basicBlocks.back().get();
        //return CurrentFunction->basicBlocks.back().get();
        return CIR::BasicBlockRef(CurrentFunction->basicBlocks.size()-1, bb_name);
    }

    CIR::BasicBlock* get_basic_block(CIR::BasicBlockRef basic_block_ref){
        return CurrentFunction->basicBlocks.at(basic_block_ref.get_pos()).get();
    }

    CIR::Instruction* get_instruction(CIR::InstructionRef instruction_ref){
        int ret_instruction_pos = 0;
        int instruction_ref_pos = instruction_ref.get_pos();
        for (int i = 0; i < CurrentFunction->basicBlocks.size(); i++){
            for (int j = 0; j < CurrentFunction->basicBlocks.at(i)->instructions.size(); j++){
                if (ret_instruction_pos == instruction_ref_pos){
                    return CurrentFunction->basicBlocks.at(i)->instructions.at(j).get();
                }
                ret_instruction_pos++;
            }
        }
        return nullptr;
    }

    CIR::BasicBlockRef get_basic_block_from_name(std::string bb_name){
        for (int i = 0; i < CurrentFunction->basicBlocks.size(); i++){
            if (CurrentFunction->basicBlocks.at(i)->name == bb_name){
                return CIR::BasicBlockRef(i, bb_name);
            }
        }
        return CIR::BasicBlockRef();
    }

    void set_insert_point(CIR::BasicBlockRef basic_block){
        CurrentBasicBlock = basic_block;
    }

    CIR::InstructionRef add_instruction(std::unique_ptr<CIR::Instruction> instruction){
        //assert(CurrentBasicBlock != nullptr);
        CIR::BasicBlock* bb = get_basic_block(CurrentBasicBlock);
        bb->instructions.push_back(std::move(instruction));
        return CIR::InstructionRef(bb->instructions.size()-1);
        /*if (InsertPoint == CurrentBasicBlock->instructions.size()){
            CurrentBasicBlock->instructions.push_back(std::move(instruction));
            InsertPoint++;
        } else {
            CurrentBasicBlock->instructions.insert(CurrentBasicBlock->instructions.begin() + InsertPoint, std::move(instruction));
        }*/
    }

    void start_global_context(){
        CurrentFunction = &global_context;
        std::string global_bb_name = "globals";
        global_context.basicBlocks.push_back(std::make_unique<CIR::BasicBlock>(global_bb_name));
        set_insert_point(get_basic_block_from_name(global_bb_name));
    }

    void end_global_context(){
        CurrentFunction = nullptr;
        CurrentBasicBlock = CIR::BasicBlockRef();
    }
    
    std::string to_string();
};

#endif

#endif