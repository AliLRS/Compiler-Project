#include "Lexer.h"

// classifying characters
namespace charinfo
{
    // ignore whitespaces
    LLVM_READNONE inline bool isWhitespace(char c)
    {
        return c == ' ' || c == '\t' || c == '\f' || c == '\v' ||
               c == '\r' || c == '\n';
    }

    LLVM_READNONE inline bool isDigit(char c)
    {
        return c >= '0' && c <= '9';
    }

    LLVM_READNONE inline bool isLetter(char c)
    {
        return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
    }

    LLVM_READNONE inline bool isSpecialSign(char c)
    {
        return c == '/' || c == '%' || c == '-' || c == '+' || c == '^' ||
               c == '*' || c == '<' || c == '>' || c == '=' || c == '!';
    }
}

void Lexer::next(Token &token)
{
    while (*BufferPtr && charinfo::isWhitespace(*BufferPtr))
    {
        ++BufferPtr;
    }
    // make sure we didn't reach the end of input
    if (!*BufferPtr)
    {
        token.Kind = Token::eoi;
        return;
    }
    // collect characters and check for keywords or ident
    if (charinfo::isLetter(*BufferPtr))
    {
        const char *end = BufferPtr + 1;
        while (charinfo::isLetter(*end))
            ++end;
        llvm::StringRef Name(BufferPtr, end - BufferPtr);
        Token::TokenKind kind;
        if (Name == "int")
            kind = Token::KW_int;
        else if (Name == "if")
            kind = Token::KW_if;
        else if (Name == "elif")
            kind = Token::KW_elif;
        else if (Name == "else")
            kind = Token::KW_else;
        else if (Name == "begin")
            kind = Token::KW_begin;
        else if (Name == "end")
            kind = Token::KW_end;
        else if (Name == "loopc")
            kind = Token::KW_loopc;
        else if (Name == "and")
            kind = Token::KW_and;
        else if (Name == "or")
            kind = Token::KW_or;
        else
            kind = Token::ident;
        // generate the token
        formToken(token, end, kind);
        return;
    }
    // check for numbers
    else if (charinfo::isDigit(*BufferPtr))
    {
        const char *end = BufferPtr + 1;
        while (charinfo::isDigit(*end))
            ++end;
        formToken(token, end, Token::number);
        return;
    }
    else if (charinfo::isSpecialSign(*BufferPtr))
    {
        const char *end = BufferPtr + 1;
        while (charinfo::isSpecialSign(*end))
            ++end;
        llvm::StringRef Sign(BufferPtr, end - BufferPtr);
        Token::TokenKind kind;
        if (Sign == "=")
            kind = Token::assign;
        else if (Sign == "-=")
            kind = Token::minus_assign;
        else if (Sign == "+=")
            kind = Token::plus_assign;
        else if (Sign == "*=")
            kind = Token::star_assign;
        else if (Sign == "/=")
            kind = Token::slash_assign;
        else if (Sign == "%=")
            kind = Token::mod_assign;
        else if (Sign == "^=")
            kind = Token::exp_assign;
        else if (Sign == "==")
            kind = Token::eq;
        else if (Sign == "!=")
            kind = Token::neq;
        else if (Sign == ">")
            kind = Token::gt;
        else if (Sign == "<")
            kind = Token::lt;
        else if (Sign == ">=")
            kind = Token::gte;
        else if (Sign == "<=")
            kind = Token::lte;
        else if (Sign == "+")
            kind = Token::plus;
        else if (Sign == "-")
            kind = Token::minus;
        else if (Sign == "*")
            kind = Token::star;
        else if (Sign == "/")
            kind = Token::slash;
        else if (Sign == "%")
            kind = Token::mod;
        else if (Sign == "^")
            kind = Token::exp;
        else
            kind = Token::unknown;
        // generate the token
        formToken(token, end, kind);
        return;
    }
    
    else
    {
        switch (*BufferPtr)
        {
#define CASE(ch, tok)                         \
    case ch:                                  \
        formToken(token, BufferPtr + 1, tok); \
        break
            CASE('(', Token::l_paren);
            CASE(')', Token::r_paren);
            CASE(';', Token::semicolon);
            CASE(':', Token::colon);
            CASE(',', Token::comma);
#undef CASE
        default:
            formToken(token, BufferPtr + 1, Token::unknown);
        }
        return;
    }
}

void Lexer::formToken(Token &Tok, const char *TokEnd,
                      Token::TokenKind Kind)
{
    Tok.Kind = Kind;
    Tok.Text = llvm::StringRef(BufferPtr, TokEnd - BufferPtr);
    BufferPtr = TokEnd;
}
