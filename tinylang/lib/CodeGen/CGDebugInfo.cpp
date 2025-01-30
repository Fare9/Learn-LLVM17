#include "tinylang/CodeGen/CGDebugInfo.h"
#include "tinylang/CodeGen/CGModule.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Path.h"

using namespace tinylang;

CGDebugInfo::CGDebugInfo(CGModule &CGM)
    : CGM(CGM), DBuilder(*CGM.getModule())
{
    /// create the path to the file to add dwarf information
    llvm::SmallString<128> Path(CGM.getASTCtx().getFileName());

    // make absolute path
    llvm::sys::fs::make_absolute(Path);

    /// file to store the Dwarf data
    llvm::DIFile *File = DBuilder.createFile(
        llvm::sys::path::filename(Path),
        llvm::sys::path::parent_path(Path));

    // now set the information for creating the compile unit
    bool IsOptimized = false;        // no optimizations are applied
    unsigned ObjCRunTimeVersion = 0; // we are not using Objective-C
    // type of debug information to emit, in this case, everything
    llvm::DICompileUnit::DebugEmissionKind EmissionKind =
        llvm::DICompileUnit::DebugEmissionKind::FullDebug;

    CU = DBuilder.createCompileUnit(
        llvm::dwarf::DW_LANG_Modula2, // type of language or similar language
        File,                         // file to store debug informaation
        "tinylang",                   // name of producer
        IsOptimized,                  // is optimized compilation?
        StringRef(),
        ObjCRunTimeVersion,
        StringRef(),
        EmissionKind);
}

unsigned CGDebugInfo::getLineNumber(SMLoc Loc)
{
    return CGM.getASTCtx().getSourceMgr().FindLineNumber(Loc);
}

llvm::DIScope *CGDebugInfo::getScope()
{
    if (ScopeStack.empty())
        openScope(CU->getFile()); // in case there's no scope, create a global one
    return ScopeStack.back();
}

void CGDebugInfo::openScope(llvm::DIScope * Scope)
{
    ScopeStack.push_back(Scope);
}

void CGDebugInfo::closeScope()
{
    ScopeStack.pop_back();
}

llvm::DIType *CGDebugInfo::getPervasiveType(TypeDeclaration *Ty)
{
    if (Ty->getName() == "INTEGER")
    {
        return DBuilder.createBasicType(Ty->getName(), // name of the basic type
        64, // size
        llvm::dwarf::DW_ATE_signed); // attributes
    }
    if (Ty->getName() == "BOOLEAN")
    {
        return DBuilder.createBasicType(Ty->getName(),
        1,
        llvm::dwarf::DW_ATE_boolean);
    }
    llvm::report_fatal_error("Unsupported pervasive type");
}

llvm::DIType *CGDebugInfo::getAliasType(AliasTypeDeclaration *Ty)
{
    return DBuilder.createTypedef(
        getType(Ty->getType()),
        Ty->getName(),
        CU->getFile(),
        getLineNumber(Ty->getLocation()),
        getScope()
    );
}

llvm::DIType *CGDebugInfo::getArrayType(ArrayTypeDeclaration *Ty)
{
    auto *ATy = llvm::cast<llvm::ArrayType>(CGM.convertType(Ty));

    const llvm::DataLayout& DL = CGM.getModule()->getDataLayout();
    
    uint64_t NumElements = 5;

    Expr * Nums = Ty->getNums();

    if (auto * Num = llvm::dyn_cast<IntegerLiteral>(Nums))
        NumElements = Num->getValue().getZExtValue();

    llvm::SmallVector<llvm::Metadata*, 4> Subscripts;

    Subscripts.push_back(DBuilder.getOrCreateSubrange(0, NumElements));
    
    return DBuilder.createArrayType(
        DL.getTypeSizeInBits(ATy) * 8,
        DL.getABITypeAlignment(ATy),
        getType(Ty->getType()),
        DBuilder.getOrCreateArray(Subscripts)
    );
}

llvm::DIType *
CGDebugInfo::getRecordType(RecordTypeDeclaration *Ty) {
  llvm::DIType *T = nullptr;
  return T;
}

llvm::DIType *CGDebugInfo::getType(TypeDeclaration *Type)
{
    if (auto *T = TypeCache[Type])
        return T;
    
    if (llvm::isa<PervasiveTypeDeclaration>(Type))
        return TypeCache[Type] = getPervasiveType(Type);
    else if (auto * AliasTy = llvm::dyn_cast<AliasTypeDeclaration>(Type))
        return TypeCache[Type] = getAliasType(AliasTy);
    else if (auto * ArrayTy = llvm::dyn_cast<ArrayTypeDeclaration>(Type))
        return TypeCache[Type] = getArrayType(ArrayTy);
    else if (auto * RecordTy = llvm::dyn_cast<RecordTypeDeclaration>(Type))
        return TypeCache[Type] = getRecordType(RecordTy);
    llvm::report_fatal_error("Unsupported Type");
    return nullptr;
}

