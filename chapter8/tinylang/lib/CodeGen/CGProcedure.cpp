#include "tinylang/CodeGen/CGProcedure.h"
#include "llvm/IR/CFG.h"
#include "llvm/Support/Casting.h"

using namespace tinylang;

void CGProcedure::writeLocalVariable(llvm::BasicBlock *BB, Decl *Decl, llvm::Value *Val)
{
    assert(BB && "Basic Block does not exist");
    assert(
        (llvm::isa<VariableDeclaration>(Decl) ||
         llvm::isa<FormalParameterDeclaration>(Decl)) &&
             "Declaration must be a variable or formal parameter");
    assert(Val && "Value does not exist");
    CurrentDef[BB].Defs[Decl] = Val;
}

llvm::Value *
CGProcedure::readLocalVariable(llvm::BasicBlock *BB,
                               Decl *Decl)
{
    assert(BB && "Basic Block does not exist");
    assert(
        (llvm::isa<VariableDeclaration>(Decl) ||
         llvm::isa<FormalParameterDeclaration>(Decl)) &&
             "Declaration must be a variable or formal parameter");
    auto Val = CurrentDef[BB].Defs.find(Decl);
    if (Val != CurrentDef[BB].Defs.end())
        return Val->second;
    // In case we didn't find the value, it doesn't mean it does not
    // exist, it could be that it's declared in a parent basic block
    return readLocalVariableRecursive(BB, Decl);
}

llvm::Value *
CGProcedure::readLocalVariableRecursive(llvm::BasicBlock *BB, Decl *Decl)
{
    llvm::Value *Val = nullptr;

    // basic blocks previous to current one have not been added
    if (!CurrentDef[BB].Sealed)
    {
        // Add incomplete phi for variable
        llvm::PHINode *Phi = addEmptyPhi(BB, Decl);
        // in the current basic block, there's a phi for the declaration
        CurrentDef[BB].IncompletePhis[Phi] = Decl;
        Val = Phi;
    }
    // in case all the basic blocks have been added (BB is sealed)
    // getSinglePredecessor will return a valid pointer only if the Basic
    // Block has a single predecessor
    else if (auto *PredBB = BB->getSinglePredecessor())
    {
        // read now from one predecessor
        Val = readLocalVariable(PredBB, Decl);
    }
    // in case it has more than one predecessor, we need now to create a phi
    // for possible variables coming from different predecessors
    else
    {
        // an empty phi break potential cycles.
        llvm::PHINode *Phi = addEmptyPhi(BB, Decl);
        writeLocalVariable(BB, Decl, Val);
        Val = addPhiOperands(BB, Decl, Phi);
    }
    writeLocalVariable(BB, Decl, Val);
    return Val;
}

llvm::PHINode *
CGProcedure::addEmptyPhi(llvm::BasicBlock *BB, Decl *Decl)
{
    // add PHI as first statement of BB
    // or add it in front
    return BB->empty()
               ? llvm::PHINode::Create(mapType(Decl), 0, "", BB)
               : llvm::PHINode::Create(mapType(Decl), 0, "", &BB->front());
}

llvm::Value *CGProcedure::addPhiOperands(llvm::BasicBlock *BB, Decl *Decl, llvm::PHINode *Phi)
{
    for (auto I = llvm::pred_begin(BB),
              E = llvm::pred_end(BB);
         I != E; ++I)
    {
        // Phi instructions need from a llvm::Value and a Basic Block
        Phi->addIncoming(readLocalVariable(*I, Decl), *I);
    }
    return optimizePhi(Phi);
}

