#include "cir.h"

#if ENABLE_CIR

#include "../types.h"

std::string CIR::ConstNumber::to_string(){
    std::string const_nb_cir = "const ";
    if (is_float){
        const_nb_cir += "double " + std::to_string(nb_val.float_nb);
    } else {
        std::string val_str = std::to_string(nb_val.int_nb);
        if (type.type == bool_type){
            val_str = (nb_val.int_nb == 1) ? "true" : "false";
        }
        const_nb_cir += create_pretty_name_for_type(type) + " " + val_str;
    }
    return const_nb_cir;
}

std::string CIR::VarInit::to_string(){
    std::string init_val_cir = (VarName.second.is_empty()) ? "" : VarName.second.to_string();
    std::string varinit_cir = "var " + create_pretty_name_for_type(var_type) + " " + VarName.first + " " + init_val_cir ;
    return varinit_cir;
}

std::string CIR::CallInstruction::to_string(){
    std::string call_cir = "call " + Callee + "(";
    for (int i = 0; i < Args.size(); i++){
        if (i != 0){
            call_cir += ", ";
        }
        call_cir += Args.at(i).to_string(); // add type of instruction for every arg
    }
    call_cir += ")";
    return call_cir;
}

std::string CIR::ReturnInstruction::to_string(){
    return "return " + ret_val.to_string();
}

std::string CIR::LoadGlobalInstruction::to_string(){
    std::string load_global_cir = "load_global ";
    if (is_string){
        load_global_cir += "(str)";
    }
    load_global_cir += std::to_string(global_pos);
    return load_global_cir;
}

std::string CIR::LoadArgInstruction::to_string(){
    return "load_arg " + create_pretty_name_for_type(arg_type) + " " + arg_name;
}

std::string CIR::GotoInstruction::to_string(){
    return "goto " + goto_bb.to_string();
}

std::string CIR::SizeofInstruction::to_string(){
    return "sizeof " + ((is_type) ? create_pretty_name_for_type(type) : var.to_string());
}

/*std::string CIR::BasicBlock::to_string(){
    std::string basic_block_cir = name + ":\n";
    for (int i = 0; i < instructions.size(); i++){
        basic_block_cir += instructions.at(i)->to_string();
    }
    return basic_block_cir;
}*/

std::string CIR::InstructionRef::to_string(){
    std::string instruction_ref_str = "%" + std::to_string(instruction_pos);
    if (instruction_pos == -1) {
        instruction_ref_str += " (invalid)";
    }
    return instruction_ref_str; 
}

std::string CIR::BasicBlock::to_string(int& InstructionIndex){
    std::string basic_block_cir = name + ":";
    if (!predecessors.empty()){
        basic_block_cir += "\t\t\t // predecessors : ";
        for (int i = 0; i < predecessors.size(); i++){
            if (i != 0){
                basic_block_cir += ", ";
            }
            basic_block_cir += predecessors.at(i).get_label();
        }
    }
    basic_block_cir += "\n";
    for (int i = 0; i < instructions.size(); i++){
        basic_block_cir += "\t%" + std::to_string(InstructionIndex);
        if (instructions.at(i)->label != ""){
            basic_block_cir += "(" + instructions.at(i)->label + ") ";
        }
        if (!instructions.at(i)->type.is_empty){
            basic_block_cir += ": " + create_pretty_name_for_type(instructions.at(i)->type);
        }
        basic_block_cir += " = " + instructions.at(i)->to_string() + "\n";
        InstructionIndex++;
    }
    return basic_block_cir;
}

std::string CIR::Function::to_string(){
    std::string func_cir = "func " + proto->name + "(";
    for (int i = 0; i < proto->args.size(); i++){
        if (i != 0){
            func_cir += ", ";
        }
        func_cir += create_pretty_name_for_type(proto->args.at(i).second) + " " + proto->args.at(i).first;
    }
    func_cir += ") ";
    func_cir += create_pretty_name_for_type(proto->return_type); 
    func_cir += " {\n";
    int InstructionIndex = 0;
    for (int i = 0; i < basicBlocks.size(); i++){
        func_cir += basicBlocks.at(i)->to_string(InstructionIndex);
    }
    func_cir += "}";
    return func_cir;
}

static std::string escape_string(const std::string& input){
    std::string output;
    output.reserve(input.size()); // Reserve space to avoid frequent re-allocations
    for (int i = 0; i < input.length(); i++){
        switch (input.at(i)){
            case '\n':
                output += "\\n";
                break;
            case '\t':
                output += "\\t";
                break;
            default:
                output += input[i];
                break;
        }
    }
    return output;
}

std::string FileCIR::to_string(){
    std::string file_cir_str = "// WARNING : this content is only for debug purposes and to be read by an human. The format of it could change at any moment.\n\n";
    
    if (!strings.empty()){
    file_cir_str += "strs = {\n";
    for (int i = 0; i < strings.size(); i++){
        file_cir_str += "\t" + std::to_string(i) + " = \"" + escape_string(strings.at(i)) + "\"\n";
    }
    file_cir_str += "}\n";
    }

    
    for (int i = 0; i < functions.size(); i++){
        file_cir_str += functions.at(i)->to_string() + "\n\n";
    }
    return file_cir_str;
}

#endif