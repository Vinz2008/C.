#include <vector>
#include <string>
#include <memory>
#include "llvm/IR/Value.h"
//#include "ast.h"

using namespace llvm;

class matchCase;

Value* MatchNotEnumCodegen(std::string matchVar, std::vector<matchCase> matchCases, Function* TheFunction);