#include "types.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Value.h"
#include "llvm/TargetParser/Triple.h"
#include "codegen.h"
#include "ast.h"
#include "log.h"
#include "lexer.h"
#include <unordered_map>

using namespace llvm;

extern std::unique_ptr<LLVMContext> TheContext;
extern std::unique_ptr<IRBuilder<>> Builder;
extern std::unordered_map<std::string, std::unique_ptr<StructDeclaration>> StructDeclarations;
extern std::unordered_map<std::string, std::unique_ptr<UnionDeclaration>> UnionDeclarations;
extern std::unordered_map<std::string, std::unique_ptr<EnumDeclaration>> EnumDeclarations;
std::vector<Cpoint_Type> typeDefTable;
extern std::pair<std::string, Cpoint_Type> TypeTemplateCallCodegen;
extern Source_location emptyLoc;

Type* get_type_llvm(Cpoint_Type cpoint_type){
    Type* type;
    if (cpoint_type.is_template_type){
        Log::Info() << "template type found get_type_llvm " << TypeTemplateCallCodegen.first << " -> " << TypeTemplateCallCodegen.second << "\n";
        type = get_type_llvm(TypeTemplateCallCodegen.second);
        goto before_is_ptr;
    }
    if (cpoint_type.type >= 0){
        Log::Info() << "Typedef type used to declare variable (size of typedef table : " << typeDefTable.size() << ")" << "\n";
        if (typeDefTable.size() < cpoint_type.type){
            Log::Info() << "type number " << cpoint_type.type << "\n";
            LogError("couldn't find type from typedef");
        }
        return get_type_llvm(typeDefTable.at(cpoint_type.type));
    }
    //Log::Info() << "cpoint_type.is_struct : " << cpoint_type.is_struct << "\n";
    if (cpoint_type.is_struct){
        Log::Info() << "cpoint_type.struct_name : " << cpoint_type.struct_name << "\n";
        std::string structName = cpoint_type.struct_name;
        if (cpoint_type.is_struct_template){
            structName = get_struct_template_name(cpoint_type.struct_name, *cpoint_type.struct_template_type_passed);
            Log::Info() << "struct type is template : " << structName << "\n";
        }
        if (StructDeclarations[structName] == nullptr){
            Log::Info() << "StructDeclarations[structName] is nullptr" << "\n";
            LogError("Using unknown struct type : %s", structName.c_str());
            return nullptr;
        }
        type = StructDeclarations[structName]->struct_type;
    } else if (cpoint_type.is_union){
        type = UnionDeclarations[cpoint_type.union_name]->union_type;
    } else if (cpoint_type.is_enum){
        type = EnumDeclarations[cpoint_type.enum_name]->enumType; 
    } else {
    switch (cpoint_type.type){
        default:
        case double_type:
            type = Type::getDoubleTy(*TheContext);
            break;
        case i32_type:
//        case int_type:
        case u32_type:
            type = Type::getInt32Ty(*TheContext);
            break;
        case float_type:
            type = Type::getFloatTy(*TheContext);
            break;
        case i8_type:
        case u8_type:
           type = Type::getInt8Ty(*TheContext);
           break;
        case i16_type:
        case u16_type:
            type = Type::getInt16Ty(*TheContext);
            break;
        case i64_type:
        case u64_type:
            type = Type::getInt64Ty(*TheContext);
            break;
        case i128_type:
        case u128_type:
            type = Type::getInt128Ty(*TheContext);
            break;
        case bool_type:
            type = Type::getInt1Ty(*TheContext);
            break;
        case void_type:
            if (!cpoint_type.is_ptr){
            type = Type::getVoidTy(*TheContext);
            } else {
            type = PointerType::get(*TheContext, 0U);
            if (cpoint_type.is_array){
                type = llvm::ArrayType::get(type, cpoint_type.nb_element);
            }
            }
            return type;
        /*case argv_type:
            return Type::getInt8PtrTy(*TheContext)->getPointerTo();*/
    }
    }
before_is_ptr:
    if (cpoint_type.is_ptr){
        type = PointerType::get(*TheContext, 0);
    }
    // code that should not be used except in var creation. The index is not found when doing the codegen
    if (cpoint_type.is_array){
        Log::Info() << "create array type with " << cpoint_type.nb_element << " member of type of " << cpoint_type.type << "\n";
        type = llvm::ArrayType::get(type, cpoint_type.nb_element);
    }
    if (cpoint_type.is_function){
        /*std::vector<Type *> args;
        for (int i = 0; i < cpoint_type.args.size(); i++){
            args.push_back(get_type_llvm(cpoint_type.args.at(i)));
        }
        if (cpoint_type.return_type == nullptr){
            Log::Info() << "cpoint_type.return_type is nullptr" << "\n";
        }
        type = llvm::FunctionType::get(get_type_llvm(*cpoint_type.return_type), args, false);*/
        type = PointerType::get(*TheContext, 0);
    }
    return type;   
}

