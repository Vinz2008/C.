#include <string>
//#include "types.h"
#include "llvm/IR/Value.h"

using namespace llvm;

class Cpoint_Type;

bool is_var_local(std::string name);
bool is_var_global(std::string name);

bool var_exists(std::string name);
Cpoint_Type* get_variable_type(std::string name);
Value* get_var_allocation(std::string name);