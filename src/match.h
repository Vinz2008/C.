#include <vector>
#include <string>
#include <memory>
#include "llvm/IR/Value.h"
#include "ast.h"

using namespace llvm;

Value* MatchNotEnumCodegen(std::string matchVar, std::vector<std::unique_ptr<matchCase>> matchCases, Function* TheFunction);