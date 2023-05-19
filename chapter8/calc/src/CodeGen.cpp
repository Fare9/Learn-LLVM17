#include "CodeGen.h"
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
        Type *Int8Ty;
        Type *Int32Ty;
        Type *Int64Ty;
        // pointer types for generating pointers
        PointerType *Int8PtrTy;
        PointerType *Int32PtrTy;
        PointerType *Int8PtrPtrTy;
        PointerType *Int64PtrTy;
        Constant *Int32Zero;

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

        Value *V;
        // map name with value
        StringMap<Value *> nameMap;

    public:
        /**
         * @brief Constructor to initialize all the used types in the program.
         * we also initialize the module and the IRBuilder.
         */
        ToIRVisitor(Module *M) : M(M), Builder(M->getContext())
        {
            // the types
            VoidTy = Type::getVoidTy(M->getContext());
            Int8Ty = Type::getInt8Ty(M->getContext());
            Int32Ty = Type::getInt32Ty(M->getContext());
            Int64Ty = Type::getInt64Ty(M->getContext());
            // the pointer types!
            Int8PtrTy = PointerType::get(Int8Ty, 0);
            Int32PtrTy = Type::getInt32PtrTy(M->getContext());
            Int8PtrPtrTy = Int8PtrTy->getPointerTo();
            Int64PtrTy = Type::getInt64PtrTy(M->getContext());

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
                Int32Ty, {Int32Ty, Int8PtrPtrTy}, false);
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
            FunctionType *ReadFty = FunctionType::get(Int32Ty, {Int8PtrTy}, false);
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
                Value *Ptr = Builder.CreateInBoundsGEP(Str->getType(), Str, {Int32Zero}, "ptr");
                CallInst *Call = Builder.CreateCall(ReadFty, ReadFn, {Ptr});
                nameMap[Var] = Call;
            }

            Node.getExpr()->accept(*this);
        }

        /// @brief Function to visit a Factor node of the AST
        /// set the value to appropiate one
        /// @param Node
        virtual void visit(Factor &Node) override
        {
            if (Node.getKind() == Factor::Ident)
                V = nameMap[Node.getVal()];
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
            }
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
                    Int8PtrTy,
                    /*isConstant*/ true,
                    /*Linking*/ GlobalValue::ExternalLinkage,
                    /*initializer*/ nullptr,
                    "_ZTIi"); // exception type

                // Declare the __cxa_allocate_exception function.
                createFunc(AllocEHFty,                 // Type of function
                           AllocEHFn,                  // Function itself
                           "__cxa_allocate_exception", // name of the function
                           Int8PtrTy,                  // return type
                           {Int64Ty});                 // parameter list

                // Declare the __cxa_throw function.
                createFunc(ThrowEHFty,                         // Type of function
                           ThrowEHFn,                          // Function itself
                           "__cxa_throw",                      // name of the function
                           VoidTy,                             // return void
                           {Int8PtrTy, Int8PtrTy, Int8PtrTy}); // parameters of the function

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
            Value *PayloadPtr = Builder.CreateBitCast(EH, Int32PtrTy);
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
                {EH, ConstantExpr::getBitCast(TypeInfo, Int8PtrTy), ConstantPointerNull::get(Int8PtrTy)} // parameters
            );
        }

        /// @brief Create the landing pad for the exception
        void addLandingPad()
        {
            FunctionType *TypeIdFty;
            Function *TypeIdFn;

            // function to obtain the typeid of an exception
            createFunc(TypeIdFty, TypeIdFn, "llvm.eh.typeid.for", Int32Ty, {Int8PtrTy});

            // function called at the beginning of catch block
            FunctionType *BeginCatchFty;
            Function *BeginCatchFn;
            createFunc(BeginCatchFty, BeginCatchFn, "__cxa_begin_catch", Int8PtrTy, {Int8PtrTy});

            // function called at the end of the catch block
            FunctionType *EndCatchFty;
            Function *EndCatchFn;
            createFunc(EndCatchFty, EndCatchFn, "__cxa_end_catch", VoidTy);

            // Function for calling Puts with the error message.
            FunctionType *PutsFty;
            Function *PutsFn;
            createFunc(PutsFty, PutsFn, "puts", Int32Ty, {Int8PtrTy});

            LandingPadInst *Exc = Builder.CreateLandingPad(
                StructType::get(Int8PtrTy, Int32Ty), 1, "exc");
            Exc->addClause(ConstantExpr::getBitCast(TypeInfo, Int8PtrTy));
            // now create a call to llvm.eh.typeid.for for extracting the TypeInfo from
            // the exception
            Value *Sel = Builder.CreateExtractValue(Exc, {1}, "exc.sel");
            CallInst *Id = Builder.CreateCall(TypeIdFty, TypeIdFn, {ConstantExpr::getBitCast(TypeInfo, Int8PtrTy)});

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
            Value *MsgPtr = Builder.CreateGlobalStringPtr(
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
    Module *M = new Module("calc.expr", Ctx);
    ToIRVisitor ToIR(M);
    ToIR.run(Tree);
    M->print(outs(), nullptr);
}