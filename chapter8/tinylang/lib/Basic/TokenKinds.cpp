#include "tinylang/Basic/TokenKinds.h"
#include "llvm/Support/ErrorHandling.h"

using namespace tinylang;

/// @brief Name of the tokens
static const char *const TokNames[] = {
#define TOK(ID) #ID,
#define KEYWORD(ID, FLAG) #ID,
#include "tinylang/Basic/TokenKinds.def"
    nullptr // last TokNames
};

const char *tok::getTokenName(TokenKind Kind)
{
    return TokNames[Kind];
}

const char *tok::getPunctuatorSpelling(TokenKind Kind)
{
    switch (Kind)
    {
#define PUNCTUATOR(ID, SP) \
    case ID:               \
        return SP;
#include "tinylang/Basic/TokenKinds.def"
    default:
        break;
    }

    return nullptr;
}

const char *tok::getKeywordSpelling(TokenKind Kind)
{
    switch (Kind)
    {
#define KEYWORD(ID, FLAG) \
    case kw_##ID:         \
        return #ID;
#include "tinylang/Basic/TokenKinds.def"
    default:
        break;
    }

    return nullptr;
}