#include "../config.h"

#if ENABLE_CIR

#include "../types.h"

namespace CIR {
    class Instruction {
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
    class LoadVarInstruction : public CIR::Instruction {
    public:
        InstructionRef var;
        LoadVarInstruction(InstructionRef var) : var(var){}
        std::string to_string() override;
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
    };
}

#endif