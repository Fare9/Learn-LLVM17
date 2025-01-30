#include "tinylang/Lexer/Lexer.h"

using namespace tinylang;

void KeywordFilter::addKeyword(StringRef Keyword, tok::TokenKind TokenCode)
{
    HashTable.insert(std::make_pair(Keyword, TokenCode));
}

void KeywordFilter::addKeywords()
{
#define KEYWORD(NAME, FLAGS) addKeyword(StringRef(#NAME), tok::kw_##NAME);
#include "tinylang/Basic/TokenKinds.def"
}

/// @brief Namespace with utilities to check characters
namespace charinfo
{
    /**
     * @brief Check if character is an ASCII char.
     *
     * @param Ch character to test
     * @return `true` if character is ascii, `false` other case
     */

    /// @brief Check if character is inside of the ASCII characterset
    /// @param Ch character to check
    /// @return boolean indicating if is ASCII
    LLVM_READNONE inline bool isASCII(char Ch)
    {
        return static_cast<unsigned char>(Ch) <= 127;
    }

    /// @brief Check if we are moving to a new or beginning of a line
    /// @param Ch character to check
    /// @return true in case of vertical whitespace
    LLVM_READNONE inline bool isVerticalWhitespace(char Ch)
    {
        return isASCII(Ch) && (Ch == '\r' || Ch == '\n');
    }

    /// @brief Check if character is a horizontal white space
    /// @param Ch character to check
    /// @return true in case is a horizontal white space
    LLVM_READNONE inline bool isHorizontalWhitespace(char Ch)
    {
        return isASCII(Ch) && (Ch == ' ' || Ch == '\t' || Ch == '\f' || Ch == '\v');
    }

    /// @brief Is in general a whitespace? these are mostly bypassed
    /// @param Ch character to check
    /// @return true in case some kind of whitespace
    LLVM_READNONE inline bool isWhitespace(char Ch)
    {
        return isHorizontalWhitespace(Ch) || isVerticalWhitespace(Ch);
    }

    /// @brief Check of character to know if it's a character digit
    /// @param Ch
    /// @return
    LLVM_READNONE inline bool isDigit(char Ch)
    {
        return isASCII(Ch) && Ch >= '0' && Ch <= '9';
    }

    /// @brief Check if current character is a hex digit, for that it must
    /// be a digit or a value between 'A' and 'F'
    /// @param Ch
    /// @return
    LLVM_READNONE inline bool isHexDigit(char Ch)
    {
        return isASCII(Ch) && (isDigit(Ch) || (Ch >= 'A' && Ch <= 'F'));
    }

    /// @brief Check if current character is a possible identifier, for
    /// that, this value must be an under line, or a letter.
    /// @param Ch
    /// @return
    LLVM_READNONE inline bool isIdentifierHead(char Ch)
    {
        return isASCII(Ch) && ((Ch == '_' || (Ch >= 'A' && Ch <= 'Z')) || (Ch >= 'a' && Ch <= 'z'));
    }

    /// @brief The characters from an identifier except from the first
    /// can be also numbers.
    /// @param Ch
    /// @return
    LLVM_READNONE inline bool isIdentifierBody(char Ch)
    {
        return isIdentifierHead(Ch) || isDigit(Ch);
    }

} //! namespace charinfo

