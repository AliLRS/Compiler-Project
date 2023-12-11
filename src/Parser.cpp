#include "Parser.h"

// main point is that the whole input has been consumed
Program *Parser::parse()
{
    Program *Res = parseProgram();
    return Res;
}

Program *Parser::parseProgram()
{
    llvm::SmallVector<AST *> data;
    while (!Tok.is(Token::eoi))
    {
        switch (Tok.getKind())
        {
        case Token::KW_int:
            Declaration *d;
            d = parseDec();
            if (d)
                data.push_back(d);
            else {
                error();
                goto _error;
            }
            break;
        case Token::ident:
            Assignment *a;
            a = parseAssign();

            if (!Tok.is(Token::semicolon))
            {
                error();
                goto _error;
            }

            if (a)
                data.push_back(a);
            else {
                error();
                goto _error;
            }
            break;
        case Token::KW_if:
            IfStmt *i;
            i = parseIf();
            if (i)
                data.push_back(i);
            else {
                error();
                goto _error;
            }
            break;
        case Token::KW_loopc:
            IterStmt *l;
            l = parseIter();
            if (l)
                data.push_back(l);
            else {
                error();
                goto _error;
            }
            break;
        default:
            goto _error;
            break;
        }
        advance();
    }
    return new Program(data);
_error:
    while (Tok.getKind() != Token::eoi)
        advance();
    return nullptr;
}

Declaration *Parser::parseDec()
{
    Expr *E;
    llvm::SmallVector<llvm::StringRef, 8> Vars;
    llvm::SmallVector<Expr *, 8> Values;
    int count = 1;
    
    if (expect(Token::KW_int))
        goto _error;

    advance();
    
    if (expect(Token::ident))
        goto _error;
    Vars.push_back(Tok.getText());
    advance();

    
    while (Tok.is(Token::comma))
    {
        advance();
        if (expect(Token::ident))
            goto _error;
        Vars.push_back(Tok.getText());
        count++;
        advance();
    }

    if (Tok.is(Token::eq))
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

    return new Declaration(Vars, Values);
_error: 
    while (Tok.getKind() != Token::eoi)
        advance();
    return nullptr;
}

Assignment *Parser::parseAssign()
{
    Expr *E;
    Final *F;
    Assignment::AssignKind AK;

    F = (Final *)(parseFinal());

    if (Tok.is(Token::assign))
    {
        AK = Assignment::Assign;
    }
    else if (Tok.is(Token::plus_assign))
    {
        AK = Assignment::Plus_assign;
    }
    else if (Tok.is(Token::minus_assign))
    {
        AK = Assignment::Minus_assign;
    }
    else if (Tok.is(Token::star_assign))
    {
        AK = Assignment::Star_assign;
    }
    else if (Tok.is(Token::slash_assign))
    {
        AK = Assignment::Slash_assign;
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
        goto _error;
    }

    advance();
    E = parseExpr();
    return new Assignment(F, E, AK);

_error:
    while (Tok.getKind() != Token::eoi)
        advance();
    return nullptr;
}

Expr *Parser::parseExpr()
{
    Expr *Left = parseTerm();
    while (Tok.isOneOf(Token::plus, Token::minus))
    {
        BinaryOp::Operator Op;
        if (Tok.is(Token::plus))
            Op = BinaryOp::Plus;
        else if (Tok.is(Token::minus))
            Op = BinaryOp::Minus;
        else {
            error();
            goto _error;
        }
        advance();
        Expr *Right = parseTerm();
        Left = new BinaryOp(Op, Left, Right);
    }
    return Left;

_error:
    while (Tok.getKind() != Token::eoi)
        advance();
    return nullptr;
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
            goto _error;
        }
        advance();
        Expr *Right = parseFactor();
        Left = new BinaryOp(Op, Left, Right);
    }
    return Left;

_error:
    while (Tok.getKind() != Token::eoi)
        advance();
    return nullptr;
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
            goto _error;
        }
        advance();
        Expr *Right = parseFactor();
        Left = new BinaryOp(Op, Left, Right);
    }
    return Left;

_error:
    while (Tok.getKind() != Token::eoi)
        advance();
    return nullptr;
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
        if (consume(Token::r_paren))
            break;
    default:
        error();
        goto _error;
    }
    return Res;

_error:
    while (Tok.getKind() != Token::eoi)
        advance();
    return nullptr;
}

Logic *Parser::parseComparison()
{
    Logic *Res = nullptr;
    if (Tok.is(Token::l_paren)) {
        advance();
        Res = parseLogic();
        if (consume(Token::r_paren))
            goto _error;
    }
    else {
        Expr *Left = parseExpr();
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
                    goto _error;
                }
            advance();
            Expr *Right = parseExpr();
            Res = new Comparison(Left, Right, Op);
    }
    
    return Res;

_error:
    while (Tok.getKind() != Token::eoi)
        advance();
    return nullptr;
}

Logic *Parser::parseLogic()
{
    Logic *Left = parseComparison();
    while (Tok.isOneOf(Token::KW_and, Token::KW_or))
    {
        LogicalExpr::Operator Op;
        if (Tok.is(Token::KW_and))
            Op = LogicalExpr::And;
        else if (Tok.is(Token::KW_or))
            Op = LogicalExpr::Or;
        else {
            error();
            goto _error;
        }
        advance();
        Logic *Right = parseComparison();
        Left = new LogicalExpr(Left, Right, Op);
    }
    return Left;

_error:
    while (Tok.getKind() != Token::eoi)
        advance();
    return nullptr;
}

IfStmt *Parser::parseIf()
{
    llvm::SmallVector<Assignment *, 8> ifAssignments;
    llvm::SmallVector<Assignment *, 8> elseAssignments;
    llvm::SmallVector<elifStmt *, 8> elifStmts;
    Logic *Cond;
    Assignment *ifAsgnmnt;

    if (expect(Token::KW_if))
        goto _error;

    advance();

    Cond = parseLogic();

    if (expect(Token::colon))
        goto _error;

    advance();

    if (expect(Token::KW_begin))
        goto _error;

    advance();
    
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
        llvm::SmallVector<Assignment *, 8> elifAssignments;
        Logic *Cond;

        Cond = parseLogic();

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

    return new IfStmt(Cond, ifAssignments, elseAssignments, elifStmts);

_error:
    while (Tok.getKind() != Token::eoi)
        advance();
    return nullptr;
}

IterStmt *Parser::parseIter()
{
    llvm::SmallVector<Assignment *, 8> assignments;
    Logic *Cond;

    if (expect(Token::KW_loopc))
        goto _error;

    advance();

    Cond = parseLogic();

    if (expect(Token::KW_begin))
        goto _error;

    advance();

    while (!Tok.is(Token::KW_end))
    {
        Assignment *asgnmnt = parseAssign();
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

_error:
    while (Tok.getKind() != Token::eoi)
        advance();
    return nullptr;
}