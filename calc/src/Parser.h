/***
 * @file Parser.h
 * @brief The parser manages the tokens for generating the AST of the
 * code. It must properly parse the tokens to properly keep the precedence
 * from the expressions.
 */
#ifndef PARSER_H
#define PARSER_H

#include "AST.h"
#include "Lexer.h"

// since LLVM forbid iostream, we will use
// llvm raw_ostream for error messages
#include "llvm/Support/raw_ostream.h"

class Parser
{
    //! reference to a lexer
    Lexer &Lex;
    //! object for the next token (look-ahead parsing)
    Token Tok;
    //! cache for error
    bool HasError;


    //!----------------------------------------------------
    //! Utility methods used by the Parser to generate the
    //! different objects from the model of the AST.

    /**
     * @brief Print an error message and
     *        set that there was an error.
     *
     */
    void error()
    {
        llvm::errs() << "Unexpected: " << Tok.getText()
                     << "\n";
        HasError = true;
    }

    /**
     * @brief Move to the next token with the lexer.
     */
    void advance()
    {
        Lex.next(Tok); // move Tok to next token
    }

    /**
     * @brief Check if the current token is an
     *        expected one or not. We return True
     *        in case it is incorrect, in that case
     *        we can throw an exception always that
     *        if (expect(Kind)) is incorrect
     *
     * @param Kind expected token
     * @return true in case the token is incorrect, false other case.
     */
    bool expect(Token::TokenKind Kind)
    {
        if (!Tok.is(Kind))
        {
            error();
            return true;
        }

        return false;
    }

    /**
     * @brief Consume tokens and advance, in case of error
     *        because Kind is not expected, error.
     *
     * @param Kind expected token
     * @return true in case token is not the expected one, false
     *         if we consume token.
     */
    bool consume(Token::TokenKind Kind)
    {
        if (expect(Kind)) // if Kind is not the expected
                          // one to consume...
            return true;  // return an error
        advance();
        return false;
    }

    /**
     * @brief Parse a whole Calc program, return the AST.
     *
     * @return whole program represented by an AST.
     */
    AST *parseCalc();

    //! Parsers for the expressions, a recursive descent parser
    //! with a look ahead of 1 token
    /**
     * @brief Parse an expression from the program
     *
     * @return expression parsed
     */
    Expr *parseExpr();

    /**
     * @brief Parse a term token, these are tokens that finalize
     *        the EBNF.
     *
     * @return Expression parsed.
     */
    Expr *parseTerm();

    /**
     * @brief Parse a factor from Calc language.
     *
     * @return Expression with factor parsed.
     */
    Expr *parseFactor();

public:
    //! interface of the class.

    /**
     * @brief constructor for the parser we will have
     *        some constructor initializer for example
     *        the one for the Lex reference.
     *
     */
    Parser(Lexer &Lex) : Lex(Lex),
                         HasError(false)
    {
        // retrieve the first token
        advance();
    }

    /**
     * @brief Member function to return HasError
     * @return value of HasError.
     */
    bool hasError() const
    {
        return HasError;
    }

    /**
     * @brief public method to parse our AST, this can be used
     * to initialize values, and call the internal methods to
     * parse the expressions.
     * 
     * @return pointer to an AST.
     */
    AST *parse();
};

#endif