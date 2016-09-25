//
// Created by ztuowen on 9/24/16.
//

#include "printer/cfamily.h"
#include <iomanip>


using namespace std;
using namespace chill::printer;

bool opInSet(const char *set, char *op) {
  string tmp = op;
  tmp = " " + tmp + " ";
  return strstr(set, tmp.c_str()) != NULL;
}

bool ifSemicolonFree(CHILLAST_NODE_TYPE t) {
  return t == CHILLAST_NODE_FUNCTIONDECL || t == CHILLAST_NODE_IFSTMT ||
         t == CHILLAST_NODE_FORSTMT || t == CHILLAST_NODE_MACRODEFINITION;
}

void CFamily::printS(std::string ident, chillAST_ArraySubscriptExpr *n, std::ostream &o) {
  print(ident, n->base, o);
  o << "[";
  print(ident, n->index, o);
  o << "]";
}

//! I'm just a bit lazy to write ifs ...
const char *binaryPrec[] = {
    " :: ",
    " . -> ",
    "",
    " .* ->* ",
    " * / % ",
    " + - ",
    " << >> ",
    " < <= > >=",
    " == != ",
    " & ",
    " ^ ",
    " | ",
    " && ",
    " || ",
    " = += -= *= /= %= <<= >>= &= ^= |= ",
    " , "
};

int CFamily::getPrecS(chillAST_BinaryOperator *n) {
  for (int i = 0; i < 16; ++i)
    if (opInSet(binaryPrec[i], n->op)) return defGetPrecS() + i + 1;
  CHILL_ERROR("Unrecognized binary operator: %s\n", n->op);
  return defGetPrecS();
}

void CFamily::printS(std::string ident, chillAST_BinaryOperator *n, std::ostream &o) {
  int prec = getPrec(n);
  if (n->lhs) printPrec(ident, n->lhs, o, prec);
  else o << "(NULL)";
  o << " " << n->op << " ";
  if (n->rhs) printPrec(ident, n->rhs, o, prec);
  else o << "(NULL)";
}

int CFamily::getPrecS(chillAST_CallExpr *n) {
  return defGetPrecS() + 2;
}

void CFamily::printS(std::string ident, chillAST_CallExpr *n, std::ostream &o) {
  chillAST_FunctionDecl *FD = NULL;
  chillAST_MacroDefinition *MD = NULL;
  if (n->callee->isDeclRefExpr()) {
    chillAST_DeclRefExpr *DRE = (chillAST_DeclRefExpr *) (n->callee);
    if (!(DRE->decl)) {
      o << DRE->declarationName;
      return;
    }
    if (DRE->decl->isFunctionDecl()) FD = (chillAST_FunctionDecl *) (DRE->decl);
    else
      CHILL_ERROR("Function DRE of type %s\n", DRE->decl->getTypeString());
  } else if (n->callee->isFunctionDecl())
    FD = (chillAST_FunctionDecl *) n->callee;
  else if (n->callee->isMacroDefinition())
    MD = (chillAST_MacroDefinition *) n->callee;
  if (FD) {
    o << FD->functionName;
    if (n->grid && n->block)
      o << "<<<" << n->grid->varname << "," << n->block->varname << ">>>";
    o << "(";
  }
  if (MD && n->args.size())
    o << "(";
  for (int i = 0; i < n->args.size(); ++i) {
    if (i != 0) o << ", ";
    print(ident, n->args[i], o);
  }
  if (FD || n->args.size())
    o << ")";
}

void CFamily::printS(std::string ident, chillAST_CompoundStmt *n, std::ostream &o) {
  chillAST_NodeList *c = n->getChildren();
  string nid = ident + identSpace;
  if (c->size() > 1) o << "{";
  for (int i = 0; i < c->size(); ++i) {
    o << "\n" << nid;
    print(nid, c->at(i), o);
    if (!ifSemicolonFree(c->at(i)->getType())) o << ";";
  }
  if (c->size() > 1) o << "\n" << ident << "}";
}

int CFamily::getPrecS(chillAST_CStyleAddressOf *n) {
  return defGetPrecS() + 3;
}

void CFamily::printS(std::string ident, chillAST_CStyleAddressOf *n, std::ostream &o) {
  int prec = getPrec(n);
  printPrec(ident, n->subexpr, o, prec);
}

int CFamily::getPrecS(chillAST_CStyleCastExpr *n) {
  return defGetPrecS() + 3;
}

