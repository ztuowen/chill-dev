//
// Created by ztuowen on 9/24/16.
//

#include "printer/dump.h"
#include "chillAST.h"

using namespace chill::printer;
using namespace std;

void dumpVector(GenericPrinter *p, string ident, chillAST_NodeList *n, ostream &o) {
  for (int i = 0; i < n->size(); ++i)
    p->print(ident, (*n)[i], o);
}

void dumpVector(GenericPrinter *p, string ident, chillAST_SymbolTable *n, ostream &o) {
  for (int i = 0; i < n->size(); ++i)
    p->print(ident, (*n)[i], o);
}

void dumpVector(GenericPrinter *p, string ident, chillAST_TypedefTable *n, ostream &o) {
  for (int i = 0; i < n->size(); ++i)
    p->print(ident, (*n)[i], o);
}

void Dump::print(string ident, chillAST_Node *n, ostream &o) {
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

void Dump::printS(std::string ident, chillAST_ArraySubscriptExpr *n, std::ostream &o) {
  if (n->basedecl)
    o << "(" << n->basedecl->varname << ") ";
  if (n->basedecl && n->basedecl->vartype)
    o << n->basedecl->vartype;
  if (n->imwrittento) {
    if (n->imreadfrom)
      o << "lvalue AND rvalue ";
    else
      o << "lvalue ";
  } else o << "rvalue ";
  print(ident, n->base, o);
  print(ident, n->index, o);
}

void Dump::printS(std::string ident, chillAST_BinaryOperator *n, std::ostream &o) {
  o << n->op << " ";
  if (n->getLHS()) print(ident, n->getLHS(), o);
  else o << "(NULL) ";
  if (n->getRHS()) print(ident, n->getRHS(), o);
  else o << "(NULL) ";
}

void Dump::printS(std::string ident, chillAST_CallExpr *n, std::ostream &o) {
  if (n->getCallee())
    print(ident, n->getCallee(), o);
}

void Dump::printS(std::string ident, chillAST_CompoundStmt *n, std::ostream &o) {
  dumpVector(this, ident, n->getChildren(), o);
}

void Dump::printS(std::string ident, chillAST_CStyleAddressOf *n, std::ostream &o) {
  print(ident, n->getSubExpr(), o);
}

void Dump::printS(std::string ident, chillAST_CStyleCastExpr *n, std::ostream &o) {
  o << n->towhat << " ";
  print(ident, n->getSubExpr(), o);
}

void Dump::printS(std::string ident, chillAST_CudaFree *n, std::ostream &o) {
  print(ident, n->getPointer(), o);
}

void Dump::printS(std::string ident, chillAST_CudaKernelCall *n, std::ostream &o) {
  CHILL_ERROR("Not implemented");
}

void Dump::printS(std::string ident, chillAST_CudaMalloc *n, std::ostream &o) {
  print(ident, n->getDevPtr(), o);
  print(ident, n->getSize(), o);
}

void Dump::printS(std::string ident, chillAST_CudaMemcpy *n, std::ostream &o) {
  o << n->cudaMemcpyKind << " ";
  print(ident, n->getDest(), o);
  print(ident, n->getSrc(), o);
  print(ident, n->getSize(), o);
}

void Dump::printS(std::string ident, chillAST_CudaSyncthreads *n, std::ostream &o) {}

void Dump::printS(std::string ident, chillAST_DeclRefExpr *n, std::ostream &o) {
  chillAST_VarDecl *vd = n->getVarDecl();
  if (vd)
    if (vd->isAParameter) o << "ParmVar "; else o << "Var ";
  o << n->declarationName << " ";
  chillAST_FunctionDecl *fd = n->getFunctionDecl();
  if (fd) dumpVector(this, ident, fd->getParameters(), o);
}

void Dump::printS(std::string ident, chillAST_FloatingLiteral *n, std::ostream &o) {
  if (n->precision == 1) o << "float ";
  else o << "double ";
  o << n->value;
}

void Dump::printS(std::string ident, chillAST_ForStmt *n, std::ostream &o) {
  print(ident, n->getInit(), o);
  print(ident, n->getCond(), o);
  print(ident, n->getInc(), o);
  print(ident, n->getBody(), o);
}

void Dump::printS(std::string ident, chillAST_Free *n, std::ostream &o) {}

void Dump::printS(std::string ident, chillAST_FunctionDecl *n, std::ostream &o) {
  if (n->filename) o << n->filename << " ";
  if (n->isFromSourceFile) o << "FromSourceFile" << " ";
  o << n->returnType << " " << n->functionName << " ";
  if (n->getBody()) print(ident, n->getBody(), o);
}

void Dump::printS(std::string ident, chillAST_IfStmt *n, std::ostream &o) {
  print(ident, n->getCond(), o);
  print(ident, n->getThen(), o);
  if (n->getElse())
    print(ident, n->getElse(), o);
}

void Dump::printS(std::string ident, chillAST_IntegerLiteral *n, std::ostream &o) {
  o << n->value << " ";
}

void Dump::printS(std::string ident, chillAST_ImplicitCastExpr *n, std::ostream &o) {
  print(ident, n->getSubExpr(), o);
}

void Dump::printS(std::string ident, chillAST_MacroDefinition *n, std::ostream &o) {
  o << n->macroName << " ";
  dumpVector(this, ident, n->getParameters(), o);
  print(ident, n->getBody(), o);
}

void Dump::printS(std::string ident, chillAST_Malloc *n, std::ostream &o) {
  print(ident, n->getSize(), o);
}

void Dump::printS(std::string ident, chillAST_MemberExpr *n, std::ostream &o) {
  print(ident, n->base, o);
  if (n->exptype == CHILLAST_MEMBER_EXP_ARROW) o << "-> ";
  else o << ". ";
  o << n->member << " ";
}

void Dump::printS(std::string ident, chillAST_NULL *n, std::ostream &o) {
  o << "(NULL) ";
}

void Dump::printS(std::string ident, chillAST_NoOp *n, std::ostream &o) {}

void Dump::printS(std::string ident, chillAST_ParenExpr *n, std::ostream &o) {
  print(ident, n->getSubExpr(), o);
}

void Dump::printS(std::string ident, chillAST_Preprocessing *n, std::ostream &o) {}

void Dump::printS(std::string ident, chillAST_RecordDecl *n, std::ostream &o) {
  // TODO access control
  o << n->getName() << " ";
  o << n->isAStruct() << " ";
  o << n->isAUnion() << " ";
}

void Dump::printS(std::string ident, chillAST_ReturnStmt *n, std::ostream &o) {
  if (n->getRetVal()) print(ident, n->getRetVal(), o);
}

void Dump::printS(std::string ident, chillAST_Sizeof *n, std::ostream &o) {
  o << n->thing << " ";
}

void Dump::printS(std::string ident, chillAST_SourceFile *n, std::ostream &o) {
  dumpVector(this, ident, n->getChildren(), o);
}

void Dump::printS(std::string ident, chillAST_TypedefDecl *n, std::ostream &o) {
  o << n->underlyingtype << " " << n->newtype << " " << n->arraypart << " ";
}

void Dump::printS(std::string ident, chillAST_TernaryOperator *n, std::ostream &o) {
  o << n->op << " ";
  print(ident, n->getCond(), o);
  print(ident, n->getLHS(), o);
  print(ident, n->getRHS(), o);
}

void Dump::printS(std::string ident, chillAST_UnaryOperator *n, std::ostream &o) {
  if (n->prefix) o << "prefix ";
  else o << "postfix ";
  print(ident, n->getSubExpr(), o);
}

void Dump::printS(std::string ident, chillAST_VarDecl *n, std::ostream &o) {
  o << "\"'" << n->vartype << "' '" << n->varname << "' '" << n->arraypart << "'\" dim " << n->numdimensions << " ";
}

