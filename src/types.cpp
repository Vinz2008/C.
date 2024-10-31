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

#if ENABLE_CIR
#include "CIR/cir.h"
#endif

#include "backends/llvm/llvm.h" // TODO : remove eveything that needs llvm from here in the future

using namespace llvm;

extern std::unique_ptr<LLVMContext> TheContext;
extern std::unique_ptr<IRBuilder<>> Builder;
extern std::unordered_map<std::string, std::unique_ptr<StructDeclaration>> StructDeclarations;
extern std::unordered_map<std::string, std::unique_ptr<UnionDeclaration>> UnionDeclarations;
extern std::unordered_map<std::string, std::unique_ptr<EnumDeclaration>> EnumDeclarations;
std::vector<Cpoint_Type> typeDefTable;
extern std::pair<std::string, Cpoint_Type> TypeTemplateCallCodegen;
extern Source_location emptyLoc;

Type* get_type_llvm(LLVMContext& context, std::unordered_map<std::string, Type*>& struct_declars, Cpoint_Type cpoint_type);

// by default, use the global TheContext (TODO : remove it after finishing CIR)
Type* get_type_llvm(Cpoint_Type cpoint_type){
    auto struct_declars_empty = std::unordered_map<std::string, Type*>();
    return get_type_llvm(*TheContext, struct_declars_empty, cpoint_type);
}

Type* get_type_llvm(std::unique_ptr<LLVM::Context>& llvm_context, Cpoint_Type cpoint_type){
    return get_type_llvm(*llvm_context->TheContext, llvm_context->structDeclars, cpoint_type);
}

Type* get_type_llvm(LLVMContext& context, Cpoint_Type cpoint_type){
    auto struct_declars_empty = std::unordered_map<std::string, Type*>();
    return get_type_llvm(context, struct_declars_empty, cpoint_type);
}

Type* get_type_llvm(LLVMContext& context, std::unordered_map<std::string, Type*>& struct_declars, Cpoint_Type cpoint_type){
    assert(!cpoint_type.is_empty);
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
            LogErrorE("couldn't find type from typedef");
        }
        return get_type_llvm(typeDefTable.at(cpoint_type.type));
    }
    //Log::Info() << "cpoint_type.is_struct : " << cpoint_type.is_struct << "\n";
    if (cpoint_type.is_vector_type){
        type = VectorType::get(get_type_llvm(*cpoint_type.vector_element_type), cpoint_type.vector_size, false); // should it be scalable
    } else if (cpoint_type.is_struct){
        Log::Info() << "cpoint_type.struct_name : " << cpoint_type.struct_name << "\n";
        std::string structName = cpoint_type.struct_name;
        if (cpoint_type.is_object_template){
            structName = get_object_template_name(cpoint_type.struct_name, *cpoint_type.object_template_type_passed);
            Log::Info() << "struct type is template : " << structName << "\n";
        }
        if (StructDeclarations[structName] == nullptr){
            Log::Info() << "StructDeclarations[structName] is nullptr" << "\n";
            LogErrorE("Using unknown struct type : %s", structName.c_str());
            return nullptr;
        }
        if (struct_declars.empty()){ // is not CIR backend
            type = StructDeclarations[structName]->struct_type;
        } else {
            type = struct_declars[structName];
        }
    } else if (cpoint_type.is_union){
        type = UnionDeclarations[cpoint_type.union_name]->union_type;
    } else if (cpoint_type.is_enum){
        std::string enumName = cpoint_type.enum_name;
        if (cpoint_type.is_object_template){
            enumName = get_object_template_name(cpoint_type.enum_name, *cpoint_type.object_template_type_passed);
        }
        type = EnumDeclarations[enumName]->enumType; 
    } else {
    switch (cpoint_type.type){
        default:
        case double_type:
            type = Type::getDoubleTy(context);
            break;
        case i32_type:
//        case int_type:
        case u32_type:
            type = Type::getInt32Ty(context);
            break;
        case float_type:
            type = Type::getFloatTy(context);
            break;
        case i8_type:
        case u8_type:
           type = Type::getInt8Ty(context);
           break;
        case i16_type:
        case u16_type:
            type = Type::getInt16Ty(context);
            break;
        case i64_type:
        case u64_type:
            type = Type::getInt64Ty(context);
            break;
        case i128_type:
        case u128_type:
            type = Type::getInt128Ty(context);
            break;
        case bool_type:
            type = Type::getInt1Ty(context);
            break;
        case void_type:
        case never_type:
            if (!cpoint_type.is_ptr){
            type = Type::getVoidTy(context);
            } else {
            type = PointerType::get(context, 0U);
            if (cpoint_type.is_array){
                type = llvm::ArrayType::get(type, cpoint_type.nb_element);
            }
            }
            return type;
        /*case argv_type:
            return Type::getInt8PtrTy(context)->getPointerTo();*/
    }
    }
