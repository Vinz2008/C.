#include "cir.h"

#if ENABLE_CIR

std::string CIR::ConstNumber::to_string(){
    std::string const_nb_cir = "const ";
    if (is_float){
        const_nb_cir += "double " + std::to_string(nb_val.float_nb);
    } else {
        const_nb_cir += "i32 " + std::to_string(nb_val.int_nb);
    }
    return const_nb_cir;
}

std::string CIR::VarInit::to_string(){
    std::string init_val_cir = (VarName.second.is_empty()) ? "" : VarName.second.to_string();
    std::string varinit_cir = "var " + VarName.first + " " + init_val_cir ;
    return varinit_cir;
}


std::string CIR::LoadVarInstruction::to_string(){
    return var.to_string();
}
/*std::string CIR::BasicBlock::to_string(){
    std::string basic_block_cir = name + ":\n";
    for (int i = 0; i < instructions.size(); i++){
        basic_block_cir += instructions.at(i)->to_string();
    }
    return basic_block_cir;
}*/

std::string CIR::BasicBlock::to_string(int& InstructionIndex){
    std::string basic_block_cir = name + ":\n";
    for (int i = 0; i < instructions.size(); i++){
        basic_block_cir += "\t%" + std::to_string(InstructionIndex) + " = " + instructions.at(i)->to_string() + "\n";
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

std::string FileCIR::to_string(){
    std::string file_cir_str = "// WARNING : this content is only for debug purposes and to be read by an human. The format of it could change at any moment.\n\n";
    for (int i = 0; i < functions.size(); i++){
        file_cir_str += functions.at(i)->to_string() + "\n\n";
    }
    return file_cir_str;
}

#endif