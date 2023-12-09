#include "Parser.h"

// main point is that the whole input has been consumed
AST *Parser::parse()
{
    AST *Res = parseProgram();
    return Res;
}

AST *Parser::parseProgram()
{
    llvm::SmallVector<Expr *> exprs;
    while (!Tok.is(Token::eoi))
    {
        switch (Tok.getKind())
        {
        case Token::KW_type:
            Expr *d;
            d = parseDec();
            if (d)
                exprs.push_back(d);
            else
                goto _error2;
            break;
        case Token::ident:
            Expr *a;
            a = parseAssign();

            if (!Tok.is(Token::semicolon))
            {
                error();
                goto _error2;
            }
            if (a)
                exprs.push_back(a);
            else
                goto _error2;
            break;
        default:
            goto _error2;
            break;
        }
        advance(); // TODO: watch this part
    }
    return new Program(exprs);
_error2:
    while (Tok.getKind() != Token::eoi)
        advance();
    return nullptr;
}

Expr *Parser::parseDec()
{
    Expr *E;
    llvm::SmallVector<llvm::StringRef, 8> Vars;
    llvm::SmallVector<Expr *, 8> Values;
    
    if (expect(Token::KW_int))
        goto _error;

    advance();
    
    if (expect(Token::ident))
        goto _error;
    Vars.push_back(Tok.getText());
    advance();

    int count = 1;
    while (Tok.is(Token::comma))
    {
        advance();
        if (expect(Token::ident))
            goto _error;
        Vars.push_back(Tok.getText());
        count++;
        advance();
    }

    if (Tok.is(Token::equal))
    {
        advance();
        E = parseExpr();
        Values.push_back(E);
        count--; 
        while (Tok.is(Token::comma))
        {   
            if (count == 0)
                goto _error;
            advance();
            E = parseExpr();
            Values.push_back(E);
            count--;
        }
    }

    if (expect(Token::semicolon))
        goto _error;

    return new Declaration(Vars, E);
_error: // TODO: Check this later in case of error :)
    while (Tok.getKind() != Token::eoi)
        advance();
    return nullptr;
}

Expr *Parser::parseAssign()
{
    Expr *E;
    Factor *F;
    Assignment::AssignKind AK;

    F = (Factor *)(parseFactor());

    if (Tok.is(Token::equal))
    {
        AK = Assignment::Equal;
    }
    else if (Tok.is(Token::plus_assign))
    {
        AK = Assignment::Plus_assign
    }
    else if (Tok.is(Token::minus_assign))
    {
        AK = Assignment::Minus_assign;
    }
    else if (Tok.is(Token::star_assign))
    {
        AK = Assignment::Mul_assign;
    }
    else if (Tok.is(Token::slash_assign))
    {
        AK = Assignment::Div_assign;
    }
    else if (Tok.is(Token::mod_assign))
    {
        AK = Assignment::Mod_assign;
    }
    else if (Tok.is(Token::exp_assign))
    {
        AK = Assignment::Exp_assign;
    }
    else
    {
        error();
        return nullptr;
    }

    advance();
    E = parseExpr();
    return new Assignment(F, E, AK);
}

Expr *Parser::parseExpr()
{
    Expr *Left = parseTerm();
    while (Tok.isOneOf(Token::plus, Token::minus))
    {
        // BinaryOp::Operator Op =
        //     Tok.is(Token::plus) ? BinaryOp::Plus : BinaryOp::Minus;
        BinaryOp::Operator Op;
        if (Tok.is(Token::plus))
            Op = BinaryOp::Plus;
        else if (Tok.is(Token::minus))
            Op = BinaryOp::Minus;
        else {
            error();
            return nullptr;
        }
        advance();
        Expr *Right = parseTerm();
        Left = new BinaryOp(Op, Left, Right);
    }
    return Left;
}

Expr *Parser::parseTerm()
{
    Expr *Left = parseFactor();
    while (Tok.isOneOf(Token::star, Token::slash))
    {
        // BinaryOp::Operator Op =
        //     Tok.is(Token::star) ? BinaryOp::Mul : BinaryOp::Div;
        BinaryOp::Operator Op;
        if (Tok.is(Token::star))
            Op = BinaryOp::Mul;
        else if (Tok.is(Token::slash))
            Op = BinaryOp::Div;
        else if (Tok.is(Token::mod))
            Op = BinaryOp::Mod;
        else {
            error();
            return nullptr;
        }
        advance();
        Expr *Right = parseFactor();
        Left = new BinaryOp(Op, Left, Right);
    }
    return Left;
}

Expr *Parser::parseFactor()
{
    Expr *Res = nullptr;
    switch (Tok.getKind())
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
        advance();
        Res = parseExpr();
        if (!consume(Token::r_paren))
            break;
    default: // error handling
        if (!Res)
            error();
        while (!Tok.isOneOf(Token::r_paren, Token::star, Token::plus, Token::minus, Token::slash, Token::eoi))
            advance();
        break;
    }
    return Res;
}

Expr *Parser::parseComparison()
{
    Expr *Left = parseExpr();
    while (Tok.isOneOf(Token::star, Token::slash))
    {
        Comparison::Operator Op;
        if (Tok.is(Token::eq))
            Op = Comparison::Equal;
        else if (Tok.is(Token::neq))
            Op = Comparison::Not_equal;
        else if (Tok.is(Token::gt))
            Op = Comparison::Greater;
        else if (Tok.is(Token::lt))
            Op = Comparison::Less;
        else if (Tok.is(Token::gte))
            Op = Comparison::Greater_equal;
        else if (Tok.is(Token::lte))
            Op = Comparison::Less_equal;
       else {
            error();
            return nullptr;
        }
        advance();
        Expr *Right = parseExpr();
        Left = new BinaryOp(Op, Left, Right);
    }
    return Left;
}