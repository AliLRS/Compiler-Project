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
_error: 
    while (Tok.getKind() != Token::eoi)
        advance();
    return nullptr;
}

Expr *Parser::parseAssign()
{
    Expr *E;
    Final *F;
    Assignment::AssignKind AK;

    F = (Final *)(parseFinal());

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
    Expr *Left = parseFinal();
    while (Tok.is(Token::mod))
    {
        BinaryOp::Operator Op;
        if (Tok.is(Token::exp))
            Op = BinaryOp::Exp;
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

Expr *Parser::parseFinal()
{
    Expr *Res = nullptr;
    switch (Tok.getKind())
    {
    case Token::number:
        Res = new Final(Final::Number, Tok.getText());
        advance();
        break;
    case Token::ident:
        Res = new Final(Final::Ident, Tok.getText());
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
        else if (Tok.is(Token::l_paren)) {
            advance();
            Expr *Right = parseExpr();
            if (!consume(Token::r_paren))
                break;
        }
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

Expr *Parser::parseLogicalExpr()
{
    Expr *Left = parseComparison();
    while (Tok.isOneOf(Token::star, Token::slash))
    {
        LogicalExpr::Operator Op;
        if (Tok.is(Token::and))
            Op = LogicalExpr::And;
        else if (Tok.is(Token::or))
            Op = LogicalExpr::Or;
        else if (Tok.is(Token::l_paren)) {
            advance();
            Expr *Right = parseExpr();
            if (!consume(Token::r_paren))
                break;
        }
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

Expr *Parser::parseIf()
{
    llvm::SmallVector<Expr *, 8> ifAssignments;
    llvm::SmallVector<Expr *, 8> elseAssignments;
    llvm::SmallVector<elifStmt *, 8> elifStmts;

    if (expect(Token::KW_if))
        goto _error;

    advance();

    Expr *Cond = parseLogicalExpr();

    if (expect(Token::colon))
        goto _error;

    advance();

    if (expect(Token::KW_begin))
        goto _error;

    advance();

    Expr *ifAsgnmnt;
    
    while (!Tok.is(Token::KW_end))
    {
        ifAsgnmnt = parseAssign();
        ifAssignments.push_back(ifAsgnmnt);

        if (expect(Token::semicolon))
        {
            error();
            goto _error;
        }

        advance();
    }

    advance();

    while (Tok.is(Token::KW_elif)) {

        advance();
        
        elifStmt *elif;
        llvm::SmallVector<Expr *, 8> elifAssignments;
        Expr *Cond;

        Cond = parseLogicalExpr();

        if (expect(Token::colon))
            goto _error;

        advance();

        if (expect(Token::KW_begin))
            goto _error;

        advance();

        while (!Tok.is(Token::KW_end))
        {

            elifAssignments.push_back(parseAssign());
            
            if (expect(Token::semicolon))
            {
                error();
                goto _error;
            }

            advance();
        }

        elif = new elifStmt(Cond, elifAssignments);
        elifStmts.push_back(elif);
        advance();
    }

    if (Tok.is(Token::KW_else))
    {
        advance();

        if (expect(Token::colon))
            goto _error;

        advance();

        if (expect(Token::KW_begin))
            goto _error;
        
        advance();

        while (!Tok.is(Token::KW_end))
        {
            elseAssignments.push_back(parseAssign());

            if (expect(Token::semicolon))
            {
                error();
                goto _error;
            }

            advance();
        }

        advance();

    }
    // else 
    //     elseAssignments = nullptr;

    return new If(Cond, ifAssignments, elseAssignments, elifStmts);

_error:
    while (Tok.getKind() != Token::eoi)
        advance();
    return nullptr;
}

Expr *Parser::parseIter()
{
    llvm::SmallVector<Expr *, 8> assignments;

    if (expect(Token::KW_loopc))
        goto _error;

    advance();

    Expr *Cond = parseLogicalExpr();

    if (expect(Token::KW_begin))
        goto _error;

    advance();

    while (!Tok.is(Token::KW_end))
    {
        asgnmnt = parseAssign();
        assignments.push_back(asgnmnt);

        if (expect(Token::semicolon))
        {
            error();
            goto _error;
        }

        advance();
    }

    advance();

    return new IterStmt(Cond, assignments);
}