before_is_ptr:
    if (cpoint_type.is_ptr){
        type = PointerType::get(context, 0);
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
        type = PointerType::get(context, 0);
    }
    return type;   
}


bool operator==(const Cpoint_Type& lhs, const Cpoint_Type& rhs){
    bool is_object_template_type_passed_same = false;
    if (lhs.object_template_type_passed != nullptr && rhs.object_template_type_passed != nullptr){
        is_object_template_type_passed_same = *lhs.object_template_type_passed == *rhs.object_template_type_passed;
    } else if (lhs.object_template_type_passed == nullptr && rhs.object_template_type_passed == nullptr){
        is_object_template_type_passed_same = true;
    }

    bool is_return_type_same = false;
    if (lhs.return_type != nullptr && rhs.return_type != nullptr){
        is_return_type_same = *lhs.return_type == *rhs.return_type;
    } else if (lhs.return_type == nullptr && rhs.return_type == nullptr){
        is_return_type_same = true;
    }

    bool is_vector_element_type_same = false;
    if (lhs.vector_element_type != nullptr && rhs.vector_element_type != nullptr){
        is_vector_element_type_same = *lhs.vector_element_type == *rhs.vector_element_type;
    } else if (lhs.vector_element_type == nullptr && rhs.vector_element_type == nullptr){
        is_vector_element_type_same = true;
    }

    return lhs.type == rhs.type && lhs.is_ptr == rhs.is_ptr && lhs.nb_ptr == rhs.nb_ptr && lhs.is_array == rhs.is_array && lhs.nb_element == rhs.nb_element && lhs.is_struct == rhs.is_struct && lhs.struct_name == rhs.struct_name && lhs.is_union == rhs.is_union && lhs.union_name == rhs.union_name && lhs.is_enum == rhs.is_enum && lhs.enum_name == rhs.enum_name && lhs.is_template_type == rhs.is_template_type && lhs.is_object_template == rhs.is_object_template && lhs.is_empty == rhs.is_empty && is_object_template_type_passed_same && lhs.is_function == rhs.is_function && std::equal(lhs.args.begin(), lhs.args.end(), rhs.args.begin(), rhs.args.end()) && is_return_type_same && lhs.is_vector_type == rhs.is_vector_type && is_vector_element_type_same && lhs.vector_size == rhs.vector_size;
}

bool operator!=(const Cpoint_Type& lhs, const Cpoint_Type& rhs){
    return !(lhs == rhs);
}

Cpoint_Type get_vector_type(Cpoint_Type vector_element_type, int vector_size){
    return Cpoint_Type(other_type, false, 0, false, 0, false, "", false, "", false, "", false, false, nullptr, false, {}, nullptr, true, (vector_element_type.is_empty) ? nullptr : new Cpoint_Type(vector_element_type), vector_size);
}

bool is_llvm_type_number(Type* llvm_type){
    return llvm_type == get_type_llvm(Cpoint_Type(double_type)) || llvm_type == get_type_llvm(Cpoint_Type(i8_type)) || llvm_type == get_type_llvm(Cpoint_Type(i16_type)) || llvm_type == get_type_llvm(Cpoint_Type(i32_type)) || get_type_llvm(Cpoint_Type(i64_type)) || llvm_type == get_type_llvm(Cpoint_Type(i128_type));
}

