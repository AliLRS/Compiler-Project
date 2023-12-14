#include "CodeGen.h"
#include "llvm/ADT/StringMap.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/IR/Constants.h"

using namespace llvm;

// Define a visitor class for generating LLVM IR from the AST.
namespace
ns{
  class ToIRVisitor : public ASTVisitor
  {
    Module *M;
    IRBuilder<> Builder;
    Type *VoidTy;
    Type *Int32Ty;
    Type *Int8PtrTy;
    Type *Int8PtrPtrTy;
    Constant *Int32Zero;
    Constant *Int32One;

    Value *V;
    StringMap<AllocaInst *> nameMap;

    FunctionType *CalcWriteFnTy;
    Function *CalcWriteFn;

  public:
    // Constructor for the visitor class.
    ToIRVisitor(Module *M) : M(M), Builder(M->getContext())
    {
      // Initialize LLVM types and constants.
      VoidTy = Type::getVoidTy(M->getContext());
      Int32Ty = Type::getInt32Ty(M->getContext());
      Int8PtrTy = Type::getInt8PtrTy(M->getContext());
      Int8PtrPtrTy = Int8PtrTy->getPointerTo();
      Int32Zero = ConstantInt::get(Int32Ty, 0, true);
      Int32One = ConstantInt::get(Int32Ty, 1, true);
      // Create a function type for the "gsm_write" function.
      CalcWriteFnTy = FunctionType::get(VoidTy, {Int32Ty}, false);
      // Create a function declaration for the "gsm_write" function.
      CalcWriteFn = Function::Create(CalcWriteFnTy, GlobalValue::ExternalLinkage, "gsm_write", M);
    }

    // Entry point for generating LLVM IR from the AST.
    void run(Program *Tree)
    {
      // Create the main function with the appropriate function type.
      FunctionType *MainFty = FunctionType::get(Int32Ty, {Int32Ty, Int8PtrPtrTy}, false);
      Function *MainFn = Function::Create(MainFty, GlobalValue::ExternalLinkage, "main", M);

      // Create a basic block for the entry point of the main function.
      BasicBlock *BB = BasicBlock::Create(M->getContext(), "entry", MainFn);
      Builder.SetInsertPoint(BB);

      // Visit the root node of the AST to generate IR.
      Tree->accept(*this);

      // Create a return instruction at the end of the main function.
      Builder.CreateRet(Int32Zero);
    }

    // Visit function for the Program node in the AST.
    virtual void visit(Program &Node) override
    {
      // Iterate over the children of the Program node and visit each child.
      for (llvm::SmallVector<AST *>::const_iterator I = Node.begin(), E = Node.end(); I != E; ++I)
    {
      (*I)->accept(*this); // Visit each child node
    }
    };

    virtual void visit(Assignment &Node) override
    {
      // Visit the right-hand side of the assignment and get its value.
      Node.getRight()->accept(*this);
      Value *val = V;

      // Get the name of the variable being assigned.
      llvm::StringRef varName = Node.getLeft()->getVal();
      
      // Get the value of the variable being assigned.
      Node.getLeft()->accept(*this);
      Value *varVal = V;

      switch (Node.getAssignKind())
      {
      case Assignment::Plus_assign:
        val = Builder.CreateNSWAdd(varVal, val);
        break;
      case Assignment::Minus_assign:
        val = Builder.CreateNSWSub(varVal, val);
        break;
      case Assignment::Star_assign:
        val = Builder.CreateNSWMul(varVal, val);
        break;
      case Assignment::Slash_assign:
        val = Builder.CreateSDiv(varVal, val);
        break;
      case Assignment::Mod_assign:
        val = Builder.CreateSRem(varVal, val);
        break;
      case Assignment::Exp_assign:
        val = CreateExp(varVal, val);
        break;
      default:
        break;
      }

      // Create a store instruction to assign the value to the variable.
      Builder.CreateStore(val, nameMap[varName]);

      // Create a call instruction to invoke the "gsm_write" function with the value.
      CallInst *Call = Builder.CreateCall(CalcWriteFnTy, CalcWriteFn, {val});
    };

    virtual void visit(Final &Node) override
    {
      if (Node.getKind() == Final::Ident)
      {
        // If the Final is an identifier, load its value from memory.
        V = Builder.CreateLoad(Int32Ty, nameMap[Node.getVal()]);
      }
      else
      {
        // If the Final is a literal, convert it to an integer and create a constant.
        int intval;
        Node.getVal().getAsInteger(10, intval);
        V = ConstantInt::get(Int32Ty, intval, true);
      }
    };

    virtual void visit(BinaryOp &Node) override
    {
      // Visit the left-hand side of the binary operation and get its value.
      Node.getLeft()->accept(*this);
      Value *Left = V;

      // Visit the right-hand side of the binary operation and get its value.
      Node.getRight()->accept(*this);
      Value *Right = V;

      // Perform the binary operation based on the operator type and create the corresponding instruction.
      switch (Node.getOperator())
      {
      case BinaryOp::Plus:
        V = Builder.CreateNSWAdd(Left, Right);
        break;
      case BinaryOp::Minus:
        V = Builder.CreateNSWSub(Left, Right);
        break;
      case BinaryOp::Mul:
        V = Builder.CreateNSWMul(Left, Right);
        break;
      case BinaryOp::Div:
        V = Builder.CreateSDiv(Left, Right);
        break;
      case BinaryOp::Mod:
        V = Builder.CreateSRem(Left, Right);
        break;
      case BinaryOp::Exp:
        V = CreateExp(Left, Right);
        break;
      default:
        break;
      }
    };

    Value* CreateExp(Value *Left, Value *Right)
    { 
      Value* res = Int32One;

      int intValue;
      
      if (ConstantInt* intConstant = dyn_cast<ConstantInt>(Right)) {
        // Get the integer value
        intValue = intConstant->getSExtValue(); // or getZExtValue() for unsigned values
        // Now, 'intValue' contains the actual integer value.
      } else {
        // Handle the case where the Value is not an integer constant
        llvm::errs() << "Error: Value is not an integer constant.\n";
      }

      for (int i = 0; i < intValue; ++i)
      {
        res = Builder.CreateNSWMul(res, Left);
      }
      return res;
    }

    virtual void visit(Declaration &Node) override
    {
      llvm::SmallVector<Value *, 8> vals;

      llvm::SmallVector<Expr *, 8>::const_iterator E = Node.valBegin();
      for (llvm::SmallVector<llvm::StringRef, 8>::const_iterator Var = Node.varBegin(), End = Node.varEnd(); Var != End; ++Var){
        if (E<Node.valEnd())
        {
          (*E)->accept(*this); // If the Declaration node has an expression, recursively visit the expression node
          vals.push_back(V);
        }
        else 
        {
          vals.push_back(nullptr);
        }
        E++;
      }
      StringRef Var;
      Value* val;
      llvm::SmallVector<Value *, 8>::const_iterator itVal = vals.begin();
      for (llvm::SmallVector<llvm::StringRef, 8>::const_iterator S = Node.varBegin(), End = Node.varEnd(); S != End; ++S){
        
        Var = *S;

        // Create an alloca instruction to allocate memory for the variable.
        nameMap[Var] = Builder.CreateAlloca(Int32Ty);
        
        // Store the initial value (if any) in the variable's memory location.
        if (*itVal != nullptr)
        {
          Builder.CreateStore(*itVal, nameMap[Var]);
        }
        else
        {
          Builder.CreateStore(Int32Zero, nameMap[Var]);
        }
        itVal++;
      }
    };

    virtual void visit(LogicalExpr &Node) override{
      // Visit the left-hand side of the Logical operation and get its value.
      Node.getLeft()->accept(*this);
      Value *Left = V;

      // Visit the right-hand side of the Logical operation and get its value.
      Node.getRight()->accept(*this);
      Value *Right = V;

      switch (Node.getOperator())
      {
      case LogicalExpr::And:
        Builder.CreateAnd(Left, Right);
        break;
      case LogicalExpr::Or:
        Builder.CreateOr(Left, Right);
        break;
      default:
        break;
      }
    };

    virtual void visit(Comparison &Node) override{
      // Visit the left-hand side of the Comparison operation and get its value.
      Node.getLeft()->accept(*this);
      Value *Left = V;

      // Visit the right-hand side of the Comparison operation and get its value.
      Node.getRight()->accept(*this);
      Value *Right = V;

      switch (Node.getOperator())
      {
      case Comparison::Equal:
        V = Builder.CreateICmpEQ(Left, Right);
        break;
      case Comparison::Not_equal:
        V = Builder.CreateICmpNE(Left, Right);
        break;
      case Comparison::Less:
        V = Builder.CreateICmpSLT(Left, Right);
        break;
      case Comparison::Greater:
        V = Builder.CreateICmpSGT(Left, Right);
        break;
      case Comparison::Less_equal:
        V = Builder.CreateICmpSLE(Left, Right);
        break;
      case Comparison::Greater_equal:
        V = Builder.CreateICmpSGE(Left, Right);
        break;
      default:
        break;
      }
    };

    virtual void visit(IterStmt &Node) override
    {
      llvm::BasicBlock* WhileCondBB = llvm::BasicBlock::Create(M->getContext(), "loopc.cond", Builder.GetInsertBlock()->getParent());
      llvm::BasicBlock* WhileBodyBB = llvm::BasicBlock::Create(M->getContext(), "loopc.body", Builder.GetInsertBlock()->getParent());
      llvm::BasicBlock* AfterWhileBB = llvm::BasicBlock::Create(M->getContext(), "after.loopc", Builder.GetInsertBlock()->getParent());

      Builder.CreateBr(WhileCondBB);
      Builder.SetInsertPoint(WhileCondBB);
      Node.getCond()->accept(*this);
      Value* val=V;
      Builder.CreateCondBr(val, WhileBodyBB, AfterWhileBB);
      Builder.SetInsertPoint(WhileBodyBB);

      for (llvm::SmallVector<Assignment* >::const_iterator I = Node.begin(), E = Node.end(); I != E; ++I)
        {
            (*I)->accept(*this);
        }

      Builder.CreateBr(WhileCondBB);

      Builder.SetInsertPoint(AfterWhileBB);
        
    };

    virtual void visit(IfStmt &Node) override{
      llvm::BasicBlock* IfCondBB = llvm::BasicBlock::Create(M->getContext(), "if.cond", Builder.GetInsertBlock()->getParent());
      llvm::BasicBlock* IfBodyBB = llvm::BasicBlock::Create(M->getContext(), "if.body", Builder.GetInsertBlock()->getParent());
      llvm::BasicBlock* AfterIfBB = llvm::BasicBlock::Create(M->getContext(), "after.if", Builder.GetInsertBlock()->getParent());

      Builder.CreateBr(IfCondBB);
      Builder.SetInsertPoint(IfCondBB);
      Node.getCond()->accept(*this);
      Value* IfCondVal=V;

      Builder.SetInsertPoint(IfBodyBB);

      for (llvm::SmallVector<Assignment* >::const_iterator I = Node.begin(), E = Node.end(); I != E; ++I)
        {
            (*I)->accept(*this);
        }

      Builder.CreateBr(AfterIfBB);

      llvm::BasicBlock* PreviousCondBB = IfCondBB;
      llvm::BasicBlock* PreviousBodyBB = IfBodyBB;
      Value* PreviousCondVal = IfCondVal;

      for (llvm::SmallVector<elifStmt *, 8>::const_iterator I = Node.beginElif(), E = Node.endElif(); I != E; ++I)
      {
        llvm::BasicBlock* ElifCondBB = llvm::BasicBlock::Create(M->getContext(), "elif.cond", Builder.GetInsertBlock()->getParent());
        llvm::BasicBlock* ElifBodyBB = llvm::BasicBlock::Create(M->getContext(), "elif.body", Builder.GetInsertBlock()->getParent());

        Builder.SetInsertPoint(PreviousCondBB);
        Builder.CreateCondBr(PreviousCondVal, PreviousBodyBB, ElifCondBB);

        Builder.SetInsertPoint(ElifCondBB);
        (*I)->getCond()->accept(*this);
        Value* ElifCondVal = V;

        Builder.SetInsertPoint(ElifBodyBB);
        (*I)->accept(*this);
        Builder.CreateBr(AfterIfBB);

        PreviousCondBB = ElifCondBB;
        PreviousCondVal = ElifCondVal;
        PreviousBodyBB = ElifBodyBB;
      }
      if (Node.beginElse() != Node.endElse()) {
        llvm::BasicBlock* ElseBB = llvm::BasicBlock::Create(M->getContext(), "else.body", Builder.GetInsertBlock()->getParent());
        Builder.SetInsertPoint(ElseBB);
        for (llvm::SmallVector<Assignment* >::const_iterator I = Node.beginElse(), E = Node.endElse(); I != E; ++I)
        {
            (*I)->accept(*this);
        }
        Builder.CreateBr(AfterIfBB);

        Builder.SetInsertPoint(PreviousCondBB);
        Builder.CreateCondBr(PreviousCondVal, PreviousBodyBB, ElseBB);
      }
      else {
        Builder.SetInsertPoint(PreviousCondBB);
        Builder.CreateCondBr(IfCondVal, PreviousBodyBB, AfterIfBB);
      }

      Builder.SetInsertPoint(AfterIfBB);
    };

    virtual void visit(elifStmt &Node) override{
      for (llvm::SmallVector<Assignment* >::const_iterator I = Node.begin(), E = Node.end(); I != E; ++I)
        {
            (*I)->accept(*this);
        }
    };
  };
}; // namespace

void CodeGen::compile(Program *Tree)
{
  // Create an LLVM context and a module.
  LLVMContext Ctx;
  Module *M = new Module("simple-compiler", Ctx);

  // Create an instance of the ToIRVisitor and run it on the AST to generate LLVM IR.
  ns::ToIRVisitor *ToIR = new ns::ToIRVisitor(M);


  ToIR->run(Tree);

  // Print the generated module to the standard output.
  M->print(outs(), nullptr);
}
