#ifndef IR_CLANG_HH
#define IR_CLANG_HH

#include <omega.h>
#include "ir_code.hh"
#include "chill_error.hh"

#define __STDC_CONSTANT_MACROS

#include "clang/AST/Decl.h"
#include "clang/AST/Expr.h"
#include "clang/AST/ParentMap.h"

#include "clang/AST/ASTConsumer.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/GlobalDecl.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Frontend/ASTConsumers.h"
#include "clang/Frontend/FrontendActions.h"

#include "clang/CodeGen/CodeGenAction.h"
#include "clang/Driver/Compilation.h"
#include "clang/Driver/Driver.h"
#include "clang/Driver/Tool.h"
#include "clang/Frontend/CompilerInvocation.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendDiagnostic.h"
#include "clang/Frontend/TextDiagnosticPrinter.h"
#include "clang/Basic/DiagnosticOptions.h"

#include "chillAST.h"

extern std::vector<chillAST_VarDecl *> VariableDeclarations;  // a global.   TODO
extern std::vector<chillAST_FunctionDecl *> FunctionDeclarations;

struct IR_chillScalarSymbol : public IR_ScalarSymbol {
  chillAST_VarDecl *chillvd;

  IR_chillScalarSymbol(const IR_Code *ir, chillAST_VarDecl *vd) {
    ir_ = ir;
    chillvd = vd;
  }

  std::string name() const;

  int size() const;

  bool operator==(const IR_Symbol &that) const;

  IR_Symbol *clone() const;
};


struct IR_chillArraySymbol : public IR_ArraySymbol {
  //int indirect_;             // TODO Doc
  int offset_;                 // TODO Doc
  chillAST_VarDecl *chillvd;

  IR_chillArraySymbol(const IR_Code *ir, chillAST_VarDecl *vd, int offset = 0) {
    ir_ = ir;
    chillvd = vd;
    //indirect_ = indirect;
    offset_ = offset;
  }

  // No Fortran support!
  IR_ARRAY_LAYOUT_TYPE layout_type() const {
    return IR_ARRAY_LAYOUT_ROW_MAJOR;
  }

  std::string name() const;

  int elem_size() const;

  int n_dim() const;

  omega::CG_outputRepr *size(int dim) const;

  bool operator!=(const IR_Symbol &that) const;

  bool operator==(const IR_Symbol &that) const;

  IR_Symbol *clone() const;

  // TODO Hack to pass build
  IR_CONSTANT_TYPE elem_type() const {
    fprintf(stderr, "Not implemented elem_type in IR_chillArraySymbol");
    return IR_CONSTANT_UNKNOWN;
  };

};


struct IR_chillConstantRef : public IR_ConstantRef {
  union {
    omega::coef_t i_;
    double f_;
  };

  IR_chillConstantRef(const IR_Code *ir, omega::coef_t i) {
    ir_ = ir;
    type_ = IR_CONSTANT_INT;
    i_ = i;
  }

  IR_chillConstantRef(const IR_Code *ir, double f) {
    ir_ = ir;
    type_ = IR_CONSTANT_FLOAT;
    f_ = f;
  }

  omega::coef_t integer() const {
    assert(is_integer());
    return i_;
  }

  bool operator==(const IR_Ref &that) const;

  omega::CG_outputRepr *convert();

  IR_Ref *clone() const;

};

enum OP_POSITION {
  OP_DEST = -1, OP_UNKNOWN, OP_SRC
};
#define OP_LEFT  OP_DEST
#define OP_RIGHT OP_SRC

struct IR_chillScalarRef : public IR_ScalarRef {
  /*!
   * @brief the position of the operand
   * -1 means destination operand, 0== unknown, 1 == source operand
   */
  OP_POSITION op_pos_;
  //chillAST_BinaryOperator *bop;  // binary op that contains this scalar?
  chillAST_DeclRefExpr *dre;  //!< declrefexpr that uses this scalar ref, if that exists
  chillAST_VarDecl *chillvd; //!< the vardecl for this scalar

