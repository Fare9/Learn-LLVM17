#ifndef TINYLANG_PARSER_PARSER_H
#define TINYLANG_PARSER_PARSER_H

#include "tinylang/Basic/Diagnostic.h"
#include "tinylang/Lexer/Lexer.h"
#include "tinylang/Sema/Sema.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/raw_ostream.h"

namespace tinylang
{
    class Parser
    {
        /// @brief parser will use the lexer to create the AST nodes
        Lexer &Lex;

        /// @brief semantic analyzer
        Sema &Actions;

        /// @brief Current token
        Token Tok;

        /// @brief Get the current diagnostic engine from lexer
        /// @return diagnostic engine for errors
        DiagnosticsEngine &getDiagnostics() const
        {
            return Lex.getDiagnostics();
        }
        
        /// @brief make lexer read the next token
        void advance()
        {
            Lex.next(Tok);
        }

        /// @brief look for an expected token, in case an unexpected token is found report it
        /// @param ExpectedTok token we expect now
        /// @return true in case there was an error, false everything fine
        bool expect(tok::TokenKind ExpectedTok)
        {
            if (Tok.is(ExpectedTok)) // check if is expected token
                return false; // no error

            const char *Expected = tok::getPunctuatorSpelling(ExpectedTok);

            if (!Expected)
                Expected = tok::getKeywordSpelling(ExpectedTok);

            llvm::StringRef Actual(Tok.getLocation().getPointer(), Tok.getLength());

            getDiagnostics().report(Tok.getLocation(), diag::err_expected, Expected, Actual);

            return true;
        }

        /// @brief Consume a current expected token, once consumed, advance to the next token.
        /// @param ExpectedTok token we expect
        /// @return false in case everything is fine, true in case of error
        bool consume(tok::TokenKind ExpectedTok)
        {
            if (Tok.is(ExpectedTok))
            {
                advance();
                return false;
            }

            return true;
        }

        /***
         * Full implementation of a Recursive descent parser
         * for each one of the non-terminals and terminals
         * parts of the EBNF. Since the parser is for a LL(1)
         * grammar, we must avoid left recursion, so something
         * like:
         * 
         * stmnt : stmnt | ...
         * 
         * Must be avoided, since that is translated to:
         * 
         * bool parseStatement(StmtList &Stmts)
         * {
         *      parseStatement(Stmts);
         *      ...
         * }
         * 
         * Finishing in an infinite recursion.
        */

        /// @brief Parse a compilation unit
        /// @param D module to parse
        /// @return 
        bool parseCompilationUnit(ModuleDeclaration *&D);

        /// @brief Parse an import statement that would import other files
        /// @return 
        bool parseImport();

        /// @brief Parse a basic block, basic blocks can have declarations
        /// and statements
        /// @param Decls declarations of the program
        /// @param Stmts statements of the program
        /// @return 
        bool parseBlock(DeclList &Decls, StmtList &Stmts);

        /// @brief Parse a declaration of a variable
        /// @param Decls declarations of the program
        /// @return 
        bool parseDeclaration(DeclList &Decls);
        /// @brief Parse a constant declaration, with an specified value
        /// @param Decls declarations of the program
        /// @return 
        bool parseConstantDeclaration(DeclList &Decls);
        /// @brief Parse a new declared type, this is mostly a renaming
        /// of an original type
        /// @param Decls declarations of the program
        /// @return 
        bool parseTypeDeclaration(DeclList &Decls);
        /// @brief Parse a list of fields
        /// @param Fields list of fields
        /// @return 
        bool parseFieldList(FieldList &Fields);
        /// @brief Parse a field
        /// @param Fields list of fields
        /// @return 
        bool parseField(FieldList &Fields);
        

        bool parseVariableDeclaration(DeclList &Decls);
        bool parseProcedureDeclaration(DeclList &ParentDecls);
        bool parseFormalParameters(FormalParamList &Params,
                                   Decl *&RetType);
        bool parseFormalParameterList(FormalParamList &Params);
        bool parseFormalParameter(FormalParamList &Params);
        bool parseStatementSequence(StmtList &Stmts);
        bool parseStatement(StmtList &Stmts);
        /**
         * @brief Parse:
         *
         * ifStatement : "IF" expression "THEN" statementSequence
         * ( "ELSE" statementSequence )? "END" ;
         */
        bool parseIfStatement(StmtList &Stmts);
        bool parseWhileStatement(StmtList &Stmts);
        bool parseReturnStatement(StmtList &Stmts);
        bool parseExpList(ExprList &Exprs);
        bool parseExpression(Expr *&E);
        bool parseRelation(OperatorInfo &Op);
        bool parseSimpleExpression(Expr *&E);
        bool parseAddOperator(OperatorInfo &Op);
        bool parseTerm(Expr *&E);
        bool parseMulOperator(OperatorInfo &Op);
        bool parseFactor(Expr *&E);
        // new from this version
        bool parseSelectors(Expr *&E);
        //
        bool parseQualident(Decl *&D);
        bool parseIdentList(IdentList &Ids);

    public:
        Parser(Lexer &Lex, Sema &Actions);

        ModuleDeclaration *parse();
    };

} //! namespace tinylang

#endif