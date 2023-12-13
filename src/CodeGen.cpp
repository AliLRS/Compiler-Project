#include "CodeGen.h"
#include "llvm/ADT/StringMap.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/ADT/SmallVector.h"

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

    Value *V;
    StringMap<AllocaInst *> nameMap;

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

      // Create a store instruction to assign the value to the variable.
      Builder.CreateStore(val, nameMap[varName]);

      // Create a function type for the "gsm_write" function.
      FunctionType *CalcWriteFnTy = FunctionType::get(VoidTy, {Int32Ty}, false);

      // Create a function declaration for the "gsm_write" function.
      Function *CalcWriteFn = Function::Create(CalcWriteFnTy, GlobalValue::ExternalLinkage, "gsm_write", M);

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
      // case BinaryOp::Exp:
      //   V = CreateExp(Left, Right);
      //   break;
      }
    };

    // Value CreateExp(Value *Left, Value *Right)
    // {
    //   Value res = Int32Zero;
    //   for (int i = 0; i < Right; i++)
    //   {
    //     res += Builder.CreateNSWMul(res, Left);
    //   }
    //   return res;
    // }

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

    virtual void visit(Comparison &Node) override{
      return;
    };

    virtual void visit(IterStmt &Node) override{
      return;
    };

    virtual void visit(IfStmt &Node) override{
      return;
    };

    virtual void visit(elifStmt &Node) override{
      return;
    };
    
    virtual void visit(LogicalExpr &Node) override{
      return;
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
