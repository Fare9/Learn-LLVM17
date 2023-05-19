#ifndef COUNTIR_H
#define COUNTIR_H

#include "llvm/IR/PassManager.h"
#include "llvm/Pass.h"

class CountIRPass : public llvm::PassInfoMixin<CountIRPass>
{
public:
    llvm::PreservedAnalyses run(llvm::Function &F, llvm::FunctionAnalysisManager &);
};

#endif