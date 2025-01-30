#include "CodeGen.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/DerivedTypes.h"  // for PointerType
#include "llvm/IR/Module.h"
#include "llvm/ADT/StringMap.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/Support/raw_ostream.h"

// for llvm utilities
using namespace llvm;

namespace
{
    class ToIRVisitor : public ASTVisitor
    {
        Module *M;
        IRBuilder<> Builder;
        Type *VoidTy;
        Type *Int32Ty;
        Type *Int64Ty;
        PointerType *PtrTy;
        Constant *Int32Zero;
        Value *V;
        // map name with value
        StringMap<Value *> nameMap;

        // new types to keep some C++
        // exception handling functions.
        GlobalVariable *TypeInfo = nullptr;
        /// @brief __cxa_allocate_exception function both
        /// type and function
        FunctionType *AllocEHFty = nullptr;
        Function *AllocEHFn = nullptr;
        /// @brief __cxa_throw function type and
        /// function
        FunctionType *ThrowEHFty = nullptr;
        Function *ThrowEHFn = nullptr;
        /// @brief Landing pad in case of exception
        BasicBlock *LPadBB = nullptr;
        /// @brief in case exception is not handled unreachable path
        BasicBlock *UnreachableBB = nullptr;

        

    public:
        /**
         * @brief Constructor to initialize all the used types in the program.
         * we also initialize the module and the IRBuilder.
         */
        ToIRVisitor(Module *M) : M(M), Builder(M->getContext())
        {
            // the types
            VoidTy = Type::getVoidTy(M->getContext());
            Int32Ty = Type::getInt32Ty(M->getContext());
            Int64Ty = Type::getInt64Ty(M->getContext());
            // the pointer types!
            PtrTy = PointerType::getUnqual(M->getContext());
            Int32Zero = ConstantInt::get(Int32Ty, 0, true);
        }

        /**
         * @brief Create the code for the AST, we need to create
         *        for each translation unit a Function.
         */
        void run(AST *Tree)
        {
            // create function type for Main
            FunctionType *MainFty = FunctionType::get(
                Int32Ty, {Int32Ty, PtrTy}, false);
            // now create a Main function
            Function *MainFn = Function::Create(MainFty, GlobalValue::ExternalLinkage, "main", M);

            // now time to create the basic blocks

            // first basic block, entry basic block
            BasicBlock *BB = BasicBlock::Create(M->getContext(), "entry", MainFn);

            Builder.SetInsertPoint(BB); // where created instructions must be inserted

            Tree->accept(*this);

            // now create the calc_write function to print the output of the instruction
            FunctionType *CalcWriteFnTy = FunctionType::get(VoidTy, {Int32Ty}, false);
            Function *CalcWriteFn = Function::Create(CalcWriteFnTy, GlobalValue::ExternalLinkage, "calc_write", M);
            Builder.CreateCall(CalcWriteFnTy, CalcWriteFn, {V});

            // Main finishes returning zero
            Builder.CreateRet(Int32Zero);
        }

        /**
         * @brief visit method to generate LLVM IR of WithDecl node.
         *
         * @param Node WithDecl node to generate IR
         */
        virtual void visit(WithDecl &Node) override
        {
            // type of read function
            FunctionType *ReadFty = FunctionType::get(Int32Ty, {PtrTy}, false);
            // function for reading
            Function *ReadFn = Function::Create(ReadFty, GlobalValue::ExternalLinkage, "calc_read", M);

            // go through the variables, reading each one.
            for (auto I = Node.begin(), E = Node.end(); I != E; ++I)
            {
                StringRef Var = *I;
                Constant *StrText = ConstantDataArray::getString(M->getContext(), Var);
                GlobalVariable *Str = new GlobalVariable(
                    *M,
                    StrText->getType(),
                    /*isConstant*/ true,
                    GlobalValue::PrivateLinkage,
                    StrText,
                    Twine(Var).concat(".str"));
                // now create the call to calc_read() with the just created variable
                CallInst *Call = Builder.CreateCall(ReadFty, ReadFn, {Str});
                nameMap[Var] = Call;
            }

            Node.getExpr()->accept(*this);
        }