llvm::Value *CGProcedure::optimizePhi(llvm::PHINode *Phi)
{
    llvm::Value *Same = nullptr;

    for (llvm::Value *V : Phi->incoming_values())
    {
        // check if all the values in
        // the Phi operands are the same
        if (V == Same || V == Phi)
            continue;
        if (Same && V != Same) // we found a different value, cannot be removed
            return Phi;        // return the phi register
        Same = V;
    }

    if (Same == nullptr) // there are no operands on Phi instruction, write undef
        Same = llvm::UndefValue::get(Phi->getType());

    // collect all phi instructions using the result from this one
    // we can use the chain of def-use values
    llvm::SmallVector<llvm::PHINode *, 8> CandidatePhis;
    for (llvm::Use &U : Phi->uses())
    {
        if (auto *P = llvm::dyn_cast<llvm::PHINode>(U.getUser()))
        {
            // we are in a PHI instruction using current Phi value
            if (P != Phi)
                CandidatePhis.push_back(P);
        }
    }
    // since all the values are the same or there's just one value
    // go through all use instruction replacing variable from phi
    // with the register here.
    Phi->replaceAllUsesWith(Same);
    Phi->eraseFromParent(); // erase this instruction from the basic block.
    // since removing one phi can make that other become optimizable
    // go optimizing the others!
    for (auto *P : CandidatePhis)
        optimizePhi(P);
    return Same; // return the result register
}

void CGProcedure::sealBlock(llvm::BasicBlock *BB)
{
    assert(!CurrentDef[BB].Sealed && "Attempt to seal already sealed block");

    for (auto PhiDecl : CurrentDef[BB].IncompletePhis)
    {
        addPhiOperands(BB, PhiDecl.second, PhiDecl.first);
    }
    CurrentDef[BB].IncompletePhis.clear();
    CurrentDef[BB].Sealed = true;
}

llvm::Value *CGProcedure::readVariable(llvm::BasicBlock *BB, Decl *D, bool LoadVal)
{
    // check if the declaration is a variable declaration
    if (auto *V = llvm::dyn_cast<VariableDeclaration>(D))
    {
        if (V->getEnclosingDecl() == Proc) // check if it's current procedure (local variable)
            return readLocalVariable(BB, D);
        else if (V->getEnclosingDecl() == CGM.getModuleDeclaration()) // check that variable is in module (global variable)
        {
            auto *Global = CGM.getGlobal(D);
            if (!LoadVal)
                return Global;                             // return the variable
            return Builder.CreateLoad(mapType(D), Global); // create  a load for global variable
        }
        else
            llvm::report_fatal_error("Nested procedures not yet supported");
    }
    else if (auto *FP = llvm::dyn_cast<FormalParameterDeclaration>(D))
    {
        if (FP->isVar()) // check if FP is passed by reference
        {
            if (!LoadVal)
                return FormalParams[FP];                                                       // pass the reference
            return Builder.CreateLoad(mapType(FP), FormalParams[FP]); // load from memory the value
        }
        else
            return readLocalVariable(BB, D); // if it is not a reference, read it as a local variable
    }
    else
        llvm::report_fatal_error("Unsupported declaration");
}

void CGProcedure::writeVariable(llvm::BasicBlock *BB, Decl *Decl, llvm::Value *Val)
{
    // Check if it is a declaration of a variable, one from
    // a procedure, or a global one
    if (auto *V = llvm::dyn_cast<VariableDeclaration>(Decl))
    {
        if (V->getEnclosingDecl() == Proc)
            writeLocalVariable(BB, Decl, Val);
        else if (V->getEnclosingDecl() == CGM.getModuleDeclaration()){
            auto * Inst = Builder.CreateStore(Val, CGM.getGlobal(Decl));
            CGM.decorateInst(Inst, V->getType());
        }
        else
            llvm::report_fatal_error("Nested procedures not yet supported");
    }
    else if (auto *FP = llvm::dyn_cast<FormalParameterDeclaration>(Decl))
    {
        // if it is a reference, create a store to that memory
        if (FP->isVar())
        {
            auto * Inst = Builder.CreateStore(Val, FormalParams[FP]);
            CGM.decorateInst(Inst, V->getType());
        }
        else
            writeLocalVariable(BB, Decl, Val);
    }
    else
        llvm::report_fatal_error("Unsupported declaration");
}