bool is_llvm_type_number(Type* llvm_type){
    return llvm_type == Type::getDoubleTy(*TheContext) || llvm_type == Type::getInt8Ty(*TheContext) || llvm_type == Type::getInt16Ty(*TheContext) || llvm_type == Type::getInt32Ty(*TheContext) || llvm_type == Type::getInt64Ty(*TheContext) || llvm_type == Type::getInt128Ty(*TheContext);
}

Type* get_array_llvm_type(Type* type, int nb_element){
    return llvm::ArrayType::get(type, nb_element);
}

Cpoint_Type get_cpoint_type_from_llvm(Type* llvm_type){
    int type = double_type;
    bool is_ptr = false;
    bool is_array = false;
    bool is_struct = false;
    bool is_function = false;
    std::string struct_name = "";
    int nb_element = 0;
    if (llvm_type->isPointerTy()){
        is_ptr = true;
    }
    if (llvm_type->isArrayTy()){
        is_array = true;
        nb_element = llvm_type->getArrayNumElements();
        llvm_type = llvm_type->getArrayElementType();
        if (llvm_type->isPointerTy()){
            is_ptr = true;
        }
        goto finding_type;
    }
    // TODO : find enums (how ?)
    if (llvm_type->isStructTy()){
        is_struct = true;
        StructType* struct_type = dyn_cast<StructType>(llvm_type);
        if (struct_type){
            struct_name = (std::string)struct_type->getName();
        }
    }
    if (llvm_type->isFunctionTy()){
        is_function = true;
    }

finding_type:
    if (llvm_type == Type::getDoubleTy(*TheContext)){
        type = double_type;
    } else if (llvm_type == Type::getInt32Ty(*TheContext)){
        type = i32_type;
    } else if (llvm_type == Type::getFloatTy(*TheContext)){
        type = float_type;
    } else if (llvm_type == Type::getInt8Ty(*TheContext)){
        type = i8_type;
    } else if (llvm_type == Type::getInt16Ty(*TheContext)){
        type = i16_type;
    } else if (llvm_type == Type::getInt64Ty(*TheContext)){
        type = i64_type;
    } else if (llvm_type == Type::getInt1Ty(*TheContext)){
        type = bool_type;
    } else if (llvm_type == Type::getVoidTy(*TheContext)){
        type = void_type;
    } else {
        if (is_ptr){
            type = i32_type;
        } else {
        if (!is_struct && !is_array && !is_function){
        Log::Warning(emptyLoc) << "Unknown Type" << "\n";
        }
        }
    }
    int nb_ptr = (is_ptr) ? 1 : 0;
    return Cpoint_Type(type, is_ptr, nb_ptr, is_array, nb_element, is_struct, struct_name, false, "", false, "", false, is_function);
}

Value* get_default_value(Cpoint_Type type){
    return static_cast<Value*>(get_default_constant(type));
}

