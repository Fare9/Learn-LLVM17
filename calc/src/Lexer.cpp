#include "Lexer.h"

namespace charinfo
{
    /**
    *   @brief return if the given char is a whites pace.
    *
    *   @param c current character to check.
    *   @return true if current character is a white space, false other case.
    */
    LLVM_READNONE inline bool isWhitespace(char c)
    {
        return c == ' ' ||
               c == '\t' ||
               c == '\f' ||
               c == '\v' ||
               c == '\r' ||
               c == '\n';
    }

    /**
    *   @brief check if current character is a digit.
    *
    *   @param c current character to check.
    *   @return true if c is a digit, false other case.
    */
    LLVM_READNONE inline bool isDigit(char c)
    {
        return c >= '0' && c <= '9';
    }

    /**
    *   @brief check if current character is a letter.
    *
    *   @param c current character to check.
    *   @return true if c is a letter, false other case.
    */
    LLVM_READNONE inline bool isLetter(char c)
    {
        return (c >= 'a' && c <= 'z') ||
               (c >= 'A' && c <= 'Z');
    }
}

void Lexer::next(Token &token)
{
    // check if we have characters or we finished
    while (*BufferPtr && charinfo::isWhitespace(*BufferPtr))
        ++BufferPtr; // advance when find a whitespace
    
    if (!*BufferPtr) // Did we finish the text?
    {
        // in that case end of input and return
        token.Kind = Token::eoi;
        return;
    }
    
    // we have a character, check if it is a letter
    // in this case, we detect possible keywords, or
    // identifiers.
    if (charinfo::isLetter(*BufferPtr))
    {
        // get where is the end of the token

        // we then will have the next:
        //  BufferPtr = beginning of the word
        //  end = end of the word
        const char * end = BufferPtr + 1;
        while (charinfo::isLetter(*end))
            ++end;
        // found a token end, create a stringref
        // StringRef is created using the the pointers
        llvm::StringRef Name{BufferPtr, static_cast<size_t>(end-BufferPtr)};

        // set a kind for the token
        Token::TokenKind kind = Name == "with" ? Token::KW_with : Token::ident;

        // create a token
        formToken(token, end, kind);
        return;
    }
    // now check for numbers.
    else if (charinfo::isDigit(*BufferPtr))
    {
        const char *end = BufferPtr + 1;
        while (charinfo::isDigit(*end))
            ++end;
        formToken(token, end, Token::number);
        return;
    }
    // rest of the tokens or errors
    else
    {
        switch (*BufferPtr)
        {
            // macro to check each case and assign token
            #define CASE(ch, tok) \
            case ch: formToken(token, BufferPtr + 1, tok); break
            // is it add
            CASE('+', Token::plus);
            // is it sub
            CASE('-', Token::minus);
            // is it mul
            CASE('*', Token::star);
            // is it div
            CASE('/', Token::slash);
            // is it reminder
            CASE('%', Token::rem);
            // is it exponent
            CASE('^', Token::exp);
            // is it l parenthesis
            CASE('(', Token::l_paren);
            // is it r parenthesis
            CASE(')', Token::r_paren);
            // rest
            CASE(':', Token::colon);
            CASE(',', Token::comma);
            #undef CASE
            default:
                formToken(token, BufferPtr + 1, Token::unknown);
        }
        return;
    }
}

void Lexer::formToken(Token &Tok, const char *TokEnd, Token::TokenKind Kind)
{
    Tok.Kind = Kind;
    Tok.Text = llvm::StringRef(BufferPtr, TokEnd - BufferPtr);
    // advance the bufferptr
    BufferPtr = TokEnd;
}