llvm::Type *CGProcedure::mapType(Decl *Decl)
{
    if (auto *FP = llvm::dyn_cast<FormalParameterDeclaration>(Decl))
    {
        llvm::Type *Ty = CGM.convertType(FP->getType()); // convert the type to obtain an LLVM IR Type
        if (FP->isVar())                                 // check if the parameter is a reference
            Ty = Ty->getPointerTo();                     // obtain the type of pointer from the type
        return Ty;
    }
    if (auto *V = llvm::dyn_cast<VariableDeclaration>(Decl)) // if it is a variable declaration
        return CGM.convertType(V->getType());                // just obtain the type
    return CGM.convertType(llvm::cast<TypeDeclaration>(Decl));
}

llvm::FunctionType *CGProcedure::createFunctionType(ProcedureDeclaration *Proc)
{
    llvm::Type *ResultTy = CGM.VoidTy; // by default return is a void type
    if (Proc->getRetType())
        ResultTy = mapType(Proc->getRetType());
    // get list of parameters from the procedure
    auto FormalParams = Proc->getFormalParams();
    // vector to store the types from each parameters
    llvm::SmallVector<llvm::Type *, 8> ParamTypes;
    // now store in the vector the types for the LLVM IR
    // function prototype
    for (auto FP : FormalParams)
    {
        llvm::Type *Ty = mapType(FP);
        ParamTypes.push_back(Ty);
    }
    return llvm::FunctionType::get(ResultTy, ParamTypes, false);
}

llvm::Function *CGProcedure::createFunction(ProcedureDeclaration *Proc, llvm::FunctionType *FTy)
{
    llvm::Function *Fn = llvm::Function::Create(
        Fty,                                // the type of the function
        llvm::GlobalValue::ExternalLinkage, // linkage type
        CGM.mangleName(Proc),               // function name (mangled)
        CGM.getModule()                     // module where we generate function
    );

    // Now we give the parameters' name
    size_t Idx = 0;
    for (auto I = Fn->arg_begin(), E = Fn->arg_end(); I != E; ++I, ++Idx)
    {
        llvm::Argument *Arg = I;
        FormalParameterDeclaration *FP = Proc->getFormalParams()[Idx];
        // In case we have a parameter that is a reference (isVar)
        // we need to create it as a pointer type, but this pointer
        // will have a set of restrictions, a reference cannot be
        // null like a pointer.
        if (FP->isVar())
        {

            llvm::AttrBuilder Attr(CGM.getLLVMCtx());
            // get pointer size!
            llvm::TypeSize Sz =
                CGM.getModule()->getDataLayout().getTypeStoreSize(
                    CGM.convertType(FP->getType()));
            // always dereferenceable, we can read the value
            // pointed to by risking a general protection fault
            Attr.addDereferenceableAttr(Sz);
            // pointer cannot be passed around, no copies
            // of the pointer outlive the call to the function
            Attr.addAttribute(llvm::Attribute::NoCapture);
            // even while is not included in the book
            // we add the reference cannot be null
            Attr.addAttribute(llvm::Attribute::NonNull);
            // add attributes to the argument
            Arg->addAttrs(Attr);
        }
        // give the name to the Arg
        Arg->setName(FP->getName());
    }
    return Fn;
}

