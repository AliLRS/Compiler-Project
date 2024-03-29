#include "Parser.h"

bool haveElse;

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
        haveElse = true;
        switch (Tok.getKind())
        {
        case Token::KW_int:
            Declaration *d;
            d = parseDec();
            if (d)
                data.push_back(d);
            else {
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
                goto _error;
            }
            break;
        case Token::KW_if:
            IfStmt *i;
            i = parseIf();
            if (i)
                data.push_back(i);
            else {
                goto _error;
            }
            break;
        case Token::KW_loopc:
            IterStmt *l;
            l = parseIter();
            if (l)
                data.push_back(l);
            else {
                goto _error;
            }
            break;
        default:
            error();
            goto _error;
            break;
        }
        if(haveElse){
            advance();
        }
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
    
    if (expect(Token::KW_int)){
        goto _error;
    }

    advance();
    
    if (expect(Token::ident)){
        goto _error;
    }

    Vars.push_back(Tok.getText());
    advance();

    
    while (Tok.is(Token::comma))
    {
        advance();
        if (expect(Token::ident)){
            goto _error;
        }
            
        Vars.push_back(Tok.getText());
        count++;
        advance();
    }

    if (Tok.is(Token::assign))
    {
        advance();
        E = parseExpr();
        if(E){
            Values.push_back(E);
            count--; 
        }
        else{
            goto _error;
        }
        
        while (Tok.is(Token::comma))
        {   
            if (count == 0){
                error();
                goto _error;
            }

            advance();
            E = parseExpr();
            if(E){
                Values.push_back(E);
                count--; 
            }
            else{
                goto _error;
            }
        }
    }

    if (expect(Token::semicolon)){
        goto _error;
    }


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
    if (F == nullptr)
    {
        goto _error;
    }

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
    if(E){
        return new Assignment(F, E, AK);
    }
    else{
        goto _error;
    }

_error:
    while (Tok.getKind() != Token::eoi)
        advance();
    return nullptr;
}

Expr *Parser::parseExpr()
{
    Expr *Left = parseTerm();
    if (Left == nullptr)
    {
        goto _error;
    }
    
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
        if (Right == nullptr)
        {
            goto _error;
        }
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
    if (Left == nullptr)
    {
        goto _error;
    }
    while (Tok.isOneOf(Token::star, Token::mod, Token::slash))
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
        if (Right == nullptr)
        {
            goto _error;
        }
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
    if (Left == nullptr)
    {
        goto _error;
    }
    while (Tok.is(Token::exp))
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
        if (Right == nullptr)
        {
            goto _error;
        }
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
        if(Res == nullptr){
            goto _error;
        }
        if (!consume(Token::r_paren))
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
        if (Res == nullptr)
        {
            goto _error;
        }
        if (consume(Token::r_paren))
            goto _error;
    }
    else {
        Expr *Left = parseExpr();
        if(Left == nullptr){
            goto _error;
        }
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
            if (Right == nullptr)
            {
                goto _error;
            }
            
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
    if (Left == nullptr)
    {
        goto _error;
    }
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
        if (Right == nullptr)
        {
            goto _error;
        }
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
    Assignment *elseAssignment;

    haveElse = false;

    if (expect(Token::KW_if)){
        goto _error;
    }

    advance();

    Cond = parseLogic();
    if (Cond == nullptr)
    {
        goto _error;
    }

    if (expect(Token::colon)){
        goto _error;
    }
        
    advance();

    if (expect(Token::KW_begin)){
        goto _error;
    }

    advance();
    
    while (!Tok.is(Token::KW_end))
    {
        ifAsgnmnt = parseAssign();
        if(ifAsgnmnt)
           ifAssignments.push_back(ifAsgnmnt);
        else
            goto _error;
        
        if (expect(Token::semicolon))
        {
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
        Assignment *elifAssignment;

        Cond = parseLogic();
        if (Cond == nullptr)
        {
            goto _error;
        }

        if (expect(Token::colon)){
            goto _error;
        }

        advance();

        if (expect(Token::KW_begin)){
            goto _error;
        }

        advance();

        while (!Tok.is(Token::KW_end))
        {
            elifAssignment = parseAssign();

            if (elifAssignment)
            {
                elifAssignments.push_back(elifAssignment);                
            }
            else
                goto _error;
            
            if (expect(Token::semicolon))
            {
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
        haveElse = true;

        advance();

        if (expect(Token::colon)){
            goto _error;
        }

        advance();

        if (expect(Token::KW_begin)){
            goto _error;
        }
        
        advance();

        while (!Tok.is(Token::KW_end))
        {
            elseAssignment = parseAssign();
            if(elseAssignment)
                elseAssignments.push_back(elseAssignment);
            else
                goto _error;

            if (expect(Token::semicolon))
            {
                goto _error;
            }

            advance();
        }

    }


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

    if (expect(Token::KW_loopc)){
        goto _error;
    }
        
    advance();

    Cond = parseLogic();
    if (Cond == nullptr)
    {
        goto _error;
    }
    if(expect(Token::colon)){
        goto _error;
    }

    advance();

    if (expect(Token::KW_begin)){
        goto _error;
    }

    advance();

    while (!Tok.is(Token::KW_end))
    {
        Assignment *asgnmnt = parseAssign();
        if(asgnmnt){
            assignments.push_back(asgnmnt);
        }
        else{
            goto _error;
        }
        
        if (expect(Token::semicolon))
        {
            goto _error;
        }

        advance();
    }

    return new IterStmt(Cond, assignments);

_error:
    while (Tok.getKind() != Token::eoi)
        advance();
    return nullptr;
}
