#include "mangling.h"

std::string struct_function_mangling(std::string struct_name, std::string name){
  std::string mangled_name = struct_name + "__" + name;
  return mangled_name;
}

std::string module_function_mangling(std::string module_name, std::string function_name){
  std::string mangled_name = module_name + "___" + function_name;
  return mangled_name;
}

// get template name of struct or enum
std::string get_object_template_name(std::string struct_name, /*std::string*/ Cpoint_Type type){
    //return struct_name + "____" + type;
    return struct_name + "____" + type.create_mangled_name();
}

bool is_object_template(std::string struct_name){
    return struct_name.find("____") != std::string::npos;
}

std::string remove_struct_template(std::string struct_name){
    assert(is_object_template(struct_name));
    return struct_name.substr(0, struct_name.find("____"));
}