#include "Parser.h"

AST *Parser::parse()
{
    AST *Res = parseCalc();
    expect(Token::eoi);
    return Res;
}

/**
 * Parse an expression like the next one: 'calc : ("with" ident ("," ident)* ":")? expr ;'
 */
AST *Parser::parseCalc()
{
    Expr *E;
    llvm::SmallVector<llvm::StringRef, 8> Vars;

    // since the first part is optional, we have to check
    // if it exists, and we want to parse it
    if (Tok.is(Token::KW_with))
    {
        advance(); // move to next token
        // after with we must find an identifier
        if (expect(Token::ident))
            goto _error;
        // we find a variable, push it to a vector
        // the vector makes a copy of Tok.
        Vars.push_back(Tok.getText());
        advance();
        // now we have a loop of commas and identifiers
        // since is a repeated pattern.
        while (Tok.is(Token::comma))
        {
            // move next token
            advance();
            // expect an identifier
            if (expect(Token::ident))
                goto _error;
            Vars.push_back(Tok.getText());
            // move next token
            advance();
        }

        // finally if we have the 'with' with the variables
        // we need a colon
        if (consume(Token::colon))
            goto _error;
    }

    // now we must parse an expression
    E = parseExpr();

    // now we have possible variables, and an expression
    // return the possibilities
    if (Vars.empty())
        return E;
    else
        return new WithDecl(Vars, E);

_error:
    // our error handling routing will be consuming
    // tokens until we find an eoi token.
    while (!Tok.is(Token::eoi))
        advance();
    return nullptr;
}

Expr *Parser::parseExpr()
{
    Expr *Left = parseTerm();

    while (Tok.isOneOf(Token::plus, Token::minus)) // check if it is a + or - expression
    {
        BinaryOp::Operator Op = Tok.is(Token::plus) ? BinaryOp::Plus : BinaryOp::Minus;

        advance();
        Expr *Right = parseTerm();
        Left = new BinaryOp(Op, Left, Right);
    }
    return Left;
}

Expr *Parser::parseTerm()
{
    Expr * Left = parseFactor();
    while (Tok.isOneOf(Token::star, Token::slash, Token::rem))
    {
        BinaryOp::Operator Op;

        if (Tok.is(Token::star))
            Op = BinaryOp::Mul;
        else if (Tok.is(Token::slash))
            Op = BinaryOp::Div;
        else if (Tok.is(Token::rem))
            Op = BinaryOp::Rem;
        advance();
        Expr *Right = parseFactor();
        Left = new BinaryOp(Op, Left, Right);
    }

    return Left;
}

Expr* Parser::parseFactor()
{
    Expr * Res = nullptr;
    switch(Tok.getKind())
    {
    case Token::number:
        Res = new Factor(Factor::Number, Tok.getText());
        advance();
        break;
    case Token::ident:
        Res = new Factor(Factor::Ident, Tok.getText());
        advance();
        break;
    case Token::l_paren:
        // we found a left parenthesis
        // we will have something like
        // (Expr)
        // advance and parse
        advance();
        Res = parseExpr();
        // after we parse the Expr, we have to
        // find a right parenthesis
        if (!consume(Token::r_paren)) break;
    default:
        if (!Res) error();
    }

    while(!Tok.isOneOf(Token::r_paren, Token::star, Token::plus, Token::minus, Token::slash, Token::rem, Token::eoi))
        advance();
    
    return Res;
}