llvm::DISubroutineType *CGDebugInfo::getType(ProcedureDeclaration *P)
{
    llvm::SmallVector<llvm::Metadata*, 4> Types;

    const llvm::DataLayout &DL = CGM.getModule()->getDataLayout();
    /// obtain the type at index 0
    if (auto retType = P->getRetType())
        Types.push_back(getType(retType));
    else
        Types.push_back(DBuilder.createUnspecifiedType("void"));
    for (const auto * FP : P->getFormalParams())
    {
        llvm::DIType *PT = getType(FP->getType());
        if (FP->isVar()) // it is a reference of a variable
        {
            llvm::Type *PTy = CGM.convertType(FP->getType());
            PT = DBuilder.createReferenceType(
                llvm::dwarf::DW_TAG_reference_type,
                PT,
                DL.getTypeSizeInBits(PTy) * 8,
                DL.getABITypeAlignment(PTy)
            );
        }
        Types.push_back(PT);
    }
    return DBuilder.createSubroutineType(DBuilder.getOrCreateTypeArray(Types));
}

void CGDebugInfo::emitGlobalVariable(VariableDeclaration *Decl,
                            llvm::GlobalVariable *V)
{
    llvm::DIGlobalVariableExpression * GV = DBuilder.createGlobalVariableExpression(
        getScope(),
        Decl->getName(),
        V->getName(),
        CU->getFile(),
        getLineNumber(Decl->getLocation()),
        getType(Decl->getType()),
        false
    );
    V->addDebugInfo(GV);
}

void CGDebugInfo::emitProcedure(ProcedureDeclaration *Decl,
                        llvm::Function *Fn)
{
    llvm::DISubroutineType * SubT = getType(Decl);
    llvm::DISubprogram * Sub = DBuilder.createFunction(
        getScope(),
        Decl->getName(),
        Fn->getName(),
        CU->getFile(),
        getLineNumber(Decl->getLocation()),
        SubT,
        getLineNumber(Decl->getLocation()),
        llvm::DINode::FlagPrototyped,
        llvm::DISubprogram::SPFlagDefinition
    );
    openScope(Sub);
    Fn->setSubprogram(Sub);
}

void CGDebugInfo::emitProcedureEnd(ProcedureDeclaration *Decl,
                            llvm::Function *Fn)
{
    if (Fn && Fn->getSubprogram())
        DBuilder.finalizeSubprogram(Fn->getSubprogram());
    closeScope();
}

llvm::DILocalVariable *
    CGDebugInfo::emitParameterVariable(FormalParameterDeclaration *FP,
                            size_t Idx, llvm::Value *Val,
                            llvm::BasicBlock *BB)
{
    assert(llvm::isa<llvm::DILocalScope>(getScope()) && "No local scope");

    llvm::DILocalVariable * Var = DBuilder.createParameterVariable(
        getScope(),
        FP->getName(),
        Idx,
        CU->getFile(),
        getLineNumber(FP->getLocation()),
        getType(FP->getType())
    );

    DBuilder.insertDbgValueIntrinsic(
        Val,
        Var,
        DBuilder.createExpression(),
        getDebugLoc(FP->getLocation()),
        BB
    );

    return Var;
}

void CGDebugInfo::emitValue(llvm::Value *Val,
                    llvm::DILocalVariable *Var, SMLoc Loc,
                    llvm::BasicBlock *BB)
{
    auto DLoc = getDebugLoc(Loc);
    llvm::Instruction * Instr = 
        DBuilder.insertDbgValueIntrinsic(Val, Var, DBuilder.createExpression(), DLoc, BB);
    Instr->setDebugLoc(DLoc);
}                    

llvm::DebugLoc CGDebugInfo::getDebugLoc(SMLoc Loc)
{
    std::pair<unsigned, unsigned> LineAndCol = 
        CGM.getASTCtx().getSourceMgr().getLineAndColumn(Loc);
    llvm::DILocation *DILoc = llvm::DILocation::get(
        CGM.getLLVMCtx(), LineAndCol.first, LineAndCol.second, getScope()
    );
    return llvm::DebugLoc(DILoc);
}


void CGDebugInfo::finalize()
{
    DBuilder.finalize();
}