  IR_chillScalarRef(const IR_Code *ir, chillAST_BinaryOperator *ins, OP_POSITION pos) {
    CHILL_ERROR("Not implemented");
    exit(-1);
    // this constructor takes a binary operation and an indicator of which side of the op to use,
    // and finds the scalar in the lhs or rhs of the binary op.
    ir_ = ir;
    dre = NULL;
    if (pos == OP_LEFT) {
      chillAST_Node *lhs = ins->getLHS();
      if (lhs->isDeclRefExpr()) {
        chillAST_DeclRefExpr *DRE = (chillAST_DeclRefExpr *) lhs;
        dre = DRE;
        chillvd = DRE->getVarDecl();
      } else if (lhs->isVarDecl()) {
        chillvd = (chillAST_VarDecl *) lhs;
      } else {
        CHILL_ERROR("IR_chillScalarRef constructor, I'm confused\n");
        exit(-1);
      }
    } else {
      chillAST_Node *rhs = ins->getRHS();
      if (rhs->isDeclRefExpr()) {
        chillAST_DeclRefExpr *DRE = (chillAST_DeclRefExpr *) rhs;
        dre = DRE;
        chillvd = DRE->getVarDecl();
      } else if (rhs->isVarDecl()) {
        chillvd = (chillAST_VarDecl *) rhs;
      } else {
        CHILL_ERROR("IR_chillScalarRef constructor, I'm confused\n");
        exit(-1);
      }
    }
    op_pos_ = pos;
  }

  IR_chillScalarRef(const IR_Code *ir, chillAST_DeclRefExpr *d) {
    ir_ = ir;
    dre = d;
    chillvd = d->getVarDecl();
    op_pos_ = OP_UNKNOWN;
  }

  IR_chillScalarRef(const IR_Code *ir, chillAST_VarDecl *vardecl) {
    ir_ = ir;
    dre = NULL;
    chillvd = vardecl;
    op_pos_ = OP_UNKNOWN;
  }


  bool is_write() const;

  IR_ScalarSymbol *symbol() const;

  bool operator==(const IR_Ref &that) const;

  omega::CG_outputRepr *convert();

  IR_Ref *clone() const;
};


struct IR_chillArrayRef : public IR_ArrayRef {
  chillAST_ArraySubscriptExpr *chillASE;
  int iswrite;

  IR_chillArrayRef(const IR_Code *ir, chillAST_ArraySubscriptExpr *ase, int write) {
    ir_ = ir;
    chillASE = ase;
    iswrite = write;
  }

  bool is_write() const;

  omega::CG_outputRepr *index(int dim) const;

  IR_ArraySymbol *symbol() const;

  bool operator!=(const IR_Ref &that) const;

  bool operator==(const IR_Ref &that) const;

  omega::CG_outputRepr *convert();

  IR_Ref *clone() const;

  // Not implemented
  // virtual void Dump() const;
};


struct IR_chillLoop : public IR_Loop {
  int step_size_;

  chillAST_DeclRefExpr *chillindex;   // the loop index variable  (I)  // was DeclRefExpr
  chillAST_ForStmt *chillforstmt;
  chillAST_Node *chilllowerbound;
  chillAST_Node *chillupperbound;
  chillAST_Node *chillbody;    // presumably a compound statement, but not guaranteeed
  IR_CONDITION_TYPE conditionoperator;

  IR_chillLoop(const IR_Code *ir, clang::ForStmt *tf);

  IR_chillLoop(const IR_Code *ir, chillAST_ForStmt *forstmt);

  ~IR_chillLoop() {}

  IR_ScalarSymbol *index() const { return new IR_chillScalarSymbol(ir_, chillindex->getVarDecl()); }

  omega::CG_outputRepr *lower_bound() const;

  omega::CG_outputRepr *upper_bound() const;

  IR_CONDITION_TYPE stop_cond() const;

  IR_Block *body() const;

  // Handle following types of increment expressions:

  // Unary increment/decrement
  // i += K OR i -= K
  // i = i + K OR i = i - K
  // where K is positive
  int step_size() const { return step_size_; }  // K is always an integer ???
  IR_Control *clone() const;

