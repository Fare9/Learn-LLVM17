/**
 * @file Sema.h
 * @brief This calls although not always necessary, it will implement a semantic
 * analysis in the AST.
 */
#ifndef SEMA_H
#define SEMA_H

#include "AST.h"
#include "Lexer.h"

class Sema
{
public:
    bool semantic(AST *Tree);
};

#endif