#ifndef COUNTIR_H
#define COUNTIR_H

#include "llvm/IR/PassManager.h"
#include "llvm/Pass.h"

class CountIRPass : public llvm::PassInfoMixin<CountIRPass>
{
public:
    llvm::PreservedAnalyses run(llvm::Function &F, llvm::FunctionAnalysisManager &);
};

class CountIRLegacyPass : public llvm::FunctionPass
{
public:
    /// @brief old passes need from an ID
    static char ID;

    /// @brief the ID will be used in the constructor of
    /// parent class
    CountIRLegacyPass() : llvm::FunctionPass(ID)
    {
    }

    /// @brief the pass that will be run on every function
    /// @param F function where to apply the analysis
    /// @return
    bool runOnFunction(llvm::Function &F) override;

    /// @brief Used to announce that all the analysis results are saved
    /// @param AU
    void getAnalysisUsage(llvm::AnalysisUsage &AU) const override;
};

#endif