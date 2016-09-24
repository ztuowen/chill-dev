//
// Created by ztuowen on 9/24/16.
//

#include "printers/dump.h"

using namespace chill::printer;
using namespace std;

void dumpVector(GenericPrinter *p, string ident, chillAST_NodeList *n, ostringstream &o) {
  for (int i = 0; i < n->size(); ++i)
    p->print("", (*n)[i], o);
}

void Dump::print(string ident, chillAST_Node *n, ostringStream &o) {
  o << "(" << n->getTypeString() << " ";
  if (n->getParameters()) {
    o << "(Params: ";
    dumpVector(this, ident, n->getParameters(), o);
    o << ") ";
  }
  if (n->getSymbolTable()) {
    o << "(VarScope: ";
    dumpVector(this, ident, n->getSymbolTable(), o);
    o << ") ";
  }
  if (n->getTypedefTable()) {
    o << "(TypeDef: ";
    dumpVector(this, ident, n->getTypedefTable(), o);
    o << ") ";
  }
  o << ": ";
  // Recurse
  GenericPrinter::print(ident, n, o);
  o << ") ";
}

void Dump::print(std::string ident, chillAST_ArraySubscriptExpr *n, std::ostringstream &o);

void Dump::print(std::string ident, chillAST_BinaryOperator *n, std::ostringstream &o);

void Dump::print(std::string ident, chillAST_CallExpr *n, std::ostringstream &o);

void Dump::print(std::string ident, chillAST_CompoundStmt *n, std::ostringstream &o);

void Dump::print(std::string ident, chillAST_CStyleAddressOf *n, std::ostringstream &o);

void Dump::print(std::string ident, chillAST_CStyleCastExpr *n, std::ostringstream &o);

void Dump::print(std::string ident, chillAST_CudaFree *n, std::ostringstream &o);

void Dump::print(std::string ident, chillAST_CudaKernelCall *n, std::ostringstream &o);

void Dump::print(std::string ident, chillAST_CudaMalloc *n, std::ostringstream &o) {
  print(ident, n->devPtr, o);
  print(ident, n->sizeinbytes, o);
}

void Dump::print(std::string ident, chillAST_CudaMemcpy *n, std::ostringstream &o) {
  o << cudaMemcpyKind << " ";
  print(ident, n->dest, o);
  print(ident, n->src, o);
  print(ident, n->size, o);
};

void Dump::print(std::string ident, chillAST_CudaSyncthreads *n, std::ostringstream &o);

void Dump::print(std::string ident, chillAST_DeclRefExpr *n, std::ostringstream &o) {
  chillAST_VarDecl *vd = n->getVarDecl();
  if (vd)
    if (vd->isAParameter) o << "ParmVar "; else o << "Var ";
  o << n->declarationName << " ";
  chillAST_FunctionDecl *fd = n->getFunctionDecl();
  if (fd) dumpVector(this, ident, fd->getParameters(), o);
}

void Dump::print(std::string ident, chillAST_FloatingLiteral *n, std::ostringstream &o);

void Dump::print(std::string ident, chillAST_ForStmt *n, std::ostringstream &o) {
  print(ident, n->init, o);
  print(ident, n->cond, o);
  print(ident, n->incr, o);
  print(ident, n->body, o);
}

void Dump::print(std::string ident, chillAST_Free *n, std::ostringstream &o);

void Dump::print(std::string ident, chillAST_FunctionDecl *n, std::ostringstream &o) {
  if (n->filename) o << filename << " ";
  if (n->isFromSourceFile) o << "FromSourceFile" << " ";
  o << n->returnType << " " << n->functionName << " ";
  if (n->body) print(ident, n->body, o);
}

void Dump::print(std::string ident, chillAST_IfStmt *n, std::ostringstream &o) {
  print(ident, n->cond, o);
  print(ident, n->thenpart, o);
  if (n->elsepart)
    print(ident, n->elsepart, o);
}

void Dump::print(std::string ident, chillAST_IntegerLiteral *n, std::ostringstream &o);

void Dump::print(std::string ident, chillAST_ImplicitCastExpr *n, std::ostringstream &o) {
  print(ident, n->subexpr, o);
}

void Dump::print(std::string ident, chillAST_MacroDefinition *n, std::ostringstream &o) {
  o << n->macroName << " ";
  dumpVector(this, ident, n->getParameters(), o);
  print(ident, n->getBody(), o);
  if (n->rhsString) o << " (aka " << n->rhsString << ") ";
}

void Dump::print(std::string ident, chillAST_Malloc *n, std::ostringstream &o) {
  print(ident, n->sizeexpr, o);
}

void Dump::print(std::string ident, chillAST_MemberExpr *n, std::ostringstream &o) {
  print(ident, n->base, o);
  if (n->exptype == CHILLAST_MEMBER_EXP_ARROW) o << "-> ";
  else o << ". ";
  o << n->member << " ";
}

void Dump::print(std::string ident, chillAST_NULL *n, std::ostringstream &o) {
  o << "(NULL) ";
}

void Dump::print(std::string ident, chillAST_NoOp *n, std::ostringstream &o) {};

void Dump::print(std::string ident, chillAST_ParenExpr *n, std::ostringstream &o);

void Dump::print(std::string ident, chillAST_Preprocessing *n, std::ostringstream &o);

void Dump::print(std::string ident, chillAST_RecordDecl *n, std::ostringstream &o) {
  // TODO access control
  o << n->getName() << " ";
  o << n->isAStruct() << " ";
  o << n->isAUnion() << " ";
}

void Dump::print(std::string ident, chillAST_ReturnStmt *n, std::ostringstream &o) {
  if (n->returnvalue) print(ident, n->returnvalue, o);
}

void Dump::print(std::string ident, chillAST_Sizeof *n, std::ostringstream &o);

void Dump::print(std::string ident, chillAST_SourceFile *n, std::ostringstream &o);

void Dump::print(std::string ident, chillAST_TypedefDecl *n, std::ostringstream &o) {
  o << n->underlyingtype << " " << n->newtype << " " << n->arraypart << " ";
}

void Dump::print(std::string ident, chillAST_TernaryOperator *n, std::ostringstream &o);

void Dump::print(std::string ident, chillAST_UnaryOperator *n, std::ostringstream &o);

void Dump::print(std::string ident, chillAST_VarDecl *n, std::ostringstream &o);

