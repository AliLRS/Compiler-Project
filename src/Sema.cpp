#include "Sema.h"
#include "llvm/ADT/StringSet.h"
#include "llvm/Support/raw_ostream.h"

namespace {
class InputCheck : public ASTVisitor {
  llvm::StringSet<> Scope; // StringSet to store declared variables
  bool HasError; // Flag to indicate if an error occurred

  enum ErrorType { Twice, Not }; // Enum to represent error types: Twice - variable declared twice, Not - variable not declared

  void error(ErrorType ET, llvm::StringRef V) {
    // Function to report errors
    llvm::errs() << "Variable " << V << " is "
                 << (ET == Twice ? "already" : "not")
                 << " declared\n";
    HasError = true; // Set error flag to true
  }

public:
  InputCheck() : HasError(false) {} // Constructor

  bool hasError() { return HasError; } // Function to check if an error occurred

  // Visit function for Program nodes
  virtual void visit(Program &Node) override { 
    for (AST I = Node.begin(), E = Node.end(); I != E; ++I)
    {
      (*I)->accept(*this); // Visit each child node
    }
  };

  virtual void visit(AST &Node) override {
    if (dynamic_cast<Assignment*>(Node) != nullptr){
      (Assignment)(Node)->accept(*this);
    }
    if (dynamic_cast<Declaration*>(Node) != nullptr){
      (Declaration)(Node)->accept(*this);
    }
    if (dynamic_cast<IfStmt*>(Node) != nullptr){
      (IfStmt)(Node)->accept(*this);
    }
    if (dynamic_cast<elifStmt*>(Node) != nullptr){
      (elifStmt)(Node)->accept(*this);
    }
    if (dynamic_cast<IterStmt*>(Node) != nullptr){
      (IterStmt)(Node)->accept(*this);
    }
  }

  // Visit function for Final nodes
  virtual void visit(Final &Node) override {
    if (Node.getKind() == Final::Ident) {
      // Check if identifier is in the scope
      if (Scope.find(Node.getVal()) == Scope.end())
        error(Not, Node.getVal());
    }
  };

  // Visit function for BinaryOp nodes
  virtual void visit(BinaryOp &Node) override {
    if (Node.getLeft())
      Node.getLeft()->accept(*this);
    else
      HasError = true;

    if (Node.getRight())
      Node.getRight()->accept(*this);
    else
      HasError = true;

    if (Node.getOperator() == BinaryOp::Operator::Div && right) {
      Final * f = (Final *)right;

      if (right && f->getKind() == Final::ValueKind::Number) {
        int intval = atol(f->getVal())

        if (intval == 0) {
          llvm::errs() << "Division by zero is not allowed." << "\n";
          HasError = true;
        }
      }
    }
  };

  // Visit function for Assignment nodes
  virtual void visit(Assignment &Node) override {
    Final *dest = Node.getLeft();

    dest->accept(*this);

    if (dest->getKind() == Final::Number) {
        llvm::errs() << "Assignment destination must be an identifier.";
        HasError = true;
    }

    if (dest->getKind() == Final::Ident) {
      // Check if the identifier is in the scope
      if (Scope.find(dest->getVal()) == Scope.end())
        error(Not, dest->getVal());
    }

    if (Node.getRight())
      (BinaryOp)(Node.getRight())->accept(*this);
  };

  virtual void visit(Declaration &Node) override {
    for (llvm::StringRef I = Node.varBegin(), E = Node.varEnd(); I != E;
         ++I) {
      if (!Scope.insert(*I).second)
        error(Twice, *I); // If the insertion fails (element already exists in Scope), report a "Twice" error
    }
    for (Expr I = Node.valBegin(), E = Node.valEnd(); I != E; ++I){
      (BinaryOp)(*I)->accept(*this); // If the Declaration node has an expression, recursively visit the expression node
    }
  };

  virtual void visit(Comparison &Node) override {
    if(Node.getLeft()){
      (BinaryOp)(Node.getLeft())->accept(*this);
    }
    if(Node.getRight()){
      (BinaryOp)(Node.getRight())->accept(*this);
    }
  };

  virtual void visit(LogicalExpr &Node) override {
    if(Node.getLeft()){
      Node.getLeft()->accept(*this);
    }
    if(Node.getRight()){
      Node.getRight()->accept(*this);
    }
  };

  virtual void visit(IfStmt &Node) override {
    LogicalExpr *l = Node.getCond();
    (*l)->accept(*this);

    for (Assignment I = Node.begin(), E = Node.end(); I != E; ++I) {
      (*I)->accept(*this);
    }
    for (Assignment I = Node.beginElse(), E = Node.endElse(); I != E; ++I){
      (*I)->accept(*this);
    }
    for (elifStmt I = Node.beginElif(), E = Node.endElif(); I != E; ++I){
      (*I)->accept(*this);
    }
  };

  virtual void visit(elifStmt &Node) override {
    LogicalExpr *l = Node.getCond();
    (*l)->accept(*this);

    for (Assignment I = Node.begin(), E = Node.end(); I != E; ++I) {
      (*I)->accept(*this);
    }
  }

};
}

bool Sema::semantic(Program *Tree) {
  if (!Tree)
    return false; // If the input AST is not valid, return false indicating no errors

  InputCheck Check; // Create an instance of the InputCheck class for semantic analysis
  Tree->accept(Check); // Initiate the semantic analysis by traversing the AST using the accept function

  return Check.hasError(); // Return the result of Check.hasError() indicating if any errors were detected during the analysis
}
