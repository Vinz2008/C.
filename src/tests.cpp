#include "ast.h"

std::vector<std::unique_ptr<TestAST>> testASTNodes;
extern std::string first_filename;

void generateTests(){
  if (!Comp_context->test_mode){
    return;
  }
  std::vector<std::pair<std::string,Cpoint_Type>> Args;
  Args.push_back(std::make_pair("argc", Cpoint_Type(i32_type)));
  Args.push_back(std::make_pair("argv",  Cpoint_Type(i8_type, true, 2)));
  auto Proto = std::make_unique<PrototypeAST>(emptyLoc, "main", Args, Cpoint_Type(double_type));
  std::vector<std::unique_ptr<ExprAST>> Body;
  if (/*std_mode &&*/ Comp_context->gc_mode){
    generate_gc_init(Body);
  }
  Log::Info() << "testASTNodes size : " << testASTNodes.size() << "\n";
  std::unique_ptr<ExprAST> printfCall;
  std::vector<std::unique_ptr<ExprAST>> ArgsPrintf;
  for (int i = 0; i < testASTNodes.size(); i++){
    // create print for test description
    ArgsPrintf.clear();
    std::string temp_description = testASTNodes.at(i)->description;
    std::unique_ptr<ExprAST> strExprAST;
    strExprAST = std::make_unique<StringExprAST>("Test " + temp_description + " running (in " + first_filename + ")\n");
    ArgsPrintf.push_back(std::move(strExprAST));
    printfCall = std::make_unique<CallExprAST>(emptyLoc, "printf", std::move(ArgsPrintf), Cpoint_Type(double_type));
    Body.push_back(std::move(printfCall));
    for (int j = 0; j < testASTNodes.at(i)->Body.size(); j++){
      Body.push_back(std::move(testASTNodes.at(i)->Body.at(j)));
    }
  }
  Body.push_back(std::make_unique<NumberExprAST>(0));
  
  auto funcAST = std::make_unique<FunctionAST>(std::move(Proto), std::move(Body));
  funcAST->codegen();
}