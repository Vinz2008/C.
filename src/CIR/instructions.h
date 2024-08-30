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
            if (instruction_pos == -1) {
                return "%" + std::to_string(instruction_pos) + " (invalid)";
            }
            return "%" + std::to_string(instruction_pos); 
        }
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
    class LoadVarInstruction : public CIR::Instruction {
    public:
        InstructionRef var;
        Cpoint_Type load_type;
        LoadVarInstruction(InstructionRef var, Cpoint_Type load_type) : var(var), load_type(load_type) {}
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
    class ConstVoid : public CIR::Instruction {
    public:
        ConstVoid(){}
        std::string to_string() override {
            return "void";
        }
    };
    class Null : public CIR::Instruction {
    public:
        Null(){}
        std::string to_string() override {
            return "null";
        }
    };
}

#endif