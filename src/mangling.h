#include <string>
#include "types.h"

std::string struct_function_mangling(std::string struct_name, std::string name);
std::string module_function_mangling(std::string module_name, std::string function_name);
std::string get_struct_template_name(std::string struct_name, Cpoint_Type type);