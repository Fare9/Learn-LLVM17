#ifndef TINYLANG_CODEGEN_CGDEBUGINFO_H
#define TINYLANG_CODEGEN_CGDEBUGINFO_H

#include "tinylang/AST/AST.h"
#include "llvm/IR/DIBuilder.h"
#include "llvm/IR/DebugInfo.h"
#include "llvm/IR/DebugLoc.h"
#include "llvm/IR/GlobalVariable.h"

namespace tinylang
{
class CGModule;

class CGDebugInfo
{
    /// @brief module of the builder
    /// used to transform types from AST to LLVM Types
    CGModule &CGM;

    /// @brief Builder for the debug information
    llvm::DIBuilder DBuilder;

    /// @brief A pointer to the compilation unit
    llvm::DICompileUnit *CU;

    /// @brief For keeping all the DITypes by the
    /// type declaration
    llvm::DenseMap<TypeDeclaration *, llvm::DIType *> TypeCache;

    /// @brief vector used as stack for keeping
    /// the scope
    llvm::SmallVector<llvm::DIScope *, 4> ScopeStack;

    /// @brief Get the current scope of the program
    /// @return scope
    llvm::DIScope *getScope();

    /// @brief Open a new scope when we enter for example in a function
    /// @param  
    void openScope(llvm::DIScope *);

    /// @brief Obtain a line number given a Location object
    /// @param Loc location object
    /// @return line number
    unsigned getLineNumber(SMLoc Loc);
    
    /// @brief Diferent generation of DITypes for debug information
    /// for each kind of type declaration we create one DIType
    /// @param Ty 
    /// @return 
    llvm::DIType *getPervasiveType(TypeDeclaration *Ty);
    /// @brief A renamed version of a type name, we make first use of
    /// scope and line-number information
    /// @param Ty 
    /// @return 
    llvm::DIType *getAliasType(AliasTypeDeclaration *Ty);
    /// @brief For an array we have to specify the size and the alignment,
    /// we retrieve this from the DataLayout class
    /// @param Ty 
    /// @return 
    llvm::DIType *getArrayType(ArrayTypeDeclaration *Ty);
    llvm::DIType *getRecordType(RecordTypeDeclaration *Ty);
    /// @brief Generic for obtaining from the different types
    /// @param Type 
    /// @return 
    llvm::DIType *getType(TypeDeclaration *Type);
    /// @brief Obtaining debug information from a procedure declaration
    /// @param P 
    /// @return 
    llvm::DISubroutineType *getType(ProcedureDeclaration *P);

public:
    CGDebugInfo(CGModule &CGM);

    /// @brief Close a scope and go to previous
    void closeScope();
    /// @brief Emit the debug information for a global variable
    /// @param Decl declaration of variable
    /// @param V object of global variable
    void emitGlobalVariable(VariableDeclaration *Decl,
                            llvm::GlobalVariable *V);
    /// @brief Emit the debug information from a procedure
    /// @param Decl 
    /// @param Fn 
    void emitProcedure(ProcedureDeclaration *Decl,
                        llvm::Function *Fn);
    /// @brief Inform the builder to finalize the construction of debug
    /// information. Also remove procedure from the scope stack.
    /// @param Decl 
    /// @param Fn 
    void emitProcedureEnd(ProcedureDeclaration *Decl,
                            llvm::Function *Fn);
    llvm::DILocalVariable *
    emitParameterVariable(FormalParameterDeclaration *FP,
                            size_t Idx, llvm::Value *Val,
                            llvm::BasicBlock *BB);
    void emitValue(llvm::Value *Val,
                    llvm::DILocalVariable *Var, SMLoc Loc,
                    llvm::BasicBlock *BB);

    llvm::DebugLoc getDebugLoc(SMLoc Loc);

    void finalize();
};
} // namespace tinylang

#endif