        /// @brief Function to visit a Factor node of the AST
        /// set the value to appropiate one
        /// @param Node
        virtual void visit(Factor &Node) override
        {
            // If the Factor is an identifier
            // use the memory where the variable
            // is stored
            if (Node.getKind() == Factor::Ident)
                V = nameMap[Node.getVal()];
            // In other case, create an integer
            // value to use
            else
            {
                int intval;
                Node.getVal().getAsInteger(10, intval);
                V = ConstantInt::get(Int32Ty, intval, true);
            }
        }

    
        virtual void visit(BinaryOp &Node) override
        {
            Node.getLeft()->accept(*this);
            Value *Left = V;
            Node.getRight()->accept(*this);
            Value *Right = V;
            switch (Node.getOperator())
            {
            case BinaryOp::Plus:
                V = Builder.CreateNSWAdd(Left, Right);
                break;
            case BinaryOp::Minus:
                V = Builder.CreateNSWSub(Left, Right);
                break;
            case BinaryOp::Mul:
                V = Builder.CreateNSWMul(Left, Right);
                break;
            /// The case of DIV in the calculator, it can generate
            /// an exception when dividing by 0
            case BinaryOp::Div:
            {
                BasicBlock *TrueDest, *FalseDest;
                createICmpEq(Right, Int32Zero, TrueDest, FalseDest,
                             "divbyzero", "notzero");
                Builder.SetInsertPoint(TrueDest);
                addThrow(42); // an arbitrary payload value
                // where to fall in case value is different to 0
                Builder.SetInsertPoint(FalseDest);
                V = Builder.CreateSDiv(Left, Right);
            }
                break;
            /// In the case of REM is similar to DIV, it can generate
            /// an exception when dividing by 0
            case BinaryOp::Rem:
            {
                BasicBlock *TrueDest, *FalseDest;
                /// create a comparison by zero
                createICmpEq(Right, Int32Zero, TrueDest, FalseDest, "rembyzero", "remnotzero");
                Builder.SetInsertPoint(TrueDest);
                // arbitrary payload to throw exception
                // in case division by zero
                addThrow(42);
                // in case the value is different to zero
                Builder.SetInsertPoint(FalseDest);
                // create a rem instruction
                V = Builder.CreateSRem(Left, Right);
            }
                break;
            case BinaryOp::Exp:
                V = generateExponent(Left, Right);
                break;
            }
        }

