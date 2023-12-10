#ifndef AST_H
#define AST_H

#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringRef.h"

// Forward declarations of classes used in the AST
class AST;
class Expr;
class Program;
class Declaration;
class Final;
class BinaryOp;
class Assignment;
class Comparison;
class LogicalExpr;
class IfStmt;
class IterStmt;
class elifStmt;


// ASTVisitor class defines a visitor pattern to traverse the AST
class ASTVisitor
{
public:
  // Virtual visit functions for each AST node type
  virtual void visit(AST &) {}               // Visit the base AST node
  virtual void visit(Expr &) {}              // Visit the expression node
  virtual void visit(Program &) = 0;         // Visit the group of expressions node
  virtual void visit(Final &) = 0;           // Visit the Final node
  virtual void visit(BinaryOp &) = 0;        // Visit the binary operation node
  virtual void visit(Assignment &) = 0;      // Visit the assignment expression node
  virtual void visit(Declaration &) = 0;     // Visit the variable declaration node
  virtual void visit(Comparison &) = 0;      // Visit the Comparison node
  virtual void visit(LogicalExpr &) = 0;     // Visit the LogicalExpr node
  virtual void visit(IfStmt &) = 0;          // Visit the IfStmt node
  virtual void visit(IterStmt &) = 0;        // Visit the IterStmt node
  virtual void visit(elifStmt &) = 0;        // Visit the elifStmt node
};

// AST class serves as the base class for all AST nodes
class AST
{
public:
  virtual ~AST() {}
  virtual void accept(ASTVisitor &V) = 0;    // Accept a visitor for traversal
};

// Expr class represents an expression in the AST
class Expr : public AST
{
public:
  Expr() {}
};

// Program class represents a group of expressions in the AST
class Program : public Expr
{
  using dataVector = llvm::SmallVector<AST *>;

private:
  dataVector data;                          // Stores the list of expressions

public:
  Program(llvm::SmallVector<Expr *> data) : data(data) {}

  llvm::SmallVector<Expr *> getdata() { return data; }

  dataVector::const_iterator begin() { return data.begin(); }

  dataVector::const_iterator end() { return data.end(); }

  virtual void accept(ASTVisitor &V) override
  {
    V.visit(*this);
  }
};

// Declaration class represents a variable declaration with an initializer in the AST
class Declaration : public Expr
{
  using VarVector = llvm::SmallVector<llvm::StringRef, 8>;
  using ValueVector = llvm::SmallVector<Expr *, 8>;
  VarVector Vars;                           // Stores the list of variables
  ValueVector Values;                       // Stores the list of initializers

public:
  // Declaration(llvm::SmallVector<llvm::StringRef, 8> Vars, Expr *E) : Vars(Vars), E(E) {}
  Declaration(llvm::SmallVector<llvm::StringRef, 8> Vars, llvm::SmallVector<Expr *, 8> Values) :
  Vars(Vars), Values(Values) {}

  VarVector::const_iterator begin() { return Vars.begin(); }

  VarVector::const_iterator end() { return Vars.end(); }

  // ValueVector::const_iterator begin() { return Values.begin(); }

  // ValueVector::const_iterator end() { return Values.end(); }

  virtual void accept(ASTVisitor &V) override
  {
    V.visit(*this);
  }
};


// Final class represents a Final in the AST (either an identifier or a number)
class Final : public Expr
{
public:
  enum ValueKind
  {
    Ident,
    Number
  };

private:
  ValueKind Kind;                            // Stores the kind of Final (identifier or number)
  llvm::StringRef Val;                       // Stores the value of the Final

public:
  Final(ValueKind Kind, llvm::StringRef Val) : Kind(Kind), Val(Val) {}

  ValueKind getKind() { return Kind; }

  llvm::StringRef getVal() { return Val; }

  virtual void accept(ASTVisitor &V) override
  {
    V.visit(*this);
  }
};

// BinaryOp class represents a binary operation in the AST (plus, minus, multiplication, division)
class BinaryOp : public Expr
{
public:
  enum Operator
  {
    Plus,
    Minus,
    Mul,
    Div,
    Mod,
    Exp
  };

private:
  Expr *Left;                               // Left-hand side expression
  Expr *Right;                              // Right-hand side expression
  Operator Op;                              // Operator of the binary operation

public:
  BinaryOp(Operator Op, Expr *L, Expr *R) : Op(Op), Left(L), Right(R) {}

  Expr *getLeft() { return Left; }

  Expr *getRight() { return Right; }

  Operator getOperator() { return Op; }

  virtual void accept(ASTVisitor &V) override
  {
    V.visit(*this);
  }
};

