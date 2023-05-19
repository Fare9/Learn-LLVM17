#ifndef TINYLANG_LEXER_LEXER_H
#define TINYLANG_LEXER_LEXER_H

#include "tinylang/Basic/Diagnostic.h"
#include "tinylang/Basic/LLVM.h"
#include "tinylang/Lexer/Token.h"
#include "llvm/ADT/StringMap.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/SourceMgr.h"

namespace tinylang
{
    class KeywordFilter
    {
        /// @brief Map of strings and keywords
        llvm::StringMap<tok::TokenKind> HashTable;
        /// @brief Create an entry in HashTable with the name of the keyword and the token type
        /// @param Keyword name of keyword
        /// @param TokenCode token of keyword
        void addKeyword(StringRef Keyword, tok::TokenKind TokenCode);
    
    public:
        /// @brief Add all the keywords to the hash table
        void addKeywords();

        /// @brief Get a keyword by the name or return a default token
        /// @param Name name of the keyword
        /// @param DefaultTokenCode default token to return
        /// @return requested keyword or default token
        tok::TokenKind getKeyword(StringRef Name, tok::TokenKind DefaultTokenCode = tok::unknown)
        {
            auto Result = HashTable.find(Name);
            if (Result != HashTable.end())
                return Result->second;
            return DefaultTokenCode;
        }
    };

    class Lexer
    {
        /// @brief Source manager from LLVM
        SourceMgr &SrcMgr;
        /// @brief Management of diagnostic errors
        DiagnosticsEngine &Diags;

        /// @brief Current pointer in the file for parsing
        const char *CurPtr;
        /// @brief Buffer of text
        StringRef CurBuf;

        /// CurBuffer - buffer index we're
        /// lexing from as managed by SOurceMgr object
        unsigned CurBuffer = 0;

        /// @brief Filter for managing the keywords of the language
        KeywordFilter Keywords;

    public:
        /// @brief A lexer is a class that manages a buffer with tokens, this buffer with source code will be traversed parsing tokens
        /// @param SrcMgr manager for the file
        /// @param Diags error diagnostic object
        Lexer(SourceMgr &SrcMgr, DiagnosticsEngine &Diags) : SrcMgr(SrcMgr), Diags(Diags)
        {
            // ID of the main file buffer
            CurBuffer = SrcMgr.getMainFileID();
            // current buffer with file to parse
            CurBuf = SrcMgr.getMemoryBuffer(CurBuffer)->getBuffer();
            // pointer to the buffer
            CurPtr = CurBuf.begin();
            // initialize the keywords
            Keywords.addKeywords();
        }

        DiagnosticsEngine &getDiagnostics() const
        {
            return Diags;
        }

        /// @brief Parse the buffer and always return the next token found, it is a recursive descent parser
        /// @param Result 
        void next(Token &Result);

        /// Get source code buffer.
        StringRef getBuffer() const
        {
            return CurBuf;
        }

    private:
        /// @brief Check that next token is an identifier and return an ident token.
        /// @param Result 
        void identifier(Token &Result);
        /// @brief Check next token is a number and in that case return it.
        /// @param Result 
        void number(Token &Result);
        /// @brief Check next token is a string and in that case return it.
        /// @param Result 
        void string(Token &Result);
        /// @brief check comment tokens, and skip them.
        void comment();

        /// @brief get location in code from the current token
        /// @return 
        SMLoc getLoc() const
        {
            return SMLoc::getFromPointer(CurPtr);
        }

        /// @brief Create a token object given a type and a string
        /// @param Result 
        /// @param TokEnd end of the string with the current token
        /// @param Kind type of token
        void formToken(Token &Result, const char *TokEnd, tok::TokenKind Kind);
    };
} //! namespace tinylang

#endif