  IR_Block *convert();

  virtual void dump() const;
};


struct IR_chillBlock : public IR_Block {   // ONLY ONE OF bDecl or cs_ will be nonNULL  ??
private:
  //StmtList bDecl_;    // declarations in the block?? 
  //clang::CompoundStmt *cs_;  // will a block always have a compound statement? (no)
  //StmtList *stmts;  //  ?? 
public:

  // Block is a basic block?? (no, just a chunk of code )
  std::vector<chillAST_Node *> statements;

  IR_chillBlock(const IR_chillBlock *CB) {  // clone existing IR_chillBlock
    ir_ = CB->ir_;
    for (int i = 0; i < CB->statements.size(); i++) statements.push_back(CB->statements[i]);
  }

  IR_chillBlock(const IR_Code *ir, std::vector<chillAST_Node *> ast) : statements(ast) {
    ir_ = ir;
  }

  IR_chillBlock(const IR_Code *ir, chillAST_Node *ast = NULL) { //  : cs_(NULL), bDecl_(NULL) {
    ir_ = ir;
    if (ast != NULL)
      statements.push_back(ast);
  }


  ~IR_chillBlock() {}

  omega::CG_outputRepr *extract() const;

  omega::CG_outputRepr *original() const;

  IR_Control *clone() const;

  //StmtList getStmtList() const;
  std::vector<chillAST_Node *> getStmtList() const;

  int numstatements() { return statements.size(); };

  void addStatement(chillAST_Node *s);

  void dump() const;
};

struct IR_chillIf: public IR_If {
  chillAST_Node *code;

  IR_chillIf(const IR_Code *ir, chillAST_Node *ti) {
    ir_ = ir;
    code = ti;
  }
  ~IR_chillIf() {
  }
  omega::CG_outputRepr *condition() const;
  IR_Block *then_body() const;
  IR_Block *else_body() const;
  IR_Block *convert();
  IR_Control *clone() const;
};

class aClangCompiler {
private:
  //Chill_ASTConsumer *astConsumer_;
  clang::ASTContext *astContext_;

  clang::DiagnosticOptions *diagnosticOptions;
  clang::TextDiagnosticPrinter *pTextDiagnosticPrinter;
  llvm::IntrusiveRefCntPtr<clang::DiagnosticIDs> diagID;
  clang::DiagnosticsEngine *diagnosticsEngine;
  clang::CompilerInstance *Clang;
  clang::Preprocessor *preprocessor;
  //FileManager *FileMgr;
  //clang::CompilerInvocation *CI;

  clang::FileManager *fileManager;
  clang::SourceManager *sourceManager;

  // UNUSED? 
  clang::Diagnostic *diagnostic;
  clang::LangOptions *languageOptions;
  clang::HeaderSearchOptions *headerSearchOptions;
  //clang::HeaderSearch *headerSearch;
  std::shared_ptr<clang::TargetOptions> targetOptions;
  clang::TargetInfo *pTargetInfo;
  clang::PreprocessorOptions *preprocessorOptions;
  clang::FrontendOptions *frontendOptions;
  clang::IdentifierTable *idTable;
  clang::SelectorTable *selTable;
  clang::Builtin::Context *builtinContext;


public:
  char *SourceFileName;
  chillAST_SourceFile *entire_file_AST; // TODO move out of public

  aClangCompiler(char *filename); // constructor
  chillAST_FunctionDecl *findprocedurebyname(char *name);   // someday, return the chill AST
  clang::FunctionDecl *FD;

  //Chill_ASTConsumer *getASTConsumer() { return astConsumer_; }
  clang::ASTContext *getASTContext() { return astContext_; }

  clang::SourceManager *getASTSourceManager() { return sourceManager; };
};


