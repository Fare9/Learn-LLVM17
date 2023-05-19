#ifndef TINYLANG_CODEGEN_CGPROCEDURE_H
#define TINYLANG_CODEGEN_CGPROCEDURE_H

#include "tinylang/AST/AST.h"
#include "tinylang/CodeGen/CGModule.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Value.h"

namespace llvm
{
    class Function;
} // namespace llvm

namespace tinylang
{
    class CGProcedure
    {
        CGModule &CGM;
        llvm::IRBuilder<> Builder;

        llvm::BasicBlock *Curr;

        ProcedureDeclaration *Proc;
        llvm::FunctionType *Fty;
        llvm::Function *Fn;

        /// @brief Struct to keep more information for basic block
        /// analysis, this include definitions of a variable for SSA
        /// generation. And for keeping information for writing the
        /// Phi information
        struct BasicBlockDef
        {
            /// Maps a variable (or formal parameter) to its definition in IR
            llvm::DenseMap<Decl *, llvm::TrackingVH<llvm::Value>> Defs;
            /// Set of incompleted phi instructions.
            llvm::DenseMap<llvm::PHINode *, Decl *> IncompletePhis;
            /// Block is sealed, means no more predecessors will be added nor analyzed.
            unsigned Sealed : 1;

            BasicBlockDef() : Sealed(0) {}
        };

        llvm::DenseMap<llvm::BasicBlock *, BasicBlockDef> CurrentDef;

        /// @brief write a declaration of a local variable or a formal parameter
        /// to the Defs map
        /// @param BB
        /// @param Decl
        /// @param Val
        void writeLocalVariable(llvm::BasicBlock *BB, Decl *Decl, llvm::Value *Val);
        /// @brief Given a declaration, give me the declared value in LLVM IR.
        /// @param BB
        /// @param Decl
        /// @return
        llvm::Value *readLocalVariable(llvm::BasicBlock *BB, Decl *Decl);
        /// @brief Recursively read a local variable, this will be called from readLocalVariable
        /// @param BB 
        /// @param Decl 
        /// @return 
        llvm::Value *readLocalVariableRecursive(llvm::BasicBlock *BB, Decl *Decl);
        /// @brief Add an empty Phi instruction to a basic block, these Phi instructions
        /// are an assignment to a variable, that represent the same variable from previous
        /// basic blocks
        /// @param BB
        /// @param Decl
        /// @return
        llvm::PHINode *addEmptyPhi(llvm::BasicBlock *BB, Decl *Decl);
        /// @brief Add an operand to a Phi instruction, these parameters represent
        /// a same value from different incoming paths.
        /// @param BB
        /// @param Decl
        /// @param Phi
        llvm::Value *addPhiOperands(llvm::BasicBlock *BB, Decl *Decl, llvm::PHINode *Phi);
        /// @brief A Phi instruction commonly is avoided in optimizing functions
        /// if we remove Phi instructions that only have one operand or all its
        /// operands are the same, or have no operands at all, we will be able to
        /// improve optimization of a function.
        /// @param Phi
        llvm::Value * optimizePhi(llvm::PHINode *Phi);
        /// @brief Set a basic block as sealed, this means all predecessors were analyzed
        /// and no more will be added.
        /// @param BB
        void sealBlock(llvm::BasicBlock *BB);

        llvm::DenseMap<FormalParameterDeclaration *, llvm::Argument *> FormalParams;

        /// @brief Read a variable from the basic block, a local variable
        /// will be read through previous defined methods. But for a global
        /// variable we have to use a load-and-store instructions.
        /// Also we have to differentiate between parameters by value and
        /// by reference
        /// @param BB
        /// @param D
        /// @return
        llvm::Value *readVariable(llvm::BasicBlock *BB, Decl *D, bool LoadVal = true);

        /// @brief Generation of write instruction of variables, depending on
        /// the type of variable, different strategies must be implemented.
        /// @param BB basic block where instruction happens
        /// @param Decl declaration of the variable
        /// @param Val value assigned
        void writeVariable(llvm::BasicBlock *BB, Decl *Decl, llvm::Value *Val);

        /// @brief Map the types of a declaration to a type in LLVM IR, using a
        /// function from CHModule or creating a pointer type in case of a reference.
        /// @param Decl
        llvm::Type *mapType(Decl *Decl);

        /// @brief Get the type for creating a function given a declaration of a procedure.
        /// @param Proc procedure to create the function type
        /// @return type of the function from the procedure
        llvm::FunctionType *createFunctionType(ProcedureDeclaration *Proc);

        /// @brief Create a function with a mangled name, the name of the parameters, the type
        /// of the parameters using the prototype from createFunctionType.
        llvm::Function *createFunction(ProcedureDeclaration *Proc, llvm::FunctionType *FTy);

    protected:
        void setCurr(llvm::BasicBlock *BB)
        {
            Curr = BB;
            Builder.SetInsertPoint(Curr);
        }

        llvm::BasicBlock *createBasicBlock(
            const llvm::Twine &Name,
            llvm::BasicBlock *InsertBefore = nullptr)
        {
            return llvm::BasicBlock::Create(CGM.getLLVMCtx(), Name, Fn, InsertBefore);
        }

        llvm::Value *emitInfixExpr(InfixExpression *E);
        llvm::Value *emitPrefixExpr(PrefixExpression *E);
        llvm::Value *emitExpr(Expr *E);

        void emitStmt(AssignmentStatement *Stmt);
        void emitStmt(ProcedureCallStatement *Stmt);
        void emitStmt(IfStatement *Stmt);
        void emitStmt(WhileStatement *Stmt);
        void emitStmt(ReturnStatement *Stmt);
        void emit(const StmtList &Stmts);

    public:
        CGProcedure(CGModule &CGM) : CGM(CGM), Builder(CGM.getLLVMCtx()),
                                     Curr(nullptr)
        {
        }

        /// @brief Convert a given procedure into a LLVM IR function
        /// @param Proc procedure to convert
        void run(ProcedureDeclaration *Proc);
        void run();
    };
} // namespace tinylang

#endif