        /// @brief Generate an exponent operation as a loop, and a multiplication.
        /// @param base operand which represents the base
        /// @param exponent operand which represents the exponent
        /// @return returned value
        llvm::Value* generateExponent(llvm::Value* base, llvm::Value* exponent) {
            // Get current function and context
            llvm::Function* function = Builder.GetInsertBlock()->getParent();
            llvm::LLVMContext& context = function->getContext();

            // If exponent is a constant number
            if (auto* constInt = llvm::dyn_cast<llvm::ConstantInt>(exponent)) {
                uint64_t exp_val = constInt->getZExtValue();
                if (exp_val == 0) {
                    return llvm::ConstantInt::get(Int32Ty, 1);
                }

                // For small constants (< 10), directly expand the multiplication
                if (exp_val < 10) {
                    llvm::Value* result = base;
                    switch(exp_val) {
                        case 1: return base;
                        case 2: return Builder.CreateNSWMul(base, base);
                        case 3: {
                            auto* temp = Builder.CreateNSWMul(base, base);
                            return Builder.CreateNSWMul(temp, base);
                        }
                        case 4: {
                            auto* temp = Builder.CreateNSWMul(base, base);
                            return Builder.CreateNSWMul(temp, temp);
                        }
                        case 5: {
                            auto* temp = Builder.CreateNSWMul(base, base);
                            auto* temp2 = Builder.CreateNSWMul(temp, temp);
                            return Builder.CreateNSWMul(temp2, base);
                        }
                        default: {
                            // For 6-9, use sequential multiplication
                            llvm::Value* result = base;
                            for (uint64_t i = 1; i < exp_val; i++) {
                                result = Builder.CreateNSWMul(result, base);
                            }
                            return result;
                        }
                    }
                }
                
                // For larger constants, use a loop
                llvm::BasicBlock* currentBlock = Builder.GetInsertBlock();
                llvm::BasicBlock* loopBlock = llvm::BasicBlock::Create(context, "exp.loop", function);
                llvm::BasicBlock* exitBlock = llvm::BasicBlock::Create(context, "exp.exit", function);
                
                // Initial setup
                llvm::Value* counter = Builder.CreateAlloca(Int32Ty);
                Builder.CreateStore(exponent, counter);
                llvm::Value* result = base;
                
                Builder.CreateBr(loopBlock);
                
                // Loop block
                Builder.SetInsertPoint(loopBlock);
                llvm::PHINode* resultPhi = Builder.CreatePHI(base->getType(), 2, "exp.result");
                resultPhi->addIncoming(result, currentBlock);
                
                llvm::Value* newResult = Builder.CreateNSWMul(resultPhi, base);
                
                llvm::Value* currentCount = Builder.CreateLoad(Int32Ty, counter);
                llvm::Value* newCount = Builder.CreateSub(currentCount, 
                    llvm::ConstantInt::get(Int32Ty, 1));
                Builder.CreateStore(newCount, counter);
                
                resultPhi->addIncoming(newResult, loopBlock);
                
                llvm::Value* condition = Builder.CreateICmpSGT(newCount, 
                    llvm::ConstantInt::get(Int32Ty, 1));
                Builder.CreateCondBr(condition, loopBlock, exitBlock);
                
                // Exit block
                Builder.SetInsertPoint(exitBlock);
                return newResult;
            }

            // For variables, generate the comparison and loop logic
            llvm::BasicBlock* entryBlock = Builder.GetInsertBlock();
            llvm::BasicBlock* loopBlock = llvm::BasicBlock::Create(context, "exp.loop", function);
            llvm::BasicBlock* exitBlock = llvm::BasicBlock::Create(context, "exp.exit", function);
            
            // Check if exponent is zero
            llvm::BasicBlock* zeroTrueBlock;
            llvm::BasicBlock* zeroFalseBlock;
            createICmpEq(exponent, Int32Zero, zeroTrueBlock, zeroFalseBlock);
            
            // If exponent is zero, return 1
            Builder.SetInsertPoint(zeroTrueBlock);
            llvm::Value* one = llvm::ConstantInt::get(base->getType(), 1);
            Builder.CreateBr(exitBlock);

            // Main computation block
            Builder.SetInsertPoint(zeroFalseBlock);

            // Initialize counter and result
            llvm::Value* counter = Builder.CreateAlloca(Int32Ty);
            Builder.CreateStore(exponent, counter);
            llvm::Value* result = base;

            // Branch to loop
            Builder.CreateBr(loopBlock);

            // Loop block
            Builder.SetInsertPoint(loopBlock);
            llvm::PHINode* resultPhi = Builder.CreatePHI(base->getType(), 2, "exp.result");
            resultPhi->addIncoming(result, zeroFalseBlock);

            // Multiply base by itself
            llvm::Value* newResult = Builder.CreateNSWMul(resultPhi, base);

            // Decrement counter
            llvm::Value* currentCount = Builder.CreateLoad(Int32Ty, counter);
            llvm::Value* newCount = Builder.CreateSub(currentCount, 
                llvm::ConstantInt::get(Int32Ty, 1));
            Builder.CreateStore(newCount, counter);

            // Update PHI node
            resultPhi->addIncoming(newResult, loopBlock);

            // Check if counter > 1
            llvm::Value* condition = Builder.CreateICmpSGT(newCount, 
                llvm::ConstantInt::get(Int32Ty, 1));
            Builder.CreateCondBr(condition, loopBlock, exitBlock);

            // Exit block
            Builder.SetInsertPoint(exitBlock);
            llvm::PHINode* finalResult = Builder.CreatePHI(base->getType(), 2, "exp.final");
            finalResult->addIncoming(one, zeroTrueBlock);
            finalResult->addIncoming(newResult, loopBlock);

            return finalResult;
        }

