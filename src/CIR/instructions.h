#ifndef _CIR_INSTRUCTIONS_
#define _CIR_INSTRUCTIONS_

#include "../config.h"

#if ENABLE_CIR

#include "../types.h"

#include "basic_block.h"

//#include "cir.h"

class FileCIR;

namespace CIR {
    class BasicBlockRef;
    class Instruction {
    public:
        // TODO : add a type to each instruction
        std::string label;
        Cpoint_Type type; // TODO : replace with CIR_Type ?
        Instruction(std::string label = "", Cpoint_Type type = Cpoint_Type()) : label(label), type(type) {}
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
        std::string to_string(); /*{
            std::string instruction_ref_str = "%" + std::to_string(instruction_pos);
            CIR::Instruction* instr = fileCIR->get_instruction(*this);
            if (instr->label != ""){
                instruction_ref_str += "(" + instr->label + ")";
            }
            if (!instr->type.is_empty){
                instruction_ref_str += " : " + create_pretty_name_for_type(type);
            }
            if (instruction_pos == -1) {
                instruction_ref_str += " (invalid)";
            }
            return instruction_ref_str; 
        }*/
    };
    class CallInstruction : public CIR::Instruction {
    public:
        std::string Callee;
        std::vector<InstructionRef> Args;
        CallInstruction(std::string Callee, std::vector<InstructionRef> Args) : Callee(Callee), Args(Args) {}
        std::string to_string() override;
    };
    class ReturnInstruction : public CIR::Instruction {
    public:
        InstructionRef ret_val;
        ReturnInstruction(InstructionRef ret_val) : ret_val(ret_val) {}
        std::string to_string() override;
    };
    /*class LoadVarInstruction : public CIR::Instruction {
    public:
        InstructionRef var;
        Cpoint_Type load_type;
        LoadVarInstruction(InstructionRef var, Cpoint_Type load_type) : var(var), load_type(load_type) {}
        std::string to_string() override;
    };*/
    class LoadGlobalInstruction : public CIR::Instruction {
    public:
        bool is_string;
        int global_pos; // TODO : replace with a GlobalRef type ?
        LoadGlobalInstruction(bool is_string, int global_pos) : is_string(is_string), global_pos(global_pos) {}
        std::string to_string() override;
    };
    class LoadArgInstruction : public CIR::Instruction {
    public:
        std::string arg_name;
        Cpoint_Type arg_type;
        LoadArgInstruction(std::string arg_name, Cpoint_Type arg_type) : arg_name(arg_name), arg_type(arg_type) {}
        std::string to_string() override;
    };
    class VarInit : public CIR::Instruction {
    public:
        // TODO : add multiple vars support
        std::pair<std::string, InstructionRef> VarName;
        Cpoint_Type var_type;
        VarInit(std::pair<std::string, InstructionRef> VarName, Cpoint_Type var_type) : VarName(VarName), var_type(var_type) {}
        std::string to_string() override;
    };
    class CastInstruction : public CIR::Instruction {
    public:
        InstructionRef val; // TODO : add to Instructions the types of instructions to have the from type of the cast
        Cpoint_Type cast_type;
        CastInstruction(InstructionRef val, Cpoint_Type cast_type) : val(val), cast_type(cast_type) {}
        std::string to_string() override;
    };
    
    class GotoInstruction : public CIR::Instruction {
    public:
        CIR::BasicBlockRef goto_bb;
        GotoInstruction(CIR::BasicBlockRef goto_bb) : goto_bb(goto_bb) {}
        std::string to_string() override;
    };

    class GotoIfInstruction : public CIR::Instruction {
    public:
        CIR::InstructionRef cond_instruction;
        CIR::BasicBlockRef goto_bb_if_true;
        CIR::BasicBlockRef goto_bb_if_false;
        GotoIfInstruction(CIR::InstructionRef cond_instruction, CIR::BasicBlockRef goto_bb_if_true, CIR::BasicBlockRef goto_bb_if_false) : cond_instruction(cond_instruction), goto_bb_if_true(goto_bb_if_true), goto_bb_if_false(goto_bb_if_false) {}
        std::string to_string() override;
    };

    class SizeofInstruction : public CIR::Instruction {
    public:
        bool is_type;
        Cpoint_Type type;
        InstructionRef var;
        SizeofInstruction(bool is_type, Cpoint_Type type, InstructionRef var) : is_type(is_type), type(type), var(var) {
            if (!var.is_empty()){
                assert(type.is_empty);
            }
            if (!type.is_empty){
                assert(var.is_empty());
            }
        }
        std::string to_string() override;
    };

    class AddInstruction : public CIR::Instruction {
    public:
        InstructionRef arg1;
        InstructionRef arg2;
        AddInstruction(InstructionRef arg1, InstructionRef arg2) : arg1(arg1), arg2(arg2) {}
        std::string to_string() override;
    };

    class SubInstruction : public CIR::Instruction {
    public:
        InstructionRef arg1;
        InstructionRef arg2;
        SubInstruction(InstructionRef arg1, InstructionRef arg2) : arg1(arg1), arg2(arg2) {}
        std::string to_string() override;
    };

