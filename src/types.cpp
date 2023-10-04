#include "types.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Value.h"
#include "codegen.h"
#include "ast.h"
#include "log.h"
#include "lexer.h"
#include <map>

using namespace llvm;

extern std::unique_ptr<LLVMContext> TheContext;
extern std::unique_ptr<IRBuilder<>> Builder;
extern std::map<std::string, std::unique_ptr<StructDeclaration>> StructDeclarations;
extern std::map<std::string, std::unique_ptr<UnionDeclaration>> UnionDeclarations;
extern std::map<std::string, std::unique_ptr<EnumDeclaration>> EnumDeclarations;
//extern std::map<std::string, std::unique_ptr<ClassDeclaration>> ClassDeclarations;
std::vector</*std::string*/ Cpoint_Type> typeDefTable;
extern std::pair<std::string, /*std::string*/ Cpoint_Type> TypeTemplateCallCodegen;
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
        case int_type:
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
        case argv_type:
            return Type::getInt8PtrTy(*TheContext)->getPointerTo();
    }
    }
before_is_ptr:
    if (cpoint_type.is_ptr){
        /*for (int i = 0; i < cpoint_type.nb_ptr; i++){
        type = type->getPointerTo();
        }*/
        type = PointerType::get(*TheContext, 0);
    }
    // code that should not be used except in var creation. The index is not found when doing the codegen
    if (cpoint_type.is_array){
        Log::Info() << "create array type with " << cpoint_type.nb_element << " member of type of " << cpoint_type.type << "\n";
        type = llvm::ArrayType::get(type, cpoint_type.nb_element);
    }
    //Log::Info() << "TEST before is function" << "\n";
    if (cpoint_type.is_function){
        //Log::Info() << "TEST in function" << "\n";
        /*std::vector<Type *> args;
        for (int i = 0; i < cpoint_type.args.size(); i++){
            args.push_back(get_type_llvm(cpoint_type.args.at(i)));
        }
        Log::Info() << "Generating function type" << "\n";
        if (cpoint_type.return_type == nullptr){
            Log::Info() << "cpoint_type.return_type is nullptr" << "\n";
        }
        Log::Info() << "Cpoint type : " << *cpoint_type.return_type << "\n";
        type = llvm::FunctionType::get(get_type_llvm(*cpoint_type.return_type), args, false);*/
        type = PointerType::get(*TheContext, 0);
    }
    //Log::Info() << "TEST after is function" << "\n";
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
    //Type* not_ptr_type = llvm_type;
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
        type = int_type;
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
            type = int_type;
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

