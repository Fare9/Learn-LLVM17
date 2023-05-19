#include "CountIR.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/Support/Debug.h"
// for registering our pass
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"

using namespace llvm;

/// the debug infrastructure requires
/// us to define the DEBUG_TYPE to print it
/// later with the statisticcs
#define DEBUG_TYPE "countir"

/// For defining statistics variables we
/// can use simply a macro
STATISTIC(NumOfInst, "Number of instructions.");
STATISTIC(NumOfBB, "Number of basic blocks.");


void runCounting(Function &F)
{
    for (BasicBlock &BB : F) // take all the basic blocks from a function
    {
        NumOfBB++;
        for (Instruction &I : BB)
        {
            (void)I; // make use of I to avoid warning
            NumOfInst++;
        }
    }
}


PreservedAnalyses CountIRPass::run(Function &F, FunctionAnalysisManager &)
{
    runCounting(F);
    // in the return, we say we have preserved all the
    // other analyses
    return PreservedAnalyses::all();
}


//-----------------------------------------------------------------------------
// New PM Registration
//-----------------------------------------------------------------------------
llvm::PassPluginLibraryInfo getCountIRPluginInfo()
{
    return {LLVM_PLUGIN_API_VERSION, "CountIR", "v0.1",
            // lambda function for registering the countir pass
            [](PassBuilder &PB)
            {
                PB.registerPipelineParsingCallback(
                    [](StringRef Name, FunctionPassManager &FPM,
                       ArrayRef<PassBuilder::PipelineElement>)
                    {
                        if (Name == "my-countir")
                        {
                            FPM.addPass(CountIRPass());
                            return true;
                        }
                        return false;
                    });
            }};
}

/// @brief callback called by Pass manager for registering a plugin
/// @return 
extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo()
{
    return getCountIRPluginInfo();
}