    class MulInstruction : public CIR::Instruction {
    public:
        InstructionRef arg1;
        InstructionRef arg2;
        MulInstruction(InstructionRef arg1, InstructionRef arg2) : arg1(arg1), arg2(arg2) {}
        std::string to_string() override;
    };

    class RemInstruction : public CIR::Instruction {
    public:
        InstructionRef arg1;
        InstructionRef arg2;
        RemInstruction(InstructionRef arg1, InstructionRef arg2) : arg1(arg1), arg2(arg2) {}
        std::string to_string() override;
    };

    class DivInstruction : public CIR::Instruction {
    public:
        InstructionRef arg1;
        InstructionRef arg2;
        DivInstruction(InstructionRef arg1, InstructionRef arg2) : arg1(arg1), arg2(arg2) {}
        std::string to_string() override;
    };

    class AndInstruction : public CIR::Instruction {
    public:
        InstructionRef arg1;
        InstructionRef arg2;
        AndInstruction(InstructionRef arg1, InstructionRef arg2) : arg1(arg1), arg2(arg2) {}
        std::string to_string() override;
    };

    class OrInstruction : public CIR::Instruction {
    public:
        InstructionRef arg1;
        InstructionRef arg2;
        OrInstruction(InstructionRef arg1, InstructionRef arg2) : arg1(arg1), arg2(arg2) {}
        std::string to_string() override;
    };

    class ShiftInstruction : public CIR::Instruction {
    public:
        enum shift_type_ty {
            SHIFT_LEFT,
            SHIFT_RIGHT,
        } shift_type;
        InstructionRef arg1;
        InstructionRef arg2;
        ShiftInstruction(enum shift_type_ty shift_type, InstructionRef arg1, InstructionRef arg2) : shift_type(shift_type), arg1(arg1), arg2(arg2) {}
        std::string to_string() override;
    };

    class CmpInstruction : public CIR::Instruction {
    public:
        enum cmp_type_ty {
            CMP_EQ,
            CMP_GREATER,
            CMP_GREATER_EQ,
            CMP_LOWER,
            CMP_LOWER_EQ,
        } cmp_type;
        InstructionRef arg1;
        InstructionRef arg2;
        CmpInstruction(enum cmp_type_ty cmp_type, InstructionRef arg1, InstructionRef arg2) : cmp_type(cmp_type), arg1(arg1), arg2(arg2) {}
        std::string to_string() override;
    };
    class ConstNumber;
    class ConstVoid;
    class ConstNever;
    class ConstNull;
    class ConstInstruction : public CIR::Instruction {
    public:
        ConstInstruction(){}
        friend bool operator==(const ConstInstruction& const1, const ConstInstruction& const2); /*{
            if (dynamic_cast<const ConstNumber*>(&const1) && dynamic_cast<const ConstNumber*>(&const2)){
                auto const1nb = dynamic_cast<const ConstNumber*>(&const1);
                auto const2nb = dynamic_cast<const ConstNumber*>(&const2);
                bool is_enum_val_same = false;
                if (const1nb->is_float == const2nb->is_float){
                    if (const1nb->is_float){
                        is_enum_val_same = const1nb->nb_val.float_nb == const2nb->nb_val.float_nb;
                    } else {
                        is_enum_val_same = const1nb->nb_val.int_nb == const2nb->nb_val.int_nb;
                    }
                }
                return is_enum_val_same && const1nb->type == const1nb->type;
            }
            if (dynamic_cast<const ConstVoid*>(&const1) && dynamic_cast<const ConstVoid*>(&const2)){
                return true;
            }
            if (dynamic_cast<const ConstNever*>(&const1) && dynamic_cast<const ConstNever*>(&const2)){
                return true;
            }
            if (dynamic_cast<const ConstNull*>(&const1) && dynamic_cast<const ConstNull*>(&const2)){
                return true;
            }
            return false;
        }*/
    };
    

    class ConstNumber : public CIR::ConstInstruction {
    public:
        bool is_float;
        Cpoint_Type type; // TODO : remove the type ? because it is already in the instruction ? or add a get_type member for instructions and then use it to add the types to Instructions
        union nb_val_ty {
            double float_nb;
            int int_nb;
        } nb_val;
        ConstNumber(bool is_float, Cpoint_Type type, union nb_val_ty nb_val) : is_float(is_float), type(type), nb_val(nb_val) {}
        std::string to_string() override;
    };
    class ConstVoid : public CIR::ConstInstruction {
    public:
        ConstVoid(){}
        std::string to_string() override {
            return "void";
        }
    };
    class ConstNever : public CIR::ConstInstruction {
    public:
        ConstNever(){}
        std::string to_string() override {
            return "!";
        }
    };
    class ConstNull : public CIR::ConstInstruction {
    public:
        ConstNull(){}
        std::string to_string() override {
            return "null";
        }
    };
}

#endif

#endif