        /// @brief Create a comparison instruction to check in the
        /// division if user is dividing by zero.
        /// @param Left
        /// @param Right
        /// @param TrueDest
        /// @param FalseDest
        /// @param TrueLabel
        /// @param FalseLabel
        void createICmpEq(Value *Left,
                          Value *Right,
                          BasicBlock *&TrueDest,
                          BasicBlock *&FalseDest,
                          const Twine &TrueLabel = "",
                          const Twine &FalseLabel = "")
        {
            // get a pointer to the function
            Function *Fn = Builder.GetInsertBlock()->getParent();
            // now create two blocks for the branches
            TrueDest = BasicBlock::Create(M->getContext(), TrueLabel, Fn);
            FalseDest = BasicBlock::Create(M->getContext(), FalseLabel, Fn);
            // create a comparison, this must be a comparison if equal
            Value *Cmp = Builder.CreateCmp(CmpInst::ICMP_EQ, Left, Right);
            Builder.CreateCondBr(Cmp, TrueDest, FalseDest);
        }

        /// @brief Helper function to create a LLVM Function.
        /// @param Fty Reference of a Pointer to a FunctionType we will create
        /// @param Fn Reference of a Pointer to a Function where function will be created
        /// @param N Name of the function
        /// @param Result Return type of the function
        /// @param Params Parameter types of the function
        /// @param IsVarArgs 
        void createFunc(FunctionType *&Fty, Function *&Fn, const Twine &N,
                        Type *Result, ArrayRef<Type *> Params = {}, bool IsVarArgs = false)
        {
            // create the function type
            Fty = FunctionType::get(Result, Params, IsVarArgs);
            // now create the function
            Fn = Function::Create(Fty, GlobalValue::ExternalLinkage, N, M);
        }

        /// @brief Add the throw instruction in the code
        /// @param PayloadVal value of the payload to store in exception.
        void addThrow(int PayloadVal)
        {
            if (!TypeInfo) // if no info
            {
                // create a global variable
                // of type I8*, and the exception type _ZTIi
                // which is the type information of integer
                TypeInfo = new GlobalVariable(
                    *M,
                    PtrTy,
                    /*isConstant*/ true,
                    /*Linking*/ GlobalValue::ExternalLinkage,
                    /*initializer*/ nullptr,
                    "_ZTIi"); // exception type

                // Declare the __cxa_allocate_exception function.
                createFunc(AllocEHFty,                 // Type of function
                           AllocEHFn,                  // Function itself
                           "__cxa_allocate_exception", // name of the function
                           PtrTy,                      // return type
                           {Int64Ty});                 // parameter list

                // Declare the __cxa_throw function.
                createFunc(ThrowEHFty,                         // Type of function
                           ThrowEHFn,                          // Function itself
                           "__cxa_throw",                      // name of the function
                           VoidTy,                             // return void
                           {PtrTy, PtrTy, PtrTy}); // parameters of the function

                // Declare personality function.
                // this function will be used for stack unwinding on Linux
                FunctionType *PersFty;
                Function *PersFn;
                // function comes from libunwind
                createFunc(PersFty, PersFn, "__gxx_personality_v0", Int32Ty, {}, true);

                // Attach personality function to main()
                Function *Fn = Builder.GetInsertBlock()->getParent();
                Fn->setPersonalityFn(PersFn);

                // Create and populate the landingpad block and
                // the resume block. This is the block where the
                // function called falls if there was an exception
                // it contains the catch code

                // first store the current basic block
                BasicBlock *SaveBB = Builder.GetInsertBlock();

                // now create the landing pad
                LPadBB = BasicBlock::Create(M->getContext(), "lpad", Fn);
                Builder.SetInsertPoint(LPadBB);
                addLandingPad();

                // Unreachable basic block, after throwing exception
                UnreachableBB = BasicBlock::Create(M->getContext(), "unreachable", Fn);
                Builder.SetInsertPoint(UnreachableBB);
                Builder.CreateUnreachable();

                // return to the stored basic block
                Builder.SetInsertPoint(SaveBB);
            }

            // size of payload is 4 bytes (an integer)
            Constant *PayloadSz = ConstantInt::get(Int64Ty, 4, false);

            // Call the function to allocate space for the exception
            CallInst *EH = Builder.CreateCall(AllocEHFty, AllocEHFn, {PayloadSz});

            // Store payload value
            Value *PayloadPtr = Builder.CreateBitCast(EH, PtrTy);
            Builder.CreateStore(ConstantInt::get(Int32Ty, PayloadVal, true), PayloadPtr);

            // Raise the exception with a call to __cxa_throw function
            // this call instead of using call instruction it uses invoke
            // invoke is a last instruction of block, since two blocks
            // follows it, the Landing Pad and the Unreadchable
            Builder.CreateInvoke(
                ThrowEHFty,                                                                              // function type
                ThrowEHFn,                                                                               // function
                UnreachableBB,                                                                           // block after invoke
                LPadBB,                                                                                  // block where flow jumps with exception
                {EH, ConstantExpr::getBitCast(TypeInfo, PtrTy), ConstantPointerNull::get(PtrTy)} // parameters
            );
        }