llvm::Value *
CGProcedure::emitInfixExpr(InfixExpression *E)
{
    llvm::Value *Left = emitExpr(E->getLeft());
    llvm::Value *Right = emitExpr(E->getRight());
    llvm::Value *Result = nullptr;

    switch (E->getOperatorInfo().getKind()) // check the type of operator for the expression
    {
    case tok::plus:
        Result = Builder.CreateNSWAdd(Left, Right);
        break;
    case tok::minus:
        Result = Builder.CreateNSWSub(Left, Right);
        break;
    case tok::star:
        Result = Builder.CreateNSWMul(Left, Right);
        break;
    case tok::kw_DIV:
        Result = Builder.CreateSDiv(Left, Right);
        break;
    case tok::kw_MOD:
        Result = Builder.CreateSRem(Left, Right);
        break;
    case tok::equal:
        Result = Builder.CreateICmpEQ(Left, Right);
        break;
    case tok::hash:
        Result = Builder.CreateICmpNE(Left, Right);
        break;
    case tok::less:
        Result = Builder.CreateICmpSLT(Left, Right);
        break;
    case tok::lessequal:
        Result = Builder.CreateICmpSLE(Left, Right);
        break;
    case tok::greater:
        Result = Builder.CreateICmpSGT(Left, Right);
        break;
    case tok::greaterequal:
        Result = Builder.CreateICmpSGE(Left, Right);
        break;
    case tok::kw_AND:
        Result = Builder.CreateAnd(Left, Right);
        break;
    case tok::kw_OR:
        Result = Builder.CreateOr(Left, Right);
        break;
    case tok::slash:
        // division of real numbers not supported
        LLVM_FALLTHROUGH;
    default:
        llvm_unreachable("Wrong operator");
    }

    return Result;
}

llvm::Value *
CGProcedure::emitPrefixExpr(PrefixExpression *E)
{
    llvm::Value *Result = emitExpr(E->getExpr());
    switch (E->getOperatorInfo().getKind())
    {
    case tok::plus:
        // Identity nothing to do
        break;
    case tok::minus:
        // negation of a Value
        Result = Builder.CreateNeg(Result);
        break;
    case tok::kw_NOT:
        // NOT of Value
        Result = Builder.CreateNot(Result);
        break;
    default:
        llvm_unreachable("Wrong operator");
    }
    return Result;
}

llvm::Value *
CGProcedure::emitExpr(Expr *E)
{
    if (auto *Infix = llvm::dyn_cast<InfixExpression>(E))
        return emitInfixExpr(Infix);
    else if (auto *Prefix = llvm::dyn_cast<PrefixExpression>(E))
        return emitPrefixExpr(Prefix);
    else if (auto *Var = llvm::dyn_cast<Designator>(E))
    {
        auto *Decl = Var->getDecl();
        llvm::Value *Val = readVariable(Curr, Decl);
        // we need to add here support for array
        // and records
        auto &Selectors = Var->getSelectors();

        for (auto I = Selectors.begin(), E = Selectors.end(); I != E; /* no increment */)
        {
            if (llvm::dyn_cast<IndexSelector>(*I)) // index selector of arrays?
            {
                llvm::SmallVector<llvm::Value *, 4> IdxList;

                // obtain the list of indexes
                while (I != E)
                {
                    if (auto *Sel = llvm::dyn_cast<IndexSelector>(*I))
                    {
                        IdxList.push_back(emitExpr(Sel->getIndex()));
                        ++I;
                    }
                    else
                        break;
                }

                Val = Builder.CreateInBoundsGEP(Val->getType(), Val, IdxList);
                Val = Builder.CreateLoad(Val->getType(), Val);
            }
            else if (llvm::dyn_cast<FieldSelector>(*I)) // field selector for record
            {
                llvm::SmallVector<llvm::Value *, 4> IdxList;
                while (I != E)
                {
                    if (auto *Sel = llvm::dyn_cast<FieldSelector>(*I))
                    {
                        llvm::Value *V = llvm::ConstantInt::get(CGM.Int64Ty, Sel->getIndex()); // create index of record to access as an Int64
                        IdxList.push_back(V);
                        ++I;
                    }
                    else
                        break;
                }
                Val = Builder.CreateInBoundsGEP(Val->getType(), Val, IdxList);
                Val = Builder.CreateLoad(Val->getType(), Val);
            }
            else if (llvm::dyn_cast<DereferenceSelector>(*I)) // If the selector can be deferenced
            {
                Val = Builder.CreateLoad(Val->getType(), Val);
                ++I;
            }
            else
                llvm::report_fatal_error("Unsupported selector");
        }

        return Val;
    }
    else if (auto *Const = llvm::dyn_cast<ConstantAccess>(E))
        return emitExpr(Const->getDecl()->getExpr());
    else if (auto *IntLit = llvm::dyn_cast<IntegerLiteral>(E))
        return llvm::ConstantInt::get(CGM.Int64Ty, IntLit->getValue());
    else if (auto *BoolLit = llvm::dyn_cast<BooleanLiteral>(E))
        return llvm::ConstantInt::get(CGM.Int1Ty, BoolLit->getValue());
    llvm::report_fatal_error("Unsupported expression");
}