int Cpoint_Type::get_number_of_bits(){
    switch (type)
    {
//    case int_type:
    case i32_type:
    case u32_type:
        return 32;
    case i8_type:
    case u8_type:
        return 8;
    case i16_type:
    case u16_type:
        return 16;
    case i64_type:
    case u64_type:
        return 64;
    case u128_type:
    case i128_type:
        return 128;
    case double_type:
    case float_type:
        return 64;
    case bool_type:
        return 1;
    default:
        return -1;
    }
}

extern llvm::Triple TripleLLVM;

int get_type_size(Cpoint_Type cpoint_type){
    int size = 0;
    if (cpoint_type.is_struct){
        size += find_struct_type_size(cpoint_type);
    } else if (cpoint_type.is_ptr){
        size += (TripleLLVM.isArch64Bit()) ? 64 : 32;
    } else if (cpoint_type.is_array){
        Cpoint_Type member_array_type = cpoint_type;
        member_array_type.is_array = false;
        size += cpoint_type.nb_element * get_type_size(member_array_type);
        Log::Info() << "find_struct_type_size array type " << cpoint_type << " : " << size << "\n";
    } else {
        size += cpoint_type.get_number_of_bits()/8;
        Log::Info() << "find_struct_type_size normal type " << cpoint_type << " : " << cpoint_type.get_number_of_bits()/8 << "\n";
    }
    return size;
}

int find_struct_type_size(Cpoint_Type cpoint_type){
    int size = 0;
    Log::Info() << "find_struct_type_size cpoint_type : " << cpoint_type << "\n";
    Log::Info() << "find_struct_type_size cpoint_type.is_struct : " << cpoint_type.is_struct << "\n";
    Log::Info() << "find_struct_type_size cpoint_type.struct_name : " << cpoint_type.struct_name << "\n";
    if (!cpoint_type.is_struct){
        return 0;
    }
    for (int i = 0; i < StructDeclarations[cpoint_type.struct_name]->members.size(); i++){
       Cpoint_Type member_type = StructDeclarations[cpoint_type.struct_name]->members.at(i).second;
        size += get_type_size(member_type);
    }
    return size;
}

bool is_just_type(Cpoint_Type type, int type_type){
    return type.type == type_type && !type.is_ptr && !type.is_array && !type.is_struct;
}

int struct_get_number_type(Cpoint_Type cpoint_type, int type){
    int found = 0;
    for (int i = 0; i < StructDeclarations[cpoint_type.struct_name]->members.size(); i++){
        Cpoint_Type member_type = StructDeclarations[cpoint_type.struct_name]->members.at(i).second;
        if (is_just_type(member_type, type)){
            found++;
        }
    }
    return found;
}

bool is_struct_all_type(Cpoint_Type cpoint_type, int type){
    return struct_get_number_type(cpoint_type, type) == StructDeclarations[cpoint_type.struct_name]->members.size();
}

// need to have the same type in every member from struct
VectorType* vector_type_from_struct(Cpoint_Type cpoint_type){
    Cpoint_Type member_type = StructDeclarations[cpoint_type.struct_name]->members.at(0).second;
    return VectorType::get(get_type_llvm(member_type), StructDeclarations[cpoint_type.struct_name]->members.size(), false);
}

extern std::vector<Cpoint_Type> typeDefTable;

bool Cpoint_Type::is_unsigned(){
    if (type >= 0){
        return typeDefTable.at(type).is_unsigned();
    }
    return type == u8_type || type == u16_type || type == u32_type || type == u64_type || type == u128_type;
}

bool Cpoint_Type::is_signed(){
    if (type >= 0){
        return typeDefTable.at(type).is_signed();
    }
    return type == i8_type || type == i16_type || type == i32_type /*|| type == int_type*/ || type == i64_type || type == i128_type || type == bool_type;
    //return !is_unsigned(cpoint_type);
}

bool Cpoint_Type::is_decimal_number_type(){
    return type == double_type || type == float_type;
}

// is a struct, but not a ptr and not an array
bool Cpoint_Type::is_just_struct(){
    return is_struct && !is_ptr && !is_array;
}

