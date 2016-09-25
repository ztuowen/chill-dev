//
// Created by ztuowen on 9/24/16.
//

#include "printer/cfamily.h"

using namespace std;
using namespace chill::printer;

bool ifSemicolonFree(CHILLAST_NODE_TYPE t) {
  return t == CHILLAST_NODE_FUNCTIONDECL || t == CHILLAST_NODE_IFSTMT ||
         t == CHILLAST_NODE_FORSTMT || t == CHILLAST_NODE_MACRODEFINITION;
}

void dumpVector(GenericPrinter *p, string ident, chillAST_NodeList *n, ostream &o) {
  for (int i = 0; i < n->size(); ++i)
    p->print("", (*n)[i], o);
}

void dumpVector(GenericPrinter *p, string ident, chillAST_SymbolTable *n, ostream &o) {
  for (int i = 0; i < n->size(); ++i)
    p->print("", (*n)[i], o);
}

void dumpVector(GenericPrinter *p, string ident, chillAST_TypedefTable *n, ostream &o) {
  for (int i = 0; i < n->size(); ++i)
    p->print("", (*n)[i], o);
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
  print(ident,n->base,o);
  o<<"[";
  print(ident,n->index,o);
  o<<"]";
}

void Dump::printS(std::string ident, chillAST_BinaryOperator *n, std::ostream &o) {
  o << n->op << " ";
  if (n->lhs) print(ident, n->lhs, o);
  else o << "(NULL) ";
  if (n->rhs) print(ident, n->rhs, o);
  else o << "(NULL) ";
}

void Dump::printS(std::string ident, chillAST_CallExpr *n, std::ostream &o) {
  if (n->callee)
    print(ident, n->callee, o);
}

void Dump::printS(std::string ident, chillAST_CompoundStmt *n, std::ostream &o) {
  dumpVector(this, ident, n->getChildren(), o);
}

void Dump::printS(std::string ident, chillAST_CStyleAddressOf *n, std::ostream &o) {
  print(ident, n->subexpr, o);
}

void Dump::printS(std::string ident, chillAST_CStyleCastExpr *n, std::ostream &o) {
  o << n->towhat << " ";
  print(ident, n->subexpr, o);
}

void Dump::printS(std::string ident, chillAST_CudaFree *n, std::ostream &o) {
  o << n->variable->varname << " ";
}

void Dump::printS(std::string ident, chillAST_CudaKernelCall *n, std::ostream &o) {
  CHILL_ERROR("Not implemented");
}

void Dump::printS(std::string ident, chillAST_CudaMalloc *n, std::ostream &o) {
  print(ident, n->devPtr, o);
  print(ident, n->sizeinbytes, o);
}

void Dump::printS(std::string ident, chillAST_CudaMemcpy *n, std::ostream &o) {
  o << "cudaMemcpy(" << n->dest->varname << ", " << n->src->varname << ", ";
  print(ident, n->size, o);
  o << ", " << n->cudaMemcpyKind << ")";
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
  if (n->metacomment)
    o << "// " << n->metacomment << "\n";
  o << "for (";
  print(ident, n->getInit(), o);
  o << ";";
  print(ident, n->getCond(), o);
  o << ";";
  print(ident, n->getInc(), o);
  o << ")";
  if (n->getBody()->getType() == CHILLAST_NODE_COMPOUNDSTMT) {
    if (n->getBody()->getChildren()->size() < 2) o << "\n" << ident << identSpace;
    else o << " ";
    print(ident, n->getBody(), o);
  } else {
    CHILL_ERROR("Body of for loop not COMPOUNDSTMT\n");
    print(ident, n->getBody(), o);
  }
}

void Dump::printS(std::string ident, chillAST_Free *n, std::ostream &o) {}

void Dump::printS(std::string ident, chillAST_FunctionDecl *n, std::ostream &o) {
  if (n->filename) o << n->filename << " ";
  if (n->isFromSourceFile) o << "FromSourceFile" << " ";
  o << n->returnType << " " << n->functionName << " ";
  if (n->getBody()) print(ident, n->getBody(), o);
}

void Dump::printS(std::string ident, chillAST_IfStmt *n, std::ostream &o) {
  print(ident, n->cond, o);
  print(ident, n->thenpart, o);
  if (n->elsepart)
    print(ident, n->elsepart, o);
}

void Dump::printS(std::string ident, chillAST_IntegerLiteral *n, std::ostream &o) {
  o << n->value;
}

void Dump::printS(std::string ident, chillAST_ImplicitCastExpr *n, std::ostream &o) {
  print(ident, n->subexpr, o);
}