void CGProcedure::emitStmt(AssignmentStatement *Stmt)
{
    auto *Val = emitExpr(Stmt->getExpr());
    Designator *Desig = Stmt->getVar();
    auto &Selectors = Desig->getSelectors();

    if (Selectors.empty()) // if there are not selectors, we write a variable
        writeVariable(Curr, Desig->getDecl(), Val);
    else
    {
        llvm::SmallVector<llvm::Value *, 4> IdxList;
        // First index for GEP
        IdxList.push_back(llvm::ConstantInt::get(CGM.Int32Ty, 0));

        auto *Base = readVariable(Curr, Desig->getDecl(), false);

        for (auto I = Selectors.begin(), E = Selectors.end(); I != E; ++I)
        {
            if (auto *IdxSel = llvm::dyn_cast<IndexSelector>(*I))
            {
                IdxList.push_back(emitExpr(IdxSel->getIndex()));
            }
            else if (auto *FieldSel = llvm::dyn_cast<FieldSelector>(*I))
            {
                llvm::Value *V = llvm::ConstantInt::get(CGM.Int32Ty, FieldSel->getIndex());
                IdxList.push_back(V);
            }
            else
                llvm::report_fatal_error("not implemented");
        }
        if (!IdxList.empty())
        {
            if (Base->getType()->isPointerTy())
            {
                Base = Builder.CreateInBoundsGEP(Base->getType(), Base, IdxList);
                Builder.CreateStore(Val, Base);
            }
            else
                llvm::report_fatal_error("should not happen");
        }
    }
}

void CGProcedure::emitStmt(ProcedureCallStatement *Stmt)
{
    llvm::report_fatal_error("not implemented");
}

void CGProcedure::emitStmt(IfStatement *Stmt)
{
    bool HasElse = Stmt->getElseStmts().size() > 0;

    // Create the required basic blocks (1 for if or two)
    // in case of an else
    llvm::BasicBlock *IfBB = llvm::BasicBlock::Create(CGM.getLLVMCtx(), "if.body", Fn);
    llvm::BasicBlock *ElseBB =
        HasElse ? llvm::BasicBlock::Create(CGM.getLLVMCtx(), "else.body", Fn)
                : nullptr;
    // the one after the conditional code
    llvm::BasicBlock *AfterIfBB = llvm::BasicBlock::Create(CGM.getLLVMCtx(), "after.if", Fn);

    // convert the condition code
    llvm::Value *Cond = emitExpr(Stmt->getCond());

    Builder.CreateCondBr(
        Cond,                        // condition of the branching
        IfBB,                        // if taken
        HasElse ? ElseBB : AfterIfBB // if not taken
    );

    // Current block sealed
    sealBlock(Curr);

    // current Block is If Block to add statements inside
    setCurr(IfBB);
    emit(Stmt->getIfStmts());
    // if there's not a terminator (jump to
    // next basic block), create it
    if (!Curr->getTerminator())
        Builder.CreateBr(AfterIfBB);
    sealBlock(Curr);

    // in case there's an else branch
    // do the same with else branch
    if (HasElse)
    {
        setCurr(ElseBB);
        emit(Stmt->getElseStmts());
        if (!Curr->getTerminator())
            Builder.CreateBr(AfterIfBB);
        sealBlock(Curr);
    }
    // finally continue with AfterIfBB
    setCurr(AfterIfBB);
}