Constant* get_default_constant(Cpoint_Type type){
    if (type.is_ptr){
        return ConstantPointerNull::get(PointerType::get(*TheContext, 0));
    }
    if (type.type == void_type){
        return nullptr;
    }
    if (type.type == double_type || type.type == float_type){
        return ConstantFP::get(*TheContext, APFloat(0.0));
    }

    switch (type.type){
        case bool_type:
        case i32_type:
//        case int_type:
        case u32_type:
        case i8_type:
        case u8_type:
        case i16_type:
        case u16_type:
        case i64_type:
        case u64_type:
        case i128_type:
        case u128_type:
            bool type_is_signed = type.is_signed();
            int nb_bits = type.get_number_of_bits();
            return ConstantInt::get(*TheContext, APInt(nb_bits, 0, type_is_signed));
    }
    return ConstantFP::get(*TheContext, APFloat(0.0));
}


int from_val_to_int(Value* val){
    if (dyn_cast<ConstantFP>(val)){
        auto constFP = dyn_cast<ConstantFP>(val);
        int val_int = (int)constFP->getValue().convertToDouble();
        return val_int;
    } else if (dyn_cast<ConstantInt>(val)) {
        auto constInt = dyn_cast<ConstantInt>(val);
        int val_int = (int)constInt->getSExtValue();
        return val_int;
    }
    // TODO : make error because not supported constant/not constant
    return -1;
}

Constant* from_val_to_constant(Value* val, Cpoint_Type type){
    if (dyn_cast<ConstantFP>(val)){
        auto constFP = dyn_cast<ConstantFP>(val);
        // TODO : add more types for variable types
        if (type.is_signed() || type.is_unsigned()){
            int val_int = (int)constFP->getValue().convertToDouble();
            return ConstantInt::get(*TheContext, llvm::APInt(type.get_number_of_bits(), val_int, true));
        }
        if (type.type == double_type || type.type == float_type){
            return constFP;
        }
        if (type.is_ptr){
            int val_int = (int)constFP->getValue().convertToDouble();
            if (val_int == 0){
                return ConstantPointerNull::get(PointerType::get(*TheContext, 0));
            }
        }
    } else if (dyn_cast<ConstantInt>(val)) {
        auto constInt = dyn_cast<ConstantInt>(val);
        if (type.type == double_type || type.type == float_type){
            double val_double = (double)constInt->getSExtValue();
            return ConstantFP::get(*TheContext, APFloat(val_double));
        }
        if (type.is_signed() || type.is_unsigned()){
            return constInt;
        }
    }
    LogError("Unknown type for val to constant");
    return nullptr;
    //return dyn_cast<ConstantFP>(val);
}

Constant* from_val_to_constant_infer(Value* val){
    Type* type = val->getType();
    if (type == get_type_llvm(Cpoint_Type(double_type)) || type == get_type_llvm(Cpoint_Type(float_type))){
        return dyn_cast<ConstantFP>(val);
    } else if (type == get_type_llvm(Cpoint_Type(i32_type)) /*|| type == get_type_llvm(Cpoint_Type(int_type))*/){
        return dyn_cast<ConstantInt>(val);
    }
    return dyn_cast<ConstantFP>(val);
}


