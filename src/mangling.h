#include <string>
#include "types.h"

std::string struct_function_mangling(std::string struct_name, std::string name);
std::string module_mangling(std::string module_name, std::string function_name);
std::string get_object_template_name(std::string struct_name, Cpoint_Type type);
bool is_object_template(std::string struct_name);
std::string remove_struct_template(std::string struct_name);