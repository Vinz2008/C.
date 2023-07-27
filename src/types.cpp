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
//extern std::map<std::string, std::unique_ptr<ClassDeclaration>> ClassDeclarations;
std::vector<std::string> typeDefTable;
extern std::pair<std::string, std::string> TypeTemplateCallCodegen;

Type* get_type_llvm(Cpoint_Type cpoint_type){
    Type* type;
    if (cpoint_type.is_template_type){
        Log::Info() << "template type found get_type_llvm " << TypeTemplateCallCodegen.first << " -> " << TypeTemplateCallCodegen.second << "\n";
        return get_type_llvm(get_type(TypeTemplateCallCodegen.second));
    }
    if (cpoint_type.type >= 0){
        Log::Info() << "Typedef type used to declare variable (size of typedef table : " << typeDefTable.size() << ")" << "\n";
        if (typeDefTable.size() < cpoint_type.type){
            Log::Info() << "type number " << cpoint_type.type << "\n";
            LogError("couldn't find type from typedef");
        }
        return get_type_llvm(get_type(typeDefTable.at(cpoint_type.type)));
    }
    Log::Info() << "cpoint_type.is_struct : " << cpoint_type.is_struct << "\n";
    if (cpoint_type.is_struct){
        Log::Info() << "cpoint_type.struct_name : " << cpoint_type.struct_name << "\n";
        std::string structName = cpoint_type.struct_name;
        if (cpoint_type.is_struct_template){
            structName = get_struct_template_name(cpoint_type.struct_name, cpoint_type.struct_template_name);
            Log::Info() << "struct type is template : " << structName << "\n";
        }
        if (StructDeclarations[structName] == nullptr){
            Log::Info() << "StructDeclarations[structName] is nullptr" << "\n";
        }
        type = StructDeclarations[structName]->struct_type;
    } else if (cpoint_type.is_union){
        type = UnionDeclarations[cpoint_type.union_name]->union_type;
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
    Log::Info() << "TEST before is function" << "\n";
    if (cpoint_type.is_function){
        Log::Info() << "TEST in function" << "\n";
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
     Log::Info() << "TEST after is function" << "\n";
    return type;   
}

bool is_llvm_type_number(Type* llvm_type){
    return llvm_type == Type::getDoubleTy(*TheContext) || llvm_type == Type::getInt16Ty(*TheContext) || llvm_type == Type::getInt32Ty(*TheContext) || llvm_type == Type::getInt64Ty(*TheContext);
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
    //Type* not_ptr_type = llvm_type;
    if (llvm_type->isPointerTy()){
        is_ptr = true;
    }
    if (llvm_type->isArrayTy()){
        is_array = true;
    }
    if (llvm_type->isStructTy()){
        is_struct = true;
    }
    if (llvm_type->isFunctionTy()){
        is_function = true;
    }
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
    } else {
        if (is_ptr){
            type = i8_type;
        } else {
        Log::Warning() << "Unknown Type" << "\n";
        }
    }
    int nb_ptr = (is_ptr) ? 1 : 0;
    return Cpoint_Type(type, is_ptr, nb_ptr, is_array, 0, is_struct, "", false, "", false, is_function);
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
        return 64;
    default:
        return -1;
    }
}

bool is_unsigned(Cpoint_Type cpoint_type){
    int type = cpoint_type.type;
    return type == u8_type || type == u16_type || type == u32_type || type == u64_type || type == u128_type;
}

bool is_signed(Cpoint_Type type){
    return !is_unsigned(type);
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

void convert_to_type(Cpoint_Type typeFrom, Type* typeTo, Value* &val){
    // TODO : typeFrom is detected from a Value* Type so there needs to be another way to detect if it is unsigned because llvm types don't contain them. For example by getting the name of the Value* and searching it in NamedValues
  Cpoint_Type typeTo_cpoint = get_cpoint_type_from_llvm(typeTo);
  Log::Info() << "Creating cast" << "\n";
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
    if (is_signed(typeTo_cpoint)){
      Log::Info() << "From float/double to signed int" << "\n";
      val = Builder->CreateFPToUI(val, typeTo, "cast"); // change this to signed int conversion. For now it segfaults
      return;
    } else if (is_unsigned(typeTo_cpoint)){
      Log::Info() << "From float/double to unsigned int" << "\n";
      val = Builder->CreateFPToUI(val, typeTo, "cast");
      return;
    } else if (is_decimal_number_type(typeTo_cpoint)){
      return;
    }
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
    "u128"
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
        if (i >= 14){
            return i+1-15;
        }
        return -(i + 1);
       }
    }
    return 1;
}

std::string get_string_from_type(Cpoint_Type type){
    if (type.type < 0){
        return types.at(-(type.type-1));
    } else {
        return types.at(type.type-1+15);
    }
}