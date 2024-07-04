#include <string>
#include <vector>
#include <memory>
#include <algorithm>
#include <unordered_map>
#include "codegen.h"
#include "log.h"

extern std::vector<std::unique_ptr<TemplateStructCreation>> StructTemplatesToGenerate;
extern std::vector<std::unique_ptr<TemplateCall>> TemplatesToGenerate;
extern std::unordered_map<std::string, std::unique_ptr<TemplateProto>> TemplateProtos;
extern std::pair<std::string, /*std::string*/ Cpoint_Type> TypeTemplateCallCodegen;
bool is_in_struct_templates_codegen = false;

// TODO : add this declaration to a header file
void add_manually_extern(std::string fnName, Cpoint_Type cpoint_type, std::vector<std::pair<std::string, Cpoint_Type>> ArgNames, unsigned Kind, unsigned BinaryPrecedence, bool is_variable_number_args, bool has_template, std::string TemplateName);

// TODO : maybe change name from templates to generics
void codegenTemplates(){
  StructTemplatesToGenerate.clear(); // TODO : because StructTemplatesToGenerate is populated when parsing and the codegen of templates is done at the end, we can't know what the StructTemplatesToGenerate for this function is. Need to create a vector or a map of StructTemplatesToGenerate for each templated functions
  for (int i = 0; i < TemplatesToGenerate.size(); i++){
    Log::Info() << "setting TypeTemplateCallCodegen" << "\n";
    TypeTemplateCallCodegen = std::make_pair(TemplatesToGenerate.at(i)->functionAST->Proto->template_name, TemplatesToGenerate.at(i)->type);
    TemplatesToGenerate.at(i)->functionAST->codegen();
  }
}

void callTemplate(std::string& Callee, /*std::string*/ Cpoint_Type template_passed_type){
  Log::Info() << "Callee : " << Callee << "\n";
  auto templateProto = std::make_unique<TemplateProto>(TemplateProtos[Callee]->functionAST->clone(), TemplateProtos[Callee]->template_type_name); 
  std::string typeName = "____type";
  typeName = "____" + template_passed_type.create_mangled_name();
  std::string function_temp_name = templateProto->functionAST->Proto->Name + typeName; // add something to specify type
  templateProto->functionAST->Proto->Name = function_temp_name;
  TypeTemplateCallCodegen = std::make_pair(templateProto->functionAST->Proto->template_name, template_passed_type);
  add_manually_extern(templateProto->functionAST->Proto->Name, templateProto->functionAST->Proto->cpoint_type, templateProto->functionAST->Proto->Args, (templateProto->functionAST->Proto->IsOperator) ? 1 : 0, templateProto->functionAST->Proto->getBinaryPrecedence(), templateProto->functionAST->Proto->is_variable_number_args, templateProto->functionAST->Proto->has_template, templateProto->functionAST->Proto->template_name);
  Callee = function_temp_name;
  TemplatesToGenerate.push_back(std::make_unique<TemplateCall>(template_passed_type, std::move(templateProto->functionAST)));
}

std::vector<std::pair<std::string, Cpoint_Type>> codegenedStructTemplates;


// TODO : fix functions in templated structs
extern std::unordered_map<std::string, std::unique_ptr<StructDeclaration>> StructDeclarations;

void codegenStructTemplates(){
    Log::Info() << "CODEGEN STRUCT TEMPLATES (nb : "  << StructTemplatesToGenerate.size() << ")" << "\n";
    for (int i = 0; i < StructTemplatesToGenerate.size(); i++){
        TypeTemplateCallCodegen = std::make_pair(StructTemplatesToGenerate.at(i)->structDeclarAST->template_name, StructTemplatesToGenerate.at(i)->type);
        /*std::string templateStructName = StructTemplatesToGenerate.at(i)->structDeclarAST->Name;
        std::string structName = templateStructName.substr(0, templateStructName.find("____"));
        Log::Info() << "StructDeclarations[structName]->structType : " << StructDeclarations[structName]->struct_type << "\n";
        StructDeclarations[structName] = StructDeclarations[structName]->clone();*:
        Log::Info()  << "StructTemplatesToGenerate.at(" << i << ")->structDeclarAST->Vars.size() : " << StructTemplatesToGenerate.at(i)->structDeclarAST->Vars.size() << "\n";
        /*for (int j = 0; j < StructTemplatesToGenerate.at(i)->structDeclarAST->Vars.size(); j++){
            Log::Info() << "Struct Named " << StructTemplatesToGenerate.at(i)->structDeclarAST->Name << "\n";
            if (StructTemplatesToGenerate.at(i)->structDeclarAST->Vars.at(j) == nullptr){
                Log::Info() << "StructTemplatesToGenerate.at(" << i << ")->structDeclarAST->Vars.at(" << j <<") is nullptr" << "\n";
            }
            //Log::Info() << "StructTemplatesToGenerate.at(" << i << ") : " << StructTemplatesToGenerate.at(i)->structDeclarAST->Vars.at(j)->VarNames.at(0).first << "\n";
        }*/
        bool find_in_codegened_templates = std::find(codegenedStructTemplates.begin(), codegenedStructTemplates.end(), TypeTemplateCallCodegen) != codegenedStructTemplates.end();
        if (!find_in_codegened_templates){
            is_in_struct_templates_codegen = true;
            Log::Info() << "Generating Struct " << StructTemplatesToGenerate.at(i)->structDeclarAST->Name << "\n";
            codegenedStructTemplates.push_back(TypeTemplateCallCodegen);
            StructTemplatesToGenerate.at(i)->structDeclarAST->codegen();
            is_in_struct_templates_codegen = false;
        }
    }
}