void CFamily::printS(std::string ident, chillAST_CStyleCastExpr *n, std::ostream &o) {
  o << "(" << n->towhat << ")";
  printPrec(ident, n->subexpr, o, getPrec(n));
}

void CFamily::printS(std::string ident, chillAST_CudaFree *n, std::ostream &o) {
  o << "cudaFree(" << n->variable->varname << ")";
}

void CFamily::printS(std::string ident, chillAST_CudaKernelCall *n, std::ostream &o) {
  CHILL_ERROR("Not implemented");
}

void CFamily::printS(std::string ident, chillAST_CudaMalloc *n, std::ostream &o) {
  o << "cudaMalloc(";
  print(ident, n->devPtr, o);
  o << ", ";
  print(ident, n->sizeinbytes, o);
  o << ")";
}

void CFamily::printS(std::string ident, chillAST_CudaMemcpy *n, std::ostream &o) {
  o << "cudaMemcpy(" << n->dest->varname << ", " << n->src->varname << ", ";
  print(ident, n->size, o);
  o << ", " << n->cudaMemcpyKind << ")";
}

void CFamily::printS(std::string ident, chillAST_CudaSyncthreads *n, std::ostream &o) {
  o << "__syncthreads()";
}

void CFamily::printS(std::string ident, chillAST_DeclRefExpr *n, std::ostream &o) {
  o << n->declarationName;
}

void CFamily::printS(std::string ident, chillAST_FloatingLiteral *n, std::ostream &o) {
  // Althedigits contaminates the result
  o << showpoint << n->value;
  if (n->getPrecision() == 1)
    o << "f";
}

void CFamily::printS(std::string ident, chillAST_ForStmt *n, std::ostream &o) {
  if (n->metacomment)
    o << "// " << n->metacomment << "\n";
  o << "for (";
  print(ident, n->getInit(), o);
  o << ";";
  print(ident, n->getCond(), o);
  o << ";";
  print(ident, n->getInc(), o);
  o << ") ";
  if (n->getBody()->isCompoundStmt()) {
    print(ident, n->getBody(), o);
  } else {
    CHILL_ERROR("Body of for loop not COMPOUNDSTMT\n");
    print(ident, n->getBody(), o);
  }
}

void CFamily::printS(std::string ident, chillAST_Free *n, std::ostream &o) {}

void CFamily::printS(std::string ident, chillAST_FunctionDecl *n, std::ostream &o) {
  if (n->isExtern()) o << "extern ";
  if (n->getFunctionType() == CHILLAST_FUNCTION_GPU) o << "__global__ ";
  o << n->returnType << " " << n->functionName << "(";

  chillAST_SymbolTable *pars = n->getParameters();
  for (int i = 0; i < pars->size(); ++i) {
    if (i != 0)
      o << ", ";
    print(ident, pars->at(i), o);
  }
  o << ")";
  if (!(n->isExtern() || n->isForward())) {
    o << " ";
    if (n->getBody())
      print(ident, n->getBody(), o);
    else {
      CHILL_ERROR("Non-extern or forward function decl doesn't have a body");
      o << "{}";
    }
  } else {
    o << ";";
  }
}

void CFamily::printS(std::string ident, chillAST_IfStmt *n, std::ostream &o) {
  o << "if (";
  print(ident, n->getCond(), o);
  o << ") ";
  if (!n->getThen()) {
    CHILL_ERROR("If statement is without then part!");
    exit(-1);
  }
  print(ident, n->getThen(), o);
  if (!(n->getThen()->isCompoundStmt()))
    CHILL_ERROR("Then part is not a CompoundStmt!\n");
  if (n->getElse()) {
    o << "else ";
    print(ident, n->elsepart, o);
  }
}

void CFamily::printS(std::string ident, chillAST_IntegerLiteral *n, std::ostream &o) {
  o << n->value;
}

void CFamily::printS(std::string ident, chillAST_ImplicitCastExpr *n, std::ostream &o) {
  print(ident, n->subexpr, o);
}

void CFamily::printS(std::string ident, chillAST_MacroDefinition *n, std::ostream &o) {
  o << "#define" << n->macroName << " ";
  int np = n->getParameters()->size();
  if (np) {
    o << "(" << n->getParameters()->at(0)->varname;
    for (int i = 1; i < np; ++i)
      o << ", " << n->getParameters()->at(i)->varname;
    o << ")";
  }
  // TODO newline for multiline macro
  print(ident, n->getBody(), o);
}