void Lexer::next(Token &Result)
{
    // move the current pointer while is not a white space
    // or the buffer is not empty
    while (*CurPtr && charinfo::isWhitespace(*CurPtr))
    {
        ++CurPtr;
    }
    // if there are no more tokens, set as eof and return
    if (!*CurPtr)
    {
        Result.setKind(tok::eof);
        return;
    }
    // in a recursive descendent parser we must check
    // first what is expected first

    // check for a possible identifier
    if (charinfo::isIdentifierHead(*CurPtr))
    {
        // parse identifier
        identifier(Result);
        return;
    }
    // parse possible digit number
    else if (charinfo::isDigit(*CurPtr))
    {
        number(Result);
        return;
    }
    // check for possible string, both  "" and '' are supported
    else if (*CurPtr == '"' || *CurPtr == '\'')
    {
        string(Result);
        return;
    }
    // now check for other possible tokens in the
    else
    {
        switch (*CurPtr)
        {
#define CASE(ch, tok)                       \
    case ch:                                \
        formToken(Result, CurPtr + 1, tok); \
        break

            CASE('=', tok::equal);   // = character
            CASE('#', tok::hash);    // # character
            CASE('+', tok::plus);    // + character
            CASE('-', tok::minus);   // - character
            CASE('*', tok::star);    // * character
            CASE('/', tok::slash);   // / character
            CASE(',', tok::comma);   // , character
            CASE('.', tok::period);  // . character
            CASE(';', tok::semi);    // ; character (end of code line)
            CASE(')', tok::r_paren); // end of parenthesis
#undef CASE
        // now other tokens that needs more work
        case '(':
            // in case we have (* this is a comment
            if (*(CurPtr + 1) == '*')
            {
                comment();
                next(Result);
            }
            // other case, it is a left parent from a expression
            else
                formToken(Result, CurPtr + 1, tok::l_paren);
            break;
        case ':':
            // assignment token
            if (*(CurPtr + 1) == '=')
                formToken(Result, CurPtr + 2, tok::colonequal);
            // just colon token
            else
                formToken(Result, CurPtr + 1, tok::colon);
            break;
        // next used for comparisons
        case '<':
            if (*(CurPtr + 1) == '=')
                formToken(Result, CurPtr + 2, tok::lessequal);
            else
                formToken(Result, CurPtr + 1, tok::less);
            break;
        case '>':
            if (*(CurPtr + 1) == '=')
                formToken(Result, CurPtr + 2, tok::greaterequal);
            else
                formToken(Result, CurPtr + 1, tok::greater);
            break;
        // unknown token...
        default:
            Result.setKind(tok::unknown);
        }
        return;
    }
}

void Lexer::identifier(Token &Result)
{
    const char *Start = CurPtr;
    const char *End = CurPtr + 1;
    while (charinfo::isIdentifierBody(*End))
        ++End;
    // create name for token
    StringRef Name(Start, End - Start);
    // create a token, for the token we will check
    // if it is a keyword, or in a default case, an identifier
    formToken(Result, End, Keywords.getKeyword(Name, tok::identifier));
}

void Lexer::number(Token &Result)
{
    // const char *Start = CurPtr;
    const char *End = CurPtr + 1;
    tok::TokenKind Kind = tok::unknown;

    bool IsHex = false;

    while (*End)
    {
        if (!charinfo::isHexDigit(*End)) 
            break; // if it is not an hex digit, it is neither a digit, go out
        if (!charinfo::isDigit(*End)) 
            IsHex = true; // if it is not a digit, it can be a hex digit, so set true
        ++End;
    }

    switch (*End)
    {
    case 'H': // we find an 'H', it means we have an hexadecimal number
        Kind = tok::integer_literal;
        ++End;
        break;
    default:
        if (IsHex) // if we find something different to H, and is an hex number, error
            Diags.report(getLoc(), diag::err_hex_digit_in_decimal);
        Kind = tok::integer_literal; // integer literal value
        break;
    }

    formToken(Result, End, Kind);
}

void Lexer::string(Token &Result)
{
    const char *Start = CurPtr;
    const char *End = CurPtr + 1;
    // a string can contains any kind of character, except this
    // is a vertical white space
    while (*End && *End != *Start && !charinfo::isVerticalWhitespace(*End))
        ++End;
    // if we found a vertial whitespace, error
    if (charinfo::isVerticalWhitespace(*End))
    {
        Diags.report(getLoc(), diag::err_unterminated_char_or_string);
    }
    formToken(Result, End + 1, tok::string_literal);
}

void Lexer::comment()
{
    const char *End = CurPtr + 2;
    unsigned Level = 1;
    while (*End && Level)
    {
        // each time we find (* we enter in a level
        // more of comment
        if (*End == '(' && *(End + 1) == '*')
        {
            End += 2;
            Level++;
        }
        // comments end with *)
        else if (*End == '*' && *(End + 1) == ')')
        {
            End += 2;
            Level--;
        }
        else
            ++End;
    }
    // if we find the end of the string
    // we are in front of an unterminated block
    if (!*End)
    {
        Diags.report(getLoc(), diag::err_unterminated_block_comment);
    }
    CurPtr = End;
    // comments of course do not generate tokens
}

void Lexer::formToken(Token &Result, const char *TokEnd, tok::TokenKind Kind)
{
    size_t TokLen = TokEnd - CurPtr;
    Result.Ptr = CurPtr;
    Result.Length = TokLen;
    Result.Kind = Kind;
    CurPtr = TokEnd;
}