void Dump::printS(std::string ident, chillAST_MacroDefinition *n, std::ostream &o) {
  o << "#define" << n->macroName << " ";
  int np = n->getParameters()->size();
  if (np) {
    o << "(" << n->getParameters()->at(0)->varname;
    for (int i = 1; i < np; ++i)
      o << ", " << n->getParameters()->at(i)->varname;
    o<<")";
  }
  // TODO newline for multiline macro
  print(ident, n->getBody(), o);
}

void Dump::printS(std::string ident, chillAST_Malloc *n, std::ostream &o) {
  o << "malloc(";
  print(ident, n->sizeexpr, o);
  o << ")";
}

void Dump::printS(std::string ident, chillAST_MemberExpr *n, std::ostream &o) {
  int prec = getPrec(n);
  if (n->base) printPrec(ident, n->base, o, prec);
  else o << "(NULL)";
  if (n->exptype == CHILLAST_MEMBER_EXP_ARROW) o << "->";
  else o << ".";
  if (n->member) o << n->member;
  else o << "(NULL)";
}

void Dump::printS(std::string ident, chillAST_NULL *n, std::ostream &o) {
  o << "/* (NULL statement) */";
}

void Dump::printS(std::string ident, chillAST_NoOp *n, std::ostream &o) {}

void Dump::printS(std::string ident, chillAST_ParenExpr *n, std::ostream &o) {
  print(ident, n->subexpr, o);
}

void Dump::printS(std::string ident, chillAST_Preprocessing *n, std::ostream &o) {}

void Dump::printS(std::string ident, chillAST_RecordDecl *n, std::ostream &o) {
  // TODO access control
  o << n->getName() << " ";
  o << n->isAStruct() << " ";
  o << n->isAUnion() << " ";
}

void Dump::printS(std::string ident, chillAST_ReturnStmt *n, std::ostream &o) {
  if (n->returnvalue) print(ident, n->returnvalue, o);
}

void Dump::printS(std::string ident, chillAST_Sizeof *n, std::ostream &o) {
  o << "sizeof(" << n->thing << ")";
}

void Dump::printS(std::string ident, chillAST_SourceFile *n, std::ostream &o) {
  o << "// this source is derived from CHILL AST originally from file '"
    << n->filename << "' as parsed by frontend compiler " << n->frontend << "\n\n";
  int nchild = n->getChildren()->size();
  for (int i = 0; i < nchild; ++i) {
    if (n->getChild(i)->isFromSourceFile) {
      o << ident;
      print(indent, n->getChild(i), o);
      if (!isSemiColonFree(n->getChild(i)->getType())) o << ";\n";
      else o<<"\n";
    }
  }
}

void Dump::printS(std::string ident, chillAST_TypedefDecl *n, std::ostream &o) {
  if (n->isAStruct())
    o << "/* A typedef STRUCT */\n";
  o << ident << "typedef ";
  if (n->rd) print(ident, n->rd, o);
  else if (n->isAStruct()) {
    o << "/* no rd */ ";
    // the struct subparts
  } else
    o << "/* Not A STRUCT */ " << n->getUnderlyingType() << " " << n->newtype << " " << n->arraypart;
  o << n->newtype;
}

void Dump::printS(std::string ident, chillAST_TernaryOperator *n, std::ostream &o) {
  o << n->op << " ";
  print(ident, n->condition, o);
  print(ident, n->lhs, o);
  print(ident, n->rhs, o);
}

void Dump::printS(std::string ident, chillAST_UnaryOperator *n, std::ostream &o) {
  if (n->prefix) o << "prefix ";
  else o << "postfix ";
  print(ident, n->subexpr, o);
}

void Dump::printS(std::string ident, chillAST_VarDecl *n, std::ostream &o) {
  if (n->isDevice) o << "__device__ ";
  if (n->isShared) o << "__shared__ ";
  if (n->isRestrict) o << "__restrict__ ";

  if ((!n->isAParameter) && n->isAStruct() && n->vardef) {
    print(ident, n->vardef, o);
    o << " " << n->varname;
  }

  if (n->typedefinition && n->typedefinition->isAStruct())
    o << "struct ";
  o << n->vartype << " ";
  if (n->arraypointerpart)
    o << n->arraypointerpart;
  if (n->byreference)
    o << "&";
  o << n->varname;
  if (n->knownArraySizes)
    for (int i = 0; i < (n->numdimensions); ++i) o << "[" << n->arraysizes[i] << "]";

  if (n->init) {
    o << "= ";
    print(ident, n->init, o);
  }
}