int get_type_number_of_bits(Cpoint_Type type){
    switch (type.type)
    {
    case int_type:
    case i32_type:
    case u32_type:
        return 32;
        break;
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

bool is_unsigned(Cpoint_Type cpoint_type){
    int type = cpoint_type.type;
    return type == u8_type || type == u16_type || type == u32_type || type == u64_type || type == u128_type;
}

bool is_signed(Cpoint_Type cpoint_type){
    int type = cpoint_type.type;
    return type == i8_type || type == i16_type || type == i32_type || type == int_type || type == i64_type || type == i128_type || type == bool_type;
    //return !is_unsigned(cpoint_type);
}

bool is_decimal_number_type(Cpoint_Type type){
    return type.type == double_type || type.type == float_type;
}

Constant* get_default_constant(Cpoint_Type type){
    if (type.is_ptr){
        return ConstantPointerNull::get(PointerType::get(*TheContext, 0));
    }
    if (type.type == double_type){
        return ConstantFP::get(*TheContext, APFloat(0.0));
    }

    switch (type.type){
        case bool_type:
        case i32_type:
        case int_type:
        case u32_type:
        case i8_type:
        case u8_type:
        case i16_type:
        case u16_type:
        case i64_type:
        case u64_type:
        case i128_type:
        case u128_type:
            bool type_is_signed = is_signed(type);
            int nb_bits = get_type_number_of_bits(type);
            return ConstantInt::get(*TheContext, APInt(nb_bits, 0, type_is_signed));
    }
    return ConstantFP::get(*TheContext, APFloat(0.0));
}

Constant* from_val_to_constant(Value* val, Cpoint_Type type){
    Type* val_type = val->getType();
    /*if (val_type != get_type_llvm(Cpoint_Type(double_type))){
        return nullptr;
    }*/
    if (dyn_cast<ConstantFP>(val)){
        auto constFP = dyn_cast<ConstantFP>(val);
        // TODO : add more types for variable types
        if (type == Cpoint_Type(int_type)){
            int val_int = (int)constFP->getValue().convertToDouble();
            return ConstantInt::get(*TheContext, llvm::APInt(32, val_int, true));
        }
        if (type == Cpoint_Type(double_type)){
            return constFP;
        }
    } else if (dyn_cast<ConstantInt>(val)) {
        auto constInt = dyn_cast<ConstantInt>(val);
        if (type == Cpoint_Type(double_type)){
            double val_double = (double)constInt->getSExtValue();
            return ConstantFP::get(*TheContext, APFloat(val_double));
        }
        if (type == Cpoint_Type(int_type)){
            return constInt;
        }
    }

    /*
    if (type == Cpoint_Type(double_type) || type == Cpoint_Type(float_type)){
        Log::Info() << "dyn_cast fp" << "\n";
        return dyn_cast<ConstantFP>(val);
    } else if (type == Cpoint_Type(int_type)){
        if (val_type == get_type_llvm(Cpoint_Type(double_type))){

        }
        Log::Info() << "dyn_cast int" << "\n";
        return dyn_cast<ConstantInt>(val);
    }
    std::cout << "dyn_cast default fp" << std::endl;*/
    return dyn_cast<ConstantFP>(val);
}

Constant* from_val_to_constant_infer(Value* val){
    Type* type = val->getType();
    if (type == get_type_llvm(Cpoint_Type(double_type)) || type == get_type_llvm(Cpoint_Type(float_type))){
        return dyn_cast<ConstantFP>(val);
    } else if (type == get_type_llvm(Cpoint_Type(int_type))){
        return dyn_cast<ConstantInt>(val);
    }
    return dyn_cast<ConstantFP>(val);
}

void convert_to_type(Cpoint_Type typeFrom, Type* typeTo, Value* &val){
    // TODO : typeFrom is detected from a Value* Type so there needs to be another way to detect if it is unsigned because llvm types don't contain them. For example by getting the name of the Value* and searching it in NamedValues
  Cpoint_Type typeTo_cpoint = get_cpoint_type_from_llvm(typeTo);
  Log::Info() << "Creating cast" << "\n";
  if (typeFrom.is_array && typeTo_cpoint.is_ptr){
    auto zero = llvm::ConstantInt::get(*TheContext, llvm::APInt(32, 0, true));
    Log::Info() << "from array to ptr TEST typeFrom : " << typeFrom << "\n";
    val = Builder->CreateLoad(get_type_llvm(Cpoint_Type(int_type, true, 1)), val, "load_gep_ptr");
    val = Builder->CreateGEP(get_type_llvm(typeFrom), val, {zero, zero});
    Log::Info() << "from array to ptr TEST3" << "\n";
    val = Builder->CreateLoad(get_type_llvm(Cpoint_Type(void_type, true, 1)), val);
    return;
  }
  if (typeFrom.is_array || typeFrom.is_struct || typeTo_cpoint.is_array || typeTo_cpoint.is_struct){
    return;
  }
  if (typeFrom.is_ptr && !typeTo_cpoint.is_ptr){
    val = Builder->CreatePtrToInt(val, typeTo, "ptrtoint_cast");
    return;
  } 
  if (!typeFrom.is_ptr && typeTo_cpoint.is_ptr){
    val = Builder->CreateIntToPtr(val, typeTo, "inttoptr_cast"); // TODO : test to readdd them
    return;
  }
  if (typeFrom.is_ptr || typeTo_cpoint.is_ptr){
    return;
  }

  if (is_decimal_number_type(typeFrom)){
    Log::Info() << "TypeFrom is decimal" << "\n";
    Log::Info() << "typeFrom : " << typeFrom << " " << "typeTo : " << typeTo_cpoint << "\n";
    if (is_decimal_number_type(typeTo_cpoint)){
        if (typeFrom.type == double_type && typeTo_cpoint.type == float_type){
            Log::Info() << "From double to float" << "\n";
            //val = Builder->CreateFPExt(val, typeTo, "cast");
            val = Builder->CreateFPTrunc(val, typeTo, "cast");
            return;
        } else if (typeFrom.type == float_type && typeTo_cpoint.type == double_type){
            Log::Info() << "From float to double" << "\n";
            //val = Builder->CreateFPTrunc(val, typeTo, "cast");
            val = Builder->CreateFPExt(val, typeTo, "cast");
            return;
        }
        return;
    } else if (is_signed(typeTo_cpoint)){
      Log::Info() << "From float/double to signed int" << "\n";
      val = Builder->CreateFPToUI(val, typeTo, "cast"); // change this to signed int conversion. For now it segfaults
      return;
    } else if (is_unsigned(typeTo_cpoint)){
      Log::Info() << "From float/double to unsigned int" << "\n";
      val = Builder->CreateFPToUI(val, typeTo, "cast");
      return;
    }
    return;
  }
  int nb_bits_type_from = -1;
  int nb_bits_type_to = -1;
  if (typeTo == Type::getDoubleTy(*TheContext) || typeTo == Type::getFloatTy(*TheContext)){
        Log::Info() << "is_signed typeFrom.type " << typeFrom.type << " : " << is_signed(typeFrom.type) << "\n";
        if (is_signed(typeFrom.type)){
            val = Builder->CreateSIToFP(val, typeTo, "sitofp_cast");
        } else {
            val = Builder->CreateUIToFP(val, typeTo, "uitofp_cast");
        }
        return;
    } else if ((nb_bits_type_from = get_type_number_of_bits(typeFrom)) != -1 && nb_bits_type_from != (nb_bits_type_to = get_type_number_of_bits(typeTo_cpoint))){
        if (nb_bits_type_from < nb_bits_type_to){
            if (is_signed(typeFrom.type)){
                Log::Info() << "Sext cast" << "\n";
                val = Builder->CreateSExt(val, typeTo, "sext_cast");
            } else {
                Log::Info() << "Zext cast" << "\n";
                val = Builder->CreateZExt(val, typeTo, "zext_cast");
            }
            return;
        } else {
            Log::Info() << "Trunc cast" << "\n";
            val = Builder->CreateTrunc(val, typeTo, "trunc_cast");
            return;
        }
    } else if (typeTo_cpoint.type == typeFrom.type){
        return;
    }


  /*switch (typeFrom.type)
  {
  case int_type:
    if (typeTo == Type::getDoubleTy(*TheContext) || typeTo == Type::getFloatTy(*TheContext)){
      Log::Info() << "From int to float/double" << "\n";
      Log::Info() << "typeFrom " << typeFrom.type << "\n";
      val = Builder->CreateSIToFP(val, typeTo, "cast");
    } else if (typeTo == Type::getInt32Ty(*TheContext)){
      break;
    }
    break;
  case float_type:
  case double_type:
    if (typeTo == Type::getInt32Ty(*TheContext)){
      Log::Info() << "From float/double to int" << "\n";
      val = Builder->CreateFPToUI(val, typeTo, "cast");
    } else if (typeTo == Type::getDoubleTy(*TheContext) || typeTo == Type::getFloatTy(*TheContext)){
      break;
    }
    break;
  default:
    break;
  }*/
}


std::vector<std::string> types{
    "double",
    "int",
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
    "bool"
};


bool is_type(std::string type){
    for (int i = 0; i < types.size(); i++){
       if (type.compare(types.at(i)) == 0){
	    return true;
       }
    }
    return false;
}

int get_type(std::string type){
    for (int i = 0; i < types.size(); i++){
       if (type == types.at(i)){
        if (i >= 15){
            return i+1-16;
        }
        return -(i + 1);
       }
    }
    return 1;
}

std::string get_string_from_type(Cpoint_Type type){
    if (type.type < 0){
        return types.at(-(type.type + 1));
    } else {
        return types.at(type.type-1+16);
    }
}


std::string create_mangled_name_from_type(Cpoint_Type type){
    std::string name;
    name = get_string_from_type(type);
    if (type.is_ptr){
        name += "_ptr";
        // TODO : maybe use the nb_ptr to mangle differently
    }
    if (type.is_array){
        name += "_array";
    }
    //if (type)
    return name;   
}

std::string create_pretty_name_for_type(Cpoint_Type type){
    if (type.is_struct){
        return "struct " + type.struct_name;
    }
    std::string name;
    name = get_string_from_type(type);
    if (type.is_ptr){
        name += " ptr";
        // TODO : maybe use the nb_ptr to mangle differently
    }
    if (type.is_array){
        name += " array";
    }
    return name;
}