void CFamily::printS(std::string ident, chillAST_Malloc *n, std::ostream &o) {
  o << "malloc(";
  print(ident, n->sizeexpr, o);
  o << ")";
}

void CFamily::printS(std::string ident, chillAST_MemberExpr *n, std::ostream &o) {
  int prec = getPrec(n);
  if (n->base) printPrec(ident, n->base, o, prec);
  else o << "(NULL)";
  if (n->exptype == CHILLAST_MEMBER_EXP_ARROW) o << "->";
  else o << ".";
  if (n->member) o << n->member;
  else o << "(NULL)";
}

void CFamily::printS(std::string ident, chillAST_NULL *n, std::ostream &o) {
  o << "/* (NULL statement) */";
}

void CFamily::printS(std::string ident, chillAST_NoOp *n, std::ostream &o) {}

void CFamily::printS(std::string ident, chillAST_ParenExpr *n, std::ostream &o) {
  o << "(";
  print(ident, n->subexpr, o);
  o << ")";
}

void CFamily::printS(std::string ident, chillAST_Preprocessing *n, std::ostream &o) {
  CHILL_ERROR("Not implemented\n");
}

void CFamily::printS(std::string ident, chillAST_RecordDecl *n, std::ostream &o) {
  if (n->isUnnamed) return;
  if (n->isAStruct()) {
    string nid = ident + identSpace;
    o << "struct " << n->getName() << " {";
    chillAST_SymbolTable *sp = n->getSubparts();
    for (int i = 0; i < sp->size(); ++i) {
      o << "\n" << nid;
      print(nid, sp->at(i), o);
      o << ";";
    }
    o << "\n" << ident << "}";
  } else {
    CHILL_ERROR("Encountered Unkown record type");
    exit(-1);
  }
}

void CFamily::printS(std::string ident, chillAST_ReturnStmt *n, std::ostream &o) {
  o << "return";
  if (n->returnvalue) {
    o << " ";
    print(ident, n->returnvalue, o);
  }
}

void CFamily::printS(std::string ident, chillAST_Sizeof *n, std::ostream &o) {
  o << "sizeof(" << n->thing << ")";
}

void CFamily::printS(std::string ident, chillAST_SourceFile *n, std::ostream &o) {
  o << "// this source is derived from CHILL AST originally from file '"
    << n->filename << "' as parsed by frontend compiler " << n->frontend << "\n\n";
  int nchild = n->getChildren()->size();
  for (int i = 0; i < nchild; ++i) {
    if (n->getChild(i)->isFromSourceFile) {
      o << ident;
      print(ident, n->getChild(i), o);
      if (!ifSemicolonFree(n->getChild(i)->getType())) o << ";\n";
      else o << "\n";
    }
  }
}

void CFamily::printS(std::string ident, chillAST_TypedefDecl *n, std::ostream &o) {
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

int CFamily::getPrecS(chillAST_TernaryOperator *n) {
  return defGetPrecS() + 15;
}

void CFamily::printS(std::string ident, chillAST_TernaryOperator *n, std::ostream &o) {
  int prec = getPrec(n);
  printPrec(ident, n->condition, o, prec);
  o << "" << n->op << "";
  printPrec(ident, n->lhs, o, prec);
  o << ":";
  printPrec(ident, n->rhs, o, prec);
}

const char *unaryPrec[] = {
    "",
    " -- ++ ",
    " -- ++ + - ! ~ * & ",
};

int CFamily::getPrecS(chillAST_UnaryOperator *n) {
  if (n->prefix) {
    for (int i = 2; i >= 0; --i)
      if (opInSet(unaryPrec[i], n->op)) return defGetPrecS() + i + 1;
  } else
    for (int i = 1; i < 3; ++i)
      if (opInSet(unaryPrec[i], n->op)) return defGetPrecS() + i + 1;
  return defGetPrecS();
}

void CFamily::printS(std::string ident, chillAST_UnaryOperator *n, std::ostream &o) {
  int prec = getPrec(n);
  if (n->prefix) o << n->op;
  printPrec(ident, n->subexpr, o, prec);
  if (!n->prefix) o << n->op;
}

void CFamily::printS(std::string ident, chillAST_VarDecl *n, std::ostream &o) {
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
