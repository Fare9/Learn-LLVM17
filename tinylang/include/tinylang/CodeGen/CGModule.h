#ifndef TINYLANG_CODEGEN_CGMODULE_H
#define TINYLANG_CODEGEN_CGMODULE_H

#include "tinylang/AST/AST.h"
#include "tinylang/AST/ASTContext.h"
#include "tinylang/CodeGen/CGDebugInfo.h"
#include "tinylang/CodeGen/CGTBAA.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"

namespace tinylang
{

    class CGModule
    {
        ASTContext &ASTCtx;
        llvm::Module *M;

        ModuleDeclaration *Mod;

        llvm::DenseMap<TypeDeclaration *, llvm::Type *> TypeCache;

        llvm::DenseMap<Decl *, llvm::GlobalObject *> Globals;

        CGTBAA TBAA;
        std::unique_ptr<CGDebugInfo> DebugInfo;

    public:
        llvm::Type *VoidTy;
        llvm::Type *Int1Ty;
        llvm::Type *Int32Ty;
        llvm::Type *Int64Ty;
        llvm::Constant *Int32Zero;

    public:
        CGModule(ASTContext &ASTCtx, llvm::Module *M);
        
        /// @brief Initialize the pointers to the types and constant values
        void initialize();

        /// @brief Return the ASTContext
        /// @return 
        ASTContext &getASTCtx()
        {
            return ASTCtx;
        }

        /// @brief Return the LLVMContext object
        /// @return 
        llvm::LLVMContext &getLLVMCtx() { return M->getContext(); }
        /// @brief Return the Module object for creating LLVM IR instructions
        /// @return 
        llvm::Module *getModule() { return M; }

        /// @brief Get the module declaration from the Tinylang
        /// @return ModuleDeclaration value
        ModuleDeclaration *getModuleDeclaration() { return Mod; }

        /// @brief decorate an instruction with information for alias pointer analysis
        /// @param Inst 
        /// @param Type 
        void declareInst(llvm::Instruction * Inst, TypeDeclaration *Type);

        /// @brief Convert a given TypeDeclaration to a llvm::Type, now supported integer and boolean
        /// @param Ty declared type in code
        /// @return a pointer to the declared type in LLVM IR form
        llvm::Type *convertType(TypeDeclaration *Ty);

        /// @brief Create a "mangled" name for a declaration, this avoid problems with Linkage
        /// if two modules has same function, and these are declared as private, there is no problem
        /// since they reside in the module. But if functions are declared external there's a problem
        /// since two modules would have functions with same name, we add mangling
        /// @param D declaration to mangle
        /// @return name like _t<module name length><module name><function name length><function name>
        std::string mangleName(Decl *D);

        llvm::GlobalObject *getGlobal(Decl *);

        /// @brief Return a pointer to the debug information object
        /// @return 
        CGDebugInfo* getDbgInfo()
        {
            return DebugInfo.get();
        }

        void applyLocation(llvm::Instruction *Inst, llvm::SMLoc Loc);

        /// @brief Include into a instruction a metadata with the information
        /// about the access
        /// @param Inst instruction to decorate
        /// @param TyDe TypeDenoter to get access info
        void decorateInst(llvm::Instruction * Inst, TypeDeclaration *TyDe);

        void run(ModuleDeclaration *Mod);
    };

} // namespace tinylang

#endif