void CGProcedure::emitStmt(WhileStatement *Stmt)
{
    // create the basic blocks for the while statement

    // first one the conditional block
    llvm::BasicBlock *WhileCondBB;
    // body of the while loop
    llvm::BasicBlock *WhileBodyBB = llvm::BasicBlock::Create(
        CGM.getLLVMCtx(), "while.body", Fn);
    // the block after the while
    llvm::BasicBlock *AfterWhileBB = llvm::BasicBlock::Create(
        CGM.getLLVMCtx(), "after.while", Fn);

    if (Curr->empty())
    {
        Curr->setName("while.cond");
        WhileCondBB = Curr;
    }
    else
    {
        WhileCondBB = createBasicBlock("while.cond");
        Builder.CreateBr(WhileCondBB);
        sealBlock(Curr);
        setCurr(WhileCondBB);
    }

    llvm::Value *Cond = emitExpr(Stmt->getCond());
    Builder.CreateCondBr(Cond, WhileBodyBB, AfterWhileBB);

    // create the body of the while loop
    setCurr(WhileBodyBB);
    emit(Stmt->getWhileStmts());
    Builder.CreateBr(WhileCondBB);
    sealBlock(Curr);
    sealBlock(WhileCondBB);

    setCurr(AfterWhileBB);
}

void CGProcedure::emitStmt(ReturnStatement *Stmt)
{
    if (Stmt->getRetVal())
    {
        llvm::Value *RetVal = emitExpr(Stmt->getRetVal());
        Builder.CreateRet(RetVal);
    }
    else
    {
        Builder.CreateRetVoid();
    }
}

void CGProcedure::emit(const StmtList &Stmts)
{
    for (auto *S : Stmts)
    {
        if (auto *Stmt = llvm::dyn_cast<AssignmentStatement>(S))
            emitStmt(Stmt);
        else if (auto *Stmt =
                     llvm::dyn_cast<ProcedureCallStatement>(S))
            emitStmt(Stmt);
        else if (auto *Stmt = llvm::dyn_cast<IfStatement>(S))
            emitStmt(Stmt);
        else if (auto *Stmt = llvm::dyn_cast<WhileStatement>(S))
            emitStmt(Stmt);
        else if (auto *Stmt =
                     llvm::dyn_cast<ReturnStatement>(S))
            emitStmt(Stmt);
        else
            llvm_unreachable("Unknown statement");
    }
}

void CGProcedure::run(ProcedureDeclaration *Proc)
{
    this->Proc = Proc;
    Fty = createFunctionType(Proc);
    Fn = createFunction(Proc, Fty);
    // now create the entry basic block
    llvm::BasicBlock *BB = llvm::BasicBlock::Create(
        CGM.getLLVMCtx(), // LLVM Context
        "entry",          // name of the block
        Fn                // parent function
    );
    setCurr(BB);

    size_t Idx = 0;
    //auto &Defs = CurrentDef[BB];
    for (auto I = Fn->arg_begin(),
              E = Fn->arg_end();
         I != E; ++I, ++Idx)
    {
        llvm::Argument *Arg = I;
        FormalParameterDeclaration *FP = Proc->getFormalParams()[Idx];
        // Create a map between FormalParameter -> llvm::Argument
        FormalParams[FP] = Arg;
        writeLocalVariable(Curr, FP, Arg);
    }

    for (auto *D : Proc->getDecls())
    {
        if (auto *Var = llvm::dyn_cast<VariableDeclaration>(D))
        {
            llvm::Type *Ty = mapType(Var);

            if (Ty->isAggregateType())
            {
                llvm::Value *Val = Builder.CreateAlloca(Ty);
                writeLocalVariable(Curr, Var, Val);
            }
        }
    }

    auto Block = Proc->getStmts();
    emit(Proc->getStmts());
    if (!Curr->getTerminator())
        Builder.CreateRetVoid();
    sealBlock(Curr);
}

void CGProcedure::run()
{
}