// TODO : have multiple version of this function with args : Cpoint_Type and Cpoint_Type, Type* and Type*, Cpoint_type and Type*, just with differences like :
// Cpoint_Type and Type* is just calling the Cpoint_Type and Cpoint_Type one after calling get_cpoint_type_from_llvm on the Type*
// it will make the function more optimised in a lot of cases 
bool convert_to_type(Cpoint_Type typeFrom, Type* typeTo, Value* &val){
    // TODO : typeFrom is detected from a Value* Type so there needs to be another way to detect if it is unsigned because llvm types don't contain them. For example by getting the name of the Value* and searching it in NamedValues
  Cpoint_Type typeTo_cpoint = get_cpoint_type_from_llvm(typeTo);
  Log::Info() << "Creating cast" << "\n";
  Log::Info() << "typeFrom : " << typeFrom << "\n";
  Log::Info() << "typeTo : " << typeTo_cpoint << "\n";
  Log::Info() << "typeTo is ptr : " << typeTo->isPointerTy() << "\n";
  if (typeFrom.is_array && typeTo_cpoint.is_ptr){
    auto zero = llvm::ConstantInt::get(*TheContext, llvm::APInt(64, 0, true));
    Log::Info() << "from array to ptr TEST typeFrom : " << typeFrom << "\n";
    //val = Builder->CreateLoad(get_type_llvm(Cpoint_Type(i32_type, true, 1)), val, "load_gep_ptr");
    val = Builder->CreateGEP(get_type_llvm(typeFrom), val, {zero, zero});
    Log::Info() << "from array to ptr TEST3" << "\n";
    val = Builder->CreateLoad(get_type_llvm(Cpoint_Type(void_type, true, 1)), val);
    return true;
  }
  if (typeFrom.is_array || (typeFrom.is_struct && !typeFrom.is_ptr) || typeTo_cpoint.is_array || (typeTo_cpoint.is_struct && !typeTo_cpoint.is_ptr)){
    return false;
  }
  if (typeFrom.is_ptr && !typeTo_cpoint.is_ptr){
    Log::Info() << "PtrToInt" << "\n";
    val = Builder->CreatePtrToInt(val, typeTo, "ptrtoint_cast");
    return true;
  } 
  if (!typeFrom.is_ptr && typeTo_cpoint.is_ptr){
    if (typeFrom.type == double_type || typeFrom.type ==  float_type){
        val = Builder->CreateFPToUI(val, get_type_llvm(Cpoint_Type(i32_type)), "ui_to_fp_inttoptr");
    }
    val = Builder->CreateIntToPtr(val, typeTo, "inttoptr_cast");
    return true;
  }
  if (typeFrom.is_ptr || typeTo_cpoint.is_ptr){
    return false;
  }

  if (typeFrom.is_decimal_number_type()){
    Log::Info() << "TypeFrom is decimal" << "\n";
    Log::Info() << "typeFrom : " << typeFrom << " " << "typeTo : " << typeTo_cpoint << "\n";
    if (typeTo_cpoint.is_decimal_number_type()){
        if (typeFrom.type == double_type && typeTo_cpoint.type == float_type){
            Log::Info() << "From double to float" << "\n";
            //val = Builder->CreateFPExt(val, typeTo, "cast");
            val = Builder->CreateFPTrunc(val, typeTo, "cast");
            return true;
        } else if (typeFrom.type == float_type && typeTo_cpoint.type == double_type){
            Log::Info() << "From float to double" << "\n";
            //val = Builder->CreateFPTrunc(val, typeTo, "cast");
            val = Builder->CreateFPExt(val, typeTo, "cast");
            return true;
        }
        return false;
    } else if (typeTo_cpoint.is_signed()){
      Log::Info() << "From float/double to signed int" << "\n";
      val = Builder->CreateFPToUI(val, typeTo, "cast"); // change this to signed int conversion. For now it segfaults
      return true;
    } else if (typeTo_cpoint.is_unsigned()){
      Log::Info() << "From float/double to unsigned int" << "\n";
      val = Builder->CreateFPToUI(val, typeTo, "cast");
      return true;
    }
    return false;
  }
  int nb_bits_type_from = -1;
  int nb_bits_type_to = -1;
  if (typeTo == Type::getDoubleTy(*TheContext) || typeTo == Type::getFloatTy(*TheContext)){
        Log::Info() << "is_signed typeFrom.type " << typeFrom.type << " : " << typeFrom.is_signed() << "\n";
        if (typeFrom.is_signed()){
            Log::Info() << "SIToFP\n";
            val = Builder->CreateSIToFP(val, typeTo, "sitofp_cast");
        } else {
            val = Builder->CreateUIToFP(val, typeTo, "uitofp_cast");
        }
        return true;
    } else if ((nb_bits_type_from = typeFrom.get_number_of_bits()) != -1 && nb_bits_type_from != (nb_bits_type_to = typeTo_cpoint.get_number_of_bits())){
        if (nb_bits_type_from < nb_bits_type_to){
            if (typeFrom.is_signed()){
                Log::Info() << "Sext cast" << "\n";
                val = Builder->CreateSExt(val, typeTo, "sext_cast");
            } else {
                Log::Info() << "Zext cast" << "\n";
                val = Builder->CreateZExt(val, typeTo, "zext_cast");
            }
            return true;
        } else {
            Log::Info() << "Trunc cast" << "\n";
            val = Builder->CreateTrunc(val, typeTo, "trunc_cast");
            return true;
        }
    }
    if (typeTo_cpoint.type == typeFrom.type){
        return false; // TODO : maybe return true because technically the types are the same ?
    }
    return false;
}

