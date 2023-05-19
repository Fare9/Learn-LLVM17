#ifndef TINYLANG_BASIC_TOKENKINDS_H
#define TINYLANG_BASIC_TOKENKINDS_H

namespace tinylang
{

    namespace tok
    {
        /// @brief Kind of different tokens
        enum TokenKind : unsigned short
        {
#define TOK(ID) ID,
#include "TokenKinds.def"
            NUM_TOKENS
        };

        /// @brief From previous array of strings, return token name
        /// @param Kind kind of token
        /// @return name of token
        const char *getTokenName(TokenKind Kind);

        /// @brief Get from a token kind the string that represents the token
        /// @param Kind kind of token
        /// @return string that represents the token
        const char *getPunctuatorSpelling(TokenKind Kind);

        /// @brief Get a string from the keywords
        /// @param Kind keyword to retrieve the string
        /// @return string of keyword
        const char *getKeywordSpelling(TokenKind Kind);

    } //! namespace tok
} //! namespace tinylang

#endif