// TODO : remove this (not used)
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
    bool is_vector_type = false;
    Cpoint_Type* vector_element_type = nullptr;
    int vector_size = 0;
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

    if (llvm_type->isVectorTy()){
        is_vector_type = true;
        auto vector_type = dyn_cast<VectorType>(llvm_type);
        vector_element_type = new Cpoint_Type(get_cpoint_type_from_llvm(vector_type->getElementType()));
        ElementCount element_count = vector_type->getElementCount();
        vector_size = element_count.getFixedValue();
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
        if (!is_struct && !is_array && !is_function && !is_vector_type){
        (Log::Warning(emptyLoc) << "Unknown Type" << "\n").end();
        }
        }
    }
    int nb_ptr = (is_ptr) ? 1 : 0;
    return Cpoint_Type(type, is_ptr, nb_ptr, is_array, nb_element, is_struct, struct_name, false, "", false, "", false, is_function, nullptr, false, {}, nullptr, is_vector_type, vector_element_type, vector_size);
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
    if (cpoint_type.is_just_struct()){
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
    return type.type == type_type && !type.is_ptr && !type.is_array && !type.is_struct && !type.is_union && !type.is_enum && !type.is_function && !type.is_vector_type;
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

bool Cpoint_Type::is_number_type(){
    return this->is_integer_type() || this->is_decimal_number_type();
}

bool Cpoint_Type::is_integer_type(){
    return this->is_signed() || this->is_unsigned();
}

// is a struct, but not a ptr and not an array
bool Cpoint_Type::is_just_struct(){
    return is_struct && !is_ptr && !is_array;
}

Constant* get_default_constant(Cpoint_Type type){
    return get_default_constant(*TheContext, type);
}

Constant* get_default_constant(LLVMContext& context, Cpoint_Type type){
    if (type.is_ptr){
        return ConstantPointerNull::get(PointerType::get(context, 0));
    }
    if (type.type == void_type){
        return nullptr;
    }
    if (type.type == double_type || type.type == float_type){
        return ConstantFP::get(context, APFloat(0.0));
    }

    if (type.is_vector_type){
        std::vector<Constant*> vectorConstants;
        Constant* constant = get_default_constant(*type.vector_element_type);
        for (int i = 0; i < type.vector_size; i++){
            vectorConstants.push_back(constant);
        }
        return ConstantVector::get(vectorConstants);
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
            return ConstantInt::get(context, APInt(nb_bits, 0, type_is_signed));
    }
    return ConstantFP::get(context, APFloat(0.0));
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
    return from_val_to_constant(*TheContext, val, type);
}

Constant* from_val_to_constant(LLVMContext& context, Value* val, Cpoint_Type type){
    if (dyn_cast<ConstantFP>(val)){
        auto constFP = dyn_cast<ConstantFP>(val);
        // TODO : add more types for variable types
        if (type.is_signed() || type.is_unsigned()){
            int val_int = (int)constFP->getValue().convertToDouble();
            return ConstantInt::get(context, llvm::APInt(type.get_number_of_bits(), val_int, true));
        }
        if (type.type == double_type || type.type == float_type){
            return constFP;
        }
        if (type.is_ptr){
            int val_int = (int)constFP->getValue().convertToDouble();
            if (val_int == 0){
                return ConstantPointerNull::get(PointerType::get(context, 0));
            }
        }
    } else if (dyn_cast<ConstantInt>(val)) {
        auto constInt = dyn_cast<ConstantInt>(val);
        if (type.type == double_type || type.type == float_type){
            double val_double = (double)constInt->getSExtValue();
            return ConstantFP::get(context, APFloat(val_double));
        }
        if (type.is_signed() || type.is_unsigned()){
            return constInt;
        }
    }
    LogErrorE("Unknown type for val to constant");
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

#if ENABLE_CIR

// the val passed is the val to convert, and after the function is the val converted
// if return true, the conversion FAILED (even if the conversion does nothing, it will return true)
bool cir_convert_to_type(std::unique_ptr<FileCIR>& fileCIR, Cpoint_Type typeFrom, Cpoint_Type typeTo, CIR::InstructionRef& val){
    typeFrom = typeFrom.get_real_type();
    typeTo = typeTo.get_real_type();
    if (typeFrom == typeTo){
        return true; // noop if the same type
    }
    if (typeFrom.type >= 0){}
    if (typeTo == Cpoint_Type(void_type)){
        val = fileCIR->add_instruction(std::make_unique<CIR::ConstVoid>());
        return true;
    }
    // TODO : do type checking to verify if the conversion is valid (for example with a bool check_conversion(Cpoint_Type typeFrom, Cpoint_Type typeTo) function) 
    
    auto cast_instr = std::make_unique<CIR::CastInstruction>(val, typeFrom, typeTo);
    //cast_instr->type = typeTo;
    val = fileCIR->add_instruction(std::move(cast_instr));
    return true;
}

#endif

// TODO : make it return false only when there is a problem when converting (when the conversion is a noop, return true instead of false) and verify the return to see if the conversion failed (and if it failed, call LogError)
static bool convert_to_type(std::unique_ptr<llvm::IRBuilder<>>& builder, std::unique_ptr<LLVMContext>& context, Cpoint_Type typeFrom, Cpoint_Type typeTo_cpoint, Value* &val){
  Log::Info() << "Creating cast" << "\n";
  Log::Info() << "typeFrom : " << typeFrom << "\n";
  Log::Info() << "typeTo : " << typeTo_cpoint << "\n";
  if (typeTo_cpoint == Cpoint_Type(void_type)){
    val = nullptr;
    return true;
  }
  if (typeFrom.is_vector_type || typeTo_cpoint.is_vector_type){
    if (!typeFrom.is_vector_type){
        LogErrorE("Trying to cast something that is not of a vector type to a vector");
    }
    // automatically transforms a vector of bool to a bool
    if (typeTo_cpoint.type == bool_type && typeFrom.vector_element_type && typeFrom.vector_element_type->type == bool_type){
        // TODO : optimize this -> if the vector can be bitcast to a scalar type (for example a vector of 8 i8 which can be casted to an an i64), cast it to the scalar type and compare it to -1 (the minus is because when casting if everything is true, the sign bit will be 1)
        val = builder->CreateAndReduce(val);
        return true;
    }
    if (!typeTo_cpoint.is_vector_type){
        LogErrorE("Trying to cast something that is of vector type to a not vector type");
    }
    if (typeFrom.vector_size != typeTo_cpoint.vector_size){
        LogErrorE("Trying to cast to a vector type with a different size");
    }
    return true;
  }
  //Log::Info() << "typeTo is ptr : " << typeTo->isPointerTy() << "\n";
  if (typeFrom.is_array && typeTo_cpoint.is_ptr){
    auto zero = llvm::ConstantInt::get(*context, llvm::APInt(64, 0, true));
    Log::Info() << "from array to ptr TEST typeFrom : " << typeFrom << "\n";
    //val = Builder->CreateLoad(get_type_llvm(Cpoint_Type(i32_type, true, 1)), val, "load_gep_ptr");
    val = builder->CreateGEP(get_type_llvm(*context, typeFrom), val, {zero, zero});
    Log::Info() << "from array to ptr TEST3" << "\n";
    val = builder->CreateLoad(get_type_llvm(*context, Cpoint_Type(void_type, true, 1)), val);
    return true;
  }
  if (typeFrom.is_array || (typeFrom.is_struct && !typeFrom.is_ptr) || typeTo_cpoint.is_array || (typeTo_cpoint.is_struct && !typeTo_cpoint.is_ptr)){
    return false;
  }
  if (typeFrom.is_ptr && !typeTo_cpoint.is_ptr){
    Log::Info() << "PtrToInt" << "\n";
    val = builder->CreatePtrToInt(val, get_type_llvm(*context, typeTo_cpoint), "ptrtoint_cast");
    return true;
  } 
  if (!typeFrom.is_ptr && typeTo_cpoint.is_ptr){
    if (typeFrom.type == double_type || typeFrom.type ==  float_type){
        val = builder->CreateFPToUI(val, get_type_llvm(*context, Cpoint_Type(i32_type)), "ui_to_fp_inttoptr");
    }
    val = builder->CreateIntToPtr(val, get_type_llvm(*context, typeTo_cpoint), "inttoptr_cast");
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
            val = builder->CreateFPTrunc(val, get_type_llvm(*context, typeTo_cpoint), "cast");
            return true;
        } else if (typeFrom.type == float_type && typeTo_cpoint.type == double_type){
            Log::Info() << "From float to double" << "\n";
            //val = Builder->CreateFPTrunc(val, typeTo, "cast");
            val = builder->CreateFPExt(val, get_type_llvm(*context, typeTo_cpoint), "cast");
            return true;
        }
        return false;
    } else if (typeTo_cpoint.is_signed()){
      Log::Info() << "From float/double to signed int" << "\n";
      val = builder->CreateFPToSI(val, get_type_llvm(*context, typeTo_cpoint), "cast"); // change this to signed int conversion. For now it segfaults
      return true;
    } else if (typeTo_cpoint.is_unsigned()){
      Log::Info() << "From float/double to unsigned int" << "\n";
      val = builder->CreateFPToUI(val, get_type_llvm(*context, typeTo_cpoint), "cast");
      return true;
    }
    return false;
  }
  int nb_bits_type_from = -1;
  int nb_bits_type_to = -1;
  if (typeTo_cpoint.type == double_type || typeTo_cpoint.type == float_type){
        Log::Info() << "is_signed typeFrom.type " << typeFrom.type << " : " << typeFrom.is_signed() << "\n";
        if (typeFrom.is_signed()){
            Log::Info() << "SIToFP\n";
            val = builder->CreateSIToFP(val, get_type_llvm(*context, typeTo_cpoint), "sitofp_cast");
        } else {
            val = builder->CreateUIToFP(val, get_type_llvm(*context, typeTo_cpoint), "uitofp_cast");
        }
        return true;
    } else if ((nb_bits_type_from = typeFrom.get_number_of_bits()) != -1 && nb_bits_type_from != (nb_bits_type_to = typeTo_cpoint.get_number_of_bits())){
        if (nb_bits_type_from < nb_bits_type_to){
            if (typeFrom.is_signed()){
                Log::Info() << "Sext cast" << "\n";
                val = builder->CreateSExt(val, get_type_llvm(*context, typeTo_cpoint), "sext_cast");
            } else {
                Log::Info() << "Zext cast" << "\n";
                val = builder->CreateZExt(val, get_type_llvm(*context, typeTo_cpoint), "zext_cast");
            }
            return true;
        } else {
            Log::Info() << "Trunc cast" << "\n";
            val = builder->CreateTrunc(val, get_type_llvm(*context, typeTo_cpoint), "trunc_cast");
            return true;
        }
    }
    if (typeTo_cpoint.type == typeFrom.type){
        return false; // TODO : maybe return true because technically the types are the same ?
    }
    return false;
}

