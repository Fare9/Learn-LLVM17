#include "Sema.h"
#include "llvm/ADT/StringSet.h"

#include "llvm/Support/raw_ostream.h"

namespace
{
    class DeclCheck : public ASTVisitor
    {
        llvm::StringSet<> Scope;
        bool HasError;

        enum ErrorType
        {
            Twice,
            Not
        };

        void error(ErrorType ET, llvm::StringRef V)
        {
            llvm::errs() << "Variable " << V << " "
                         << (ET == Twice ? "already" : "not")
                         << " declared\n";
            HasError = true;
        }

    public:
        DeclCheck() : HasError(false) {}

        bool hasError() const
        {
            return HasError;
        }

        /**
         * Check that a factor node is in the scope
         */
        virtual void visit(Factor &Node) override
        {
            if (Node.getKind() == Factor::Ident)
            {
                if (Scope.find(Node.getVal()) == Scope.end())
                    error(Not, Node.getVal());
            }
        }

        /**
         * Check that a binary operation nodes exists
         * and have been visited
         */
        virtual void visit(BinaryOp &Node) override
        {
            if (Node.getLeft())
                Node.getLeft()->accept(*this);
            else
                HasError = true;
            if (Node.getRight())
                Node.getRight()->accept(*this);
            else
                HasError = true;
        }

        /**
         * for the variables that have been inserted with With,
         * introduce them in the scope
        */
        virtual void visit(WithDecl &Node) override 
        {
            // go through the variables
            for (auto I = Node.begin(), E = Node.end(); I != E; ++I)
            {
                // check if it was already inserted, in that case
                // raise the error
                if (!Scope.insert(*I).second)
                    error(Twice, *I);
            }
            if (Node.getExpr())
                Node.getExpr()->accept(*this);
            else
                HasError = true;
        }
    };
}

bool Sema::semantic(AST *Tree)
{
    if (!Tree)
        return false;
    DeclCheck Check;
    Tree->accept(Check);
    return Check.hasError();
}