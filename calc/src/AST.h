#ifndef AST_H
#define AST_H

#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringRef.h"

class AST;
class Expr;
class Factor;
class BinaryOp;
class WithDecl;

class ASTVisitor
{
public:
    virtual void visit(AST &){};
    virtual void visit(Expr &){};
    virtual void visit(Factor &) = 0; // virtual function member, must be overrided
    virtual void visit(BinaryOp &) = 0;
    virtual void visit(WithDecl &) = 0;
};

/**
 * @brief AST class which represent the root of the tree
 */
class AST
{
public:
    virtual ~AST() {};
    virtual void accept(ASTVisitor &V) = 0;
};

/**
 * @brief Expr represent the root for AST classes of expressions.
 */
class Expr : public AST
{
public:
    Expr() {}
};

/**
 * @brief store a number or the name of a variable
 */
class Factor : public Expr
{
public:
    enum ValueKind
    {
        Ident,
        Number
    };

private:
    ValueKind Kind;
    llvm::StringRef Val;

public:
    Factor(ValueKind Kind, llvm::StringRef Val) : Kind(Kind), Val(Val)
    {
    }

    ValueKind getKind() const
    {
        return Kind;
    }

    llvm::StringRef getVal() const
    {
        return Val;
    }

    virtual void accept(ASTVisitor &V) override
    {
        V.visit(*this);
    }
};

/**
 * @brief Store the binary operation of an expression
 */
class BinaryOp : public Expr
{
public:
    enum Operator
    {
        Plus,  // a + b
        Minus, // a - b
        Mul,   // a * b
        Rem,   // a % b
        Div,   // a / b
        Exp,   // a ^ b
    };

private:
    Expr *Left;  // left side of the binary operation
    Expr *Right; // right side of the binary operation
    Operator Op;

public:
    BinaryOp(Operator Op, Expr *L, Expr *R) : Op(Op), Left(L), Right(R) {}

    Expr *getLeft() const
    {
        return Left;
    }

    Expr *getRight() const
    {
        return Right;
    }

    Operator getOperator() const
    {
        return Op;
    }

    virtual void accept(ASTVisitor &V) override
    {
        V.visit(*this);
    }
};

class WithDecl : public AST
{
    using VarVector = llvm::SmallVector<llvm::StringRef, 8>;
    VarVector Vars;
    Expr *E;

public:
    WithDecl(VarVector Vars, Expr *E) : Vars(Vars), E(E)
    {
    }

    VarVector::const_iterator begin()
    {
        return Vars.begin();
    }

    VarVector::const_iterator end()
    {
        return Vars.end();
    }

    Expr *getExpr()
    {
        return E;
    }

    virtual void accept(ASTVisitor &V) override
    {
        V.visit(*this);
    }
};

#endif