// Assignment class represents an assignment expression in the AST
class Assignment : public Expr
{
  public:
  enum AssignKind
  {
    Assign,         // =
    Minus_assign,   // -=
    Plus_assign,    // +=
    Star_assign,    // *=
    Slash_assign,   // /=
    Mod_assign,     // %=
    Exp_assign      // ^=
};
private:
  Final *Left;                             // Left-hand side Final (identifier)
  Expr *Right;                              // Right-hand side expression
  AssignKind AK;                            // Kind of assignment

public:
  Assignment(Final *L, Expr *R, AssignKind AK) : Left(L), Right(R), AK(AK) {}

  Final *getLeft() { return Left; }

  Expr *getRight() { return Right; }

  AssignKind getAssignKind() { return AK; }

  virtual void accept(ASTVisitor &V) override
  {
    V.visit(*this);
  }
};

// Comparison class represents a comparison expression in the AST
class Comparison : public Expr
{
  public:
  enum Operator
  {
    Equal,          // ==
    Not_equal,      // !=
    Greater,        // >
    Less,           // <
    Greater_equal,  // >=
    Less_equal      // <=
  };
    
private:
  Expr *Left;                                // Left-hand side expression
  Expr *Right;                               // Right-hand side expression
  Operator Op;                                  // Kind of assignment

public:
  Comparison(Expr *L, Expr *R, Operator Op) : Left(L), Right(R), Op(Op) {}

  Expr *getLeft() { return Left; }

  Expr *getRight() { return Right; }

  Operator getOperator() { return Op; }

  virtual void accept(ASTVisitor &V) override
  {
    V.visit(*this);
  }
};

// LogicalExpr class represents a logical expression in the AST
class LogicalExpr : public Expr
{
  public:
  enum Operator
  {
    And,          // &&
    Or,           // ||
  };

private:
  Expr *Left;                                // Left-hand side expression
  Expr *Right;                               // Right-hand side expression
  Operator Op;                                  // Kind of assignment

public:
  LogicalExpr(Expr *L, Expr *R, Operator Op) : Left(L), Right(R), Op(Op) {}

  Expr *getLeft() { return Left; }

  Expr *getRight() { return Right; }

  Operator getOperator() { return Op; }

  virtual void accept(ASTVisitor &V) override
  {
    V.visit(*this);
  }
};

class elifStmt : public Expr
{
using assignmentsVector = llvm::SmallVector<Expr *, 8>;
assignmentsVector assignments;

private:
  Expr *Cond;

public:
  elifStmt(Expr *Cond, llvm::SmallVector<Expr *, 8> assignments) : Cond(Cond), assignments(assignments) {}

  Expr *getCond() { return Cond; }

  assignmentsVector::const_iterator begin() { return assignments.begin(); }

  assignmentsVector::const_iterator end() { return assignments.end(); }

  virtual void accept(ASTVisitor &V) override
  {
    V.visit(*this);
  }

}

class IfStmt : public Expr
{
using assignmentsVector = llvm::SmallVector<Expr *, 8>;
using elifVector = llvm::SmallVector<elifStmt *, 8>;
assignmentsVector ifAssignments;
assignmentsVector elseAssignments;
elifVector elifStmts;


private:
  Expr *Cond;

public:
  IfStmt(Expr *Cond, llvm::SmallVector<Expr *, 8> ifAssignments, llvm::SmallVector<Expr *, 8> elseAssignments, llvm::SmallVector<elifStmt *, 8> elifStmts) : Cond(Cond), ifAssignments(ifAssignments), elseAssignments(elseAssignments), elifStmts(elifStmts) {}

  Expr *getCond() { return Cond; }

  assignmentsVector::const_iterator begin() { return ifAssignments.begin(); }

  assignmentsVector::const_iterator end() { return ifAssignments.end(); }

  assignmentsVector::const_iterator beginElse() { return elseAssignments.begin(); }

  assignmentsVector::const_iterator endElse() { return elseAssignments.end(); }

  elifVector::const_iterator beginElif() { return elifStmts.begin(); }

  elifVector::const_iterator endElif() { return elifStmts.end(); }

  virtual void accept(ASTVisitor &V) override
  {
    V.visit(*this);
  }
};

class IterStmt : public Expr
{
using assignmentsVector = llvm::SmallVector<Expr *, 8>;
assignmentsVector assignments;

private:
  Expr *Cond;

public:
  IterStmt(Expr *Cond, llvm::SmallVector<Expr *, 8> assignments) : Cond(Cond), assignments(assignments) {}

  Expr *getCond() { return Cond; }

  assignmentsVector::const_iterator begin() { return assignments.begin(); }

  assignmentsVector::const_iterator end() { return assignments.end(); }

  virtual void accept(ASTVisitor &V) override
  {
    V.visit(*this);
  }
};

#endif