// singleton class for global clang initialization
// TODO: Add support for multiple files in same script
class IR_clangCode_Global_Init {
private:
  static IR_clangCode_Global_Init *pinstance;   // the one and only
  // protecting the constructor is the SINGLETON PATTERN.   a global by any other name
  // IR_clangCode_Global_Init(); 
  ~IR_clangCode_Global_Init(); // is this hidden, too?
  chillAST_FunctionDecl *chillFD; // the original C code

  clang::ASTContext *astContext_;
  clang::SourceManager *sourceManager;
public:
  clang::ASTContext *getASTContext() { return astContext_; }

  clang::SourceManager *getSourceManager() { return sourceManager; };

  static IR_clangCode_Global_Init *Instance(char **argv);

  static IR_clangCode_Global_Init *Instance() { return pinstance; };
  aClangCompiler *ClangCompiler; // this is the thing we really just want one of


  void setCurrentFunction(chillAST_Node *F) { chillFD = (chillAST_FunctionDecl *) F; };

  chillAST_FunctionDecl *getCurrentFunction() { return chillFD; };


  void setCurrentASTContext(clang::ASTContext *A) { astContext_ = A; };

  clang::ASTContext *getCurrentASTContext() { return astContext_; };

  void setCurrentASTSourceManager(clang::SourceManager *S) { sourceManager = S; };

  clang::SourceManager *getCurrentASTSourceManager() { return sourceManager; };
};


class IR_clangCode : public IR_Code {   // for an entire file?  A single function?
protected:

  //  
  char *filename;
  char *procedurename;
  char *outfilename;


  chillAST_Node *entire_file_AST;
  chillAST_FunctionDecl *chillfunc;   // the function we're currenly modifying

  std::vector<chillAST_VarDecl> entire_file_symbol_table;
  // loop symbol table??   for (int i=0;  ... )  ??



  clang::FunctionDecl *func_;   // a clang construct   the function we're currenly modifying
  clang::ASTContext *astContext_;
  clang::SourceManager *sourceManager;

  // firstScope;
  //   symboltable1,2,3 ??
  // topleveldecls
  // 

public:
  clang::ASTContext *getASTContext() { return astContext_; };

  clang::SourceManager *getASTSourceManager() { return sourceManager; };

  IR_clangCode(const char *filename, const char *proc_name);
  IR_clangCode(const char *filename, const char *proc_name, const char *dest_name);

  ~IR_clangCode();

  IR_ScalarSymbol *CreateScalarSymbol(const IR_Symbol *sym, int i);

  IR_ArraySymbol *CreateArraySymbol(const IR_Symbol *sym, std::vector<omega::CG_outputRepr *> &size, int i);

  IR_ScalarRef *CreateScalarRef(const IR_ScalarSymbol *sym);

  IR_ArrayRef *CreateArrayRef(const IR_ArraySymbol *sym, std::vector<omega::CG_outputRepr *> &index);

  virtual bool FromSameStmt(IR_ArrayRef *A, IR_ArrayRef *B);

  int ArrayIndexStartAt() { return 0; } // TODO FORTRAN

  std::vector<IR_ScalarRef *> FindScalarRef(const omega::CG_outputRepr *repr) const;

  std::vector<IR_ArrayRef *> FindArrayRef(const omega::CG_outputRepr *repr) const;

  std::vector<IR_Control *> FindOneLevelControlStructure(const IR_Block *block) const;

  IR_Block *MergeNeighboringControlStructures(const std::vector<IR_Control *> &controls) const;

  IR_Block *GetCode() const;

  void ReplaceCode(IR_Control *old, omega::CG_outputRepr *repr);

  void ReplaceExpression(IR_Ref *old, omega::CG_outputRepr *repr);

  IR_CONDITION_TYPE QueryBooleanExpOperation(const omega::CG_outputRepr *) const;

  IR_OPERATION_TYPE QueryExpOperation(const omega::CG_outputRepr *repr) const;

  std::vector<omega::CG_outputRepr *> QueryExpOperand(const omega::CG_outputRepr *repr) const;

  IR_Ref *Repr2Ref(const omega::CG_outputRepr *) const;

  friend class IR_chillArraySymbol;

  friend class IR_chillArrayRef;
};

#endif