bool convert_to_type(Cpoint_Type typeFrom, Cpoint_Type typeTo_cpoint, Value* &val){
    return convert_to_type(Builder, TheContext, typeFrom, typeTo_cpoint, val);
}

bool convert_to_type(std::unique_ptr<LLVM::Context>& llvm_context, Cpoint_Type typeFrom, Cpoint_Type typeTo_cpoint, Value* &val){
    return convert_to_type(llvm_context->Builder, llvm_context->TheContext, typeFrom, typeTo_cpoint, val);
}

bool convert_to_type(Cpoint_Type typeFrom, Type* typeTo, Value* &val){
    Cpoint_Type typeTo_cpoint = get_cpoint_type_from_llvm(typeTo);
    return convert_to_type(typeFrom, typeTo_cpoint, val);
}

Cpoint_Type Cpoint_Type::deref_type(){
    Cpoint_Type new_type = *this;
    if (new_type.is_array){
        new_type.is_array = false;
    } else if (new_type.is_vector_type){
        /*new_type.is_vector_type = false;
        new_type.vector_size = 0;
        new_type.vector_element_type = nullptr;*/
        new_type = *new_type.vector_element_type;
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

Cpoint_Type Cpoint_Type::get_real_type(){
    if (is_template_type){
        Cpoint_Type template_type = TypeTemplateCallCodegen.second;
        if (is_ptr){
            template_type.is_ptr = true;
            template_type.nb_ptr++;
        }
        // TODO : add more modifiers to the type (ex : is an array)
        return template_type;
   }  else if (type >= 0 && typeDefTable.size() >= type){
        return typeDefTable.at(type); // TODO : add modifiers
    } else {
        return *this;
    }
}

std::string Cpoint_Type::to_printf_format(){
    std::string format;
    // TODO : maybe move this to a Print trait like in rust
    if (is_vector_type){
        format = "{";
        for (int i = 0; i < vector_size; i++){
            format += " " + vector_element_type->to_printf_format() + "";
        }
        format += "}";
    } else if (is_ptr){
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
        LogErrorV(emptyLoc, "Not Printable type in debug macro");
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

static std::string get_string_from_type(Cpoint_Type type){
    assert(!type.is_empty);
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
    std::string name = "";
    if (type.is_struct){
        name =  "struct " + type.struct_name;
        if (type.is_ptr){
            name += " ptr";
        }
        return name;
    }
    if (type.type == never_type){
        name = "!";
    } else {
        name = get_string_from_type(type);
    }
    if (type.is_ptr){
        name += " ptr";
        if (type.nb_ptr > 1){
            for (int i = 0; i < type.nb_ptr-1; i++){
                name += " ptr";
            }
        }
    }
    if (type.is_array){
        name += "[" + std::to_string(type.nb_element) + "]";
    }
    return name;
}