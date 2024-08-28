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

std::string CIR::BasicBlock::to_string(){
    std::string basic_block_cir = name + ":\n";
    for (int i = 0; i < instructions.size(); i++){
        basic_block_cir += instructions.at(i)->to_string();
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
    for (int i = 0; i < basicBlocks.size(); i++){
        func_cir += basicBlocks.at(i)->to_string();
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