        /// @brief Create the landing pad for the exception
        void addLandingPad()
        {
            FunctionType *TypeIdFty;
            Function *TypeIdFn;

            // function to obtain the typeid of an exception
            createFunc(TypeIdFty, TypeIdFn, "llvm.eh.typeid.for", Int32Ty, {PtrTy});

            // function called at the beginning of catch block
            FunctionType *BeginCatchFty;
            Function *BeginCatchFn;
            createFunc(BeginCatchFty, BeginCatchFn, "__cxa_begin_catch", PtrTy, {PtrTy});

            // function called at the end of the catch block
            FunctionType *EndCatchFty;
            Function *EndCatchFn;
            createFunc(EndCatchFty, EndCatchFn, "__cxa_end_catch", VoidTy);

            // Function for calling Puts with the error message.
            FunctionType *PutsFty;
            Function *PutsFn;
            createFunc(PutsFty, PutsFn, "puts", Int32Ty, {PtrTy});

            LandingPadInst *Exc = Builder.CreateLandingPad(
                StructType::get(PtrTy, Int32Ty), 1, "exc");
            Exc->addClause(ConstantExpr::getBitCast(TypeInfo, PtrTy));
            // now create a call to llvm.eh.typeid.for for extracting the TypeInfo from
            // the exception
            Value *Sel = Builder.CreateExtractValue(Exc, {1}, "exc.sel");
            CallInst *Id = Builder.CreateCall(TypeIdFty, TypeIdFn, {ConstantExpr::getBitCast(TypeInfo, PtrTy)});

            // Now we generate the IR for the comparison with the int type
            BasicBlock *TrueDest, *FalseDest;
            createICmpEq(Sel, Id, TrueDest, FalseDest, "match", "resume");

            // in case the two values do not match, we go to this code
            Builder.SetInsertPoint(FalseDest);
            Builder.CreateResume(Exc);

            // And in case the two values are equal we go to the
            // next block
            Builder.SetInsertPoint(TrueDest);
            Value *Ptr = Builder.CreateExtractValue(Exc, {0}, "exc.ptr");
            Builder.CreateCall(BeginCatchFty, BeginCatchFn, {Ptr});

            // to handle the exception we will just print
            // a message to the user
            Value *MsgPtr = Builder.CreateGlobalString(
                "Divide by zero!",
                "msg",
                0,
                M);

            Builder.CreateCall(PutsFty, PutsFn, {MsgPtr});

            // and to finish the catch exception, we will
            // call the end of catch from C++
            Builder.CreateCall(EndCatchFty, EndCatchFn);
            Builder.CreateRet(Int32Zero);
        }
    };
}

void CodeGen::compile(AST *Tree)
{
    LLVMContext Ctx;
    auto M = std::make_unique<Module>("calc.expr", Ctx);
    ToIRVisitor ToIR(M.get());
    ToIR.run(Tree);
    M->print(outs(), nullptr);
}