Cpoint_Type Cpoint_Type::deref_type(){
    Cpoint_Type new_type = *this;
    if (new_type.is_array){
        new_type.is_array = false;
    } else {
        if (new_type.is_ptr){
            new_type.nb_ptr--;
            if (new_type.nb_ptr == 0){
                new_type.is_ptr = false;
            }
        }
    }
    return new_type;
}

std::string Cpoint_Type::to_printf_format(){
    std::string format;
    if (is_ptr){
        if (type == i8_type){
            format = "%s";
        } else {
            format = "%p";
        }
    } else if (is_signed() || is_unsigned()){
        if (type == i8_type || type == u8_type){
            format = "%c";
        } else if (type == i64_type){
            format = "%ld";
        } else if (type == u64_type){
            format = "%lu";
        } else if (is_unsigned()){
            format = "%u";
        } else {
            format = "%d";
        }
    } else if (is_decimal_number_type()) {
        format = "%f";
    } else {
        //return LogErrorV(emptyLoc, "Not Printable type in debug macro");
        return "";
    }
    return format;
}

std::vector<std::string> types_list_start = {
    "double",
//    "int",
    "bool",
    "float",
    "void",
    "i8",
    "i16",
    "i32",
    "i64",
    "i128",
    "u8",
    "u16",
    "u32",
    "u64",
    "u128",
//    "jdhdhghdhdhjbdhjddhhyuuhjdhuudhuhduhduhother", // is just a random string that will never be a type so it will never detect it (TODO : replace with empty string or more random/longer string ?)
//    "bool"
};

std::vector<std::string> types_list = types_list_start;


bool is_type(std::string type){
    /*if (type == "int"){ // TODO : move this to a typedef in a core file
        return true;
    }*/
    for (int i = 0; i < types_list.size(); i++){
       if (type == types_list.at(i)){
	    return true;
       }
    }
    return false;
}

int get_type(std::string type){
    /*if (type == "int"){
        return i32_type-1;
    }*/
    Log::Info() << "types_list_start.size() : " << types_list_start.size() << "\n";
    for (int i = 0; i < types_list.size(); i++){
       if (type == types_list.at(i)){
        if (i >= types_list_start.size()){
            return i-types_list_start.size();
        }
        return -(i + 1);
       }
    }
    return 1;
}

std::string get_string_from_type(Cpoint_Type type){
    if (type.type < 0){
        return types_list.at(-(type.type + 1));
    } else {
        return types_list.at(type.type+types_list_start.size());
    }
}

std::string Cpoint_Type::create_mangled_name(){
    std::string name;
    name = get_string_from_type(*this);
    if (is_ptr){
        name += "_ptr";
        // TODO : maybe use the nb_ptr to mangle differently
    }
    if (is_array){
        name += "_array";
    }
    return name;   
}

std::string create_pretty_name_for_type(Cpoint_Type type){
    if (type.is_struct){
        std::string name =  "struct " + type.struct_name;
        if (type.is_ptr){
            name += " ptr";
        }
        return name;
    }
    std::string name;
    name = get_string_from_type(type);
    if (type.is_ptr){
        name += " ptr";
        if (type.nb_ptr > 1){
            for (int i = 0; i < type.nb_ptr-1; i++){
                name += " ptr";
            }
        }
    }
    if (type.is_array){
        name += " array";
    }
    return name;
}