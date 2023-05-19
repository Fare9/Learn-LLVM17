#ifndef LEXER_H
#define LEXER_H

// LLVM headers for lexer
#include "llvm/ADT/StringRef.h"

// read only access to a block of memory
// to read through buffer without always checking size
#include "llvm/Support/MemoryBuffer.h"

#include <initializer_list>

class Lexer;

/// @brief Class that represents a Token from the text
class Token {
    friend class Lexer;

public:
    /**
    * Kind of tokens in this calc
    */
    enum TokenKind : unsigned short {
        eoi,                // end of input
        unknown,            // error at the lexical level
        ident,              // identifier
        number,             // number
        comma,              // comma value
        colon,              // :
        plus,               // +
        minus,              // -
        star,               // *
        slash,              // '/'
        rem,                // '%'
        l_paren,            // (
        r_paren,            // )
        KW_with
    };


private:
    // the kind of token from previous list
    TokenKind Kind;
    // start of the text of the token
    llvm::StringRef Text;

public:

    /// @brief Return the kind of token, this is a constant method
    /// since it does not modify object state.
    /// @return token type
    TokenKind getKind() const
    {
        return Kind;
    }

    /// @brief return the reference to the text that starts the token.
    /// @return reference to string with token
    llvm::StringRef getText() const
    {
        return Text;
    }

    /**
    *   @brief check if current token has same kind than parameter.
    *
    *   @param K kind of token to check.
    *   @return boolean specifying if token has specified kind.
    */
    bool is(TokenKind K) const
    {
        // simple comparison
        return Kind == K;
    }
    
    /**
    *   @brief check if current token is one of the specified types.
    *
    *   @param K1 first kind to check.
    *   @param K2 second kind to check.
    *   @result boolean specifying if token is one of the types.
    */
    bool isOneOf(TokenKind K1, TokenKind K2) const 
    {
        return is(K1) || is(K2);
    }

    /**
    *   @brief use of variadic template to check for types.
    *   This is the common way for using variadic templates.
    *   
    *   @param K1 first kind to check.
    *   @param K2 second kind to check.
    *   @param Ks N kind to check.
    *   @return boolean specifying if token is one of the given kinds.
    */
    template <typename... Ts>
    bool isOneOf(TokenKind K1, TokenKind K2, Ts... Ks) const
    {
        return is(K1) || isOneOf(K2, Ks...);
    }

    /// @brief Check if it is one from the given list using
    /// an initializer_list
    /// @param list 
    /// @return 
    bool isOneOf(std::initializer_list<TokenKind> list) const
    {
        bool is_from_list = false;

        for (auto K : list)
            is_from_list |= is(K);
        
        return is_from_list;
    }
};

/// @brief Class that will go by the tokens of the text
class Lexer
{
    const char *BufferStart;
    const char *BufferPtr;

public:
    /**
    *   @brief constructor of Lexer with a buffer to parse.
    *
    *   @param Buffer buffer with text to parse.
    */
    Lexer(const llvm::StringRef& Buffer)
    {
        BufferStart = Buffer.begin();
        BufferPtr = BufferStart;
    }

    void next(Token &token);

private:
    /**
    *   @brief create a token
    */
    void formToken(Token &Tok, const char *TokEnd, Token::TokenKind Kind);
};

#endif
