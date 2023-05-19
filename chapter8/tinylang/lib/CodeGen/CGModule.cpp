#include "tinylang/CodeGen/CGModule.h"
#include "tinylang/CodeGen/CGProcedure.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/Support/CommandLine.h"

using namespace tinylang;

static llvm::cl::opt<bool>
    Debug("g", llvm::cl::desc("Generate our own debug information =D"), llvm::cl::init(false));


CGModule::CGModule(ASTContext &ASTCtx, llvm::Module *M)
    : ASTCtx(ASTCtx), M(M), TBAA(CGTBAA(*this))
{
    initialize();
}

void CGModule::declareInst(llvm::Instruction * Inst, TypeDeclaration *Type)
{
    if (auto *N = TBAA.getAccessTagInfo(Type))
        /// take the access tag from a TypeDeclaration
        /// set it as metadata in the instruction
        Inst->setMetadata(llvm::LLVMContext::MD_tbaa, N);
}

void CGModule::initialize()
{
    // initialize the pointer to the types in LLVM
    VoidTy = llvm::Type::getVoidTy(getLLVMCtx());
    Int1Ty = llvm::Type::getInt1Ty(getLLVMCtx());
    Int32Ty = llvm::Type::getInt32Ty(getLLVMCtx());
    Int64Ty = llvm::Type::getInt64Ty(getLLVMCtx());
    Int32Zero = llvm::ConstantInt::get(Int32Ty, 0, /* Signed? */ true);

    if (Debug)
        DebugInfo.reset(new CGDebugInfo(*this));
}

llvm::Type *CGModule::convertType(TypeDeclaration *Ty)
{
    if (llvm::Type * T = TypeCache[Ty])
        return T;
    
    if (llvm::isa<PervasiveTypeDeclaration>(Ty))
    {
        if (Ty->getName() == "INTEGER")
            return Int64Ty;
        if (Ty->getName() == "BOOLEAN")
            return Int1Ty;
    }
    else if (auto * AliasTy = llvm::dyn_cast<AliasTypeDeclaration>(Ty))
    {
        llvm::Type * T = convertType(AliasTy->getType());
        return TypeCache[Ty] = T;
    }
    else if (auto * ArrayTy = llvm::dyn_cast<ArrayTypeDeclaration>(Ty))
    {
        llvm::Type * Component = convertType(ArrayTy->getType());
        Expr * Nums = ArrayTy->getNums();
        uint64_t NumElements = 5;
        if (auto * Num = llvm::dyn_cast<IntegerLiteral>(Nums))
            NumElements = static_cast<uint64_t>(Num->getValue().getExtValue());
        llvm::Type * T = llvm::ArrayType::get(Component, NumElements);
        return TypeCache[Ty] = T;
    }
    else if (auto * RecordTy = llvm::dyn_cast<RecordTypeDeclaration>(Ty))
    {
        llvm::SmallVector<llvm::Type*, 4> Elements;
        for (const auto &F : RecordTy->getFields())
            Elements.push_back(convertType(F.getType()));
        llvm::Type * T = llvm::StructType::create(
            Elements, RecordTy->getName(), false);
        return TypeCache[Ty] = T;
    }
    llvm::report_fatal_error("Unsupported type");
}

std::string CGModule::mangleName(Decl *D)
{
    std::string Mangled;
    llvm::SmallString<16> Tmp;
    while (D)
    {
        llvm::StringRef Name = D->getName();
        Tmp.clear();
        // size of name
        Tmp.append(llvm::itostr(Name.size()));
        // name
        Tmp.append(Name);
        Mangled.insert(0, Tmp.c_str());
        D = D->getEnclosingDecl();
    }
    Mangled.insert(0, "_t");
    return Mangled;
}

llvm::GlobalObject *CGModule::getGlobal(Decl *D)
{
    return Globals[D];
}

void CGModule::decorateInst(llvm::Instruction * Inst, TypeDeclaration *TyDe)
{
    if (auto * N = TBAA.getAccessTagInfo(TyDe))
        Inst->setMetadata(llvm::LLVMContext::MD_tbaa, N);
}

void CGModule::run(ModuleDeclaration *Mod)
{
    this->Mod = Mod;
    for (auto *Decl : Mod->getDecls())
    {
        if (auto *Var = llvm::dyn_cast<VariableDeclaration>(Decl))
        {
            // create the global variables
            llvm::GlobalVariable *V = new llvm::GlobalVariable(
                *M, 
                convertType(Var->getType()),    // specify a LLVM IR type
                /* is constant */ false,        
                llvm::GlobalValue::PrivateLinkage, // create as private for the module
                nullptr,
                mangleName(Var)                 // mangled name for the variable
            );
            Globals[Var] = V;   // store the global variable
            // now apply the debug information
            if (CGDebugInfo * Dbg = getDbgInfo())
                Dbg->emitGlobalVariable(Var, V);
        }
        else if (auto *Proc = llvm::dyn_cast<ProcedureDeclaration>(Decl))
        {
            CGProcedure CGP(*this);
            CGP.run(Proc);
        }
    }

    if (CGDebugInfo * Dbg = getDbgInfo())
        Dbg->finalize();
}

void CGModule::applyLocation(llvm::Instruction *Inst, llvm::SMLoc Loc)
{
    if (CGDebugInfo * Dbg = getDbgInfo())
        Inst->setDebugLoc(Dbg->getDebugLoc(Loc));
}