#ifndef TINYLANG_LEXER_TOKEN_H
#define TINYLANG_LEXER_TOKEN_H

#include "tinylang/Basic/LLVM.h"
#include "tinylang/Basic/TokenKinds.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/SMLoc.h"

namespace tinylang
{
    class Lexer;

    /// @brief This class represent a token in the code, the tokens are defined by a type from the enum TokenKind
    class Token
    {
        friend class Lexer;

        const char *Ptr; // pointer to token

        size_t Length;   // length of token

        // Actual flavor of token
        tok::TokenKind Kind;

    public:
        tok::TokenKind getKind() const
        {
            return Kind;
        }

        void setKind(tok::TokenKind K)
        {
            Kind = K;
        }

        /// checks of is/isNot, same as calc
        /// we will use this to check for tokens
        bool is(tok::TokenKind K) const
        {
            return Kind == K;
        }

        bool isNot(tok::TokenKind K) const
        {
            return Kind != K;
        }

        bool isOneOf(tok::TokenKind K1, tok::TokenKind K2) const
        {
            return is(K1) || is(K2);
        }

        template<typename... Ts>
        bool isOneOf(tok::TokenKind K1, tok::TokenKind K2, Ts... Ks) const
        {
            return is(K1) || isOneOf(K2, Ks...);
        }

        const char * getName() const
        {
            return tok::getTokenName(Kind);
        }

        size_t getLength() const
        {
            return Length;
        }

        SMLoc getLocation() const
        {
            return SMLoc::getFromPointer(Ptr);
        }

        StringRef getIdentifier()
        {
            assert(is(tok::identifier) && "Cannot get identifier of non-identifier");
            return StringRef(Ptr, Length);
        }

        StringRef getLiteralData()
        {
            assert(isOneOf(tok::integer_literal, tok::string_literal) && "Cannot get literal data of non-literal");
            return StringRef(Ptr, Length);
        }
    };
} //! namespace tinylang

#endif