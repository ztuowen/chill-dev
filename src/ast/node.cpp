//
// Created by ztuowen on 9/26/16.
//

#include "chillAST.h"
#include "printer/dump.h"
#include "printer/cfamily.h"
#include <stack>

using namespace std;

int chillAST_Node::chill_scalar_counter = 0;
int chillAST_Node::chill_array_counter = 1;

const char *ChillAST_Node_Names[] = {
    "Unknown AST node type",
    "SourceFile",
    "TypedefDecl",
    "VarDecl",
    //  "ParmVarDecl",  not used any more
    "FunctionDecl",
    "RecordDecl",
    "MacroDefinition",
    "CompoundStmt",
    "ForStmt",
    "TernaryOperator",
    "BinaryOperator",
    "UnaryOperator",
    "ArraySubscriptExpr",
    "MemberExpr",
    "DeclRefExpr",
    "IntegerLiteral",
    "FloatingLiteral",
    "ImplicitCastExpr", // not sure we need this
    "ReturnStmt",
    "CallExpr",
    "DeclStmt",
    "ParenExpr",
    "CStyleCastExpr",
    "CStyleAddressOf",
    "IfStmt",
    "SizeOf",
    "Malloc",
    "Free",
    "NoOp",
// CUDA specific
    "CudaMalloc",
    "CudaFree",
    "CudaMemcpy",
    "CudaKernelCall",
    "CudaSyncthreads",
    "fake1",
    "fake2",
    "fake3"
};

const char* chillAST_Node::getTypeString() {
  return ChillAST_Node_Names[getType()];
};

void chillAST_Node::fixChildInfo() {}

void chillAST_Node::addChild(chillAST_Node *c) {
  c->parent = this;
  for (int i = 0; i < children.size(); i++) {
    if (c == children[i]) {
      CHILL_ERROR("addchild ALREADY THERE\n");
      return; // already there
    }
  }
  children.push_back(c);
};  // not usually useful

void chillAST_Node::addChildren(const chillAST_NodeList &c) {
  for (int i = 0; i < c.size(); ++i) {
    addChild(c[i]);
  }
}

void chillAST_Node::insertChild(int i, chillAST_Node *node) {
  //fprintf(stderr, "%s inserting child of type %s at location %d\n", getTypeString(), node->getTypeString(), i);
  node->parent = this;
  children.insert(children.begin() + i, node);
};

void chillAST_Node::removeChild(int i) {
  children.erase(children.begin() + i);
};

int chillAST_Node::findChild(chillAST_Node *c) {
  for (int i = 0; i < children.size(); i++) {
    if (children[i] == c) return i;
  }
  return -1;
}

void chillAST_Node::replaceChild(chillAST_Node *old, chillAST_Node *newchild) {
  for (int i = 0; i < getNumChildren(); i++) {
    if (getChild(i) == old) {
      setChild(i,newchild);
      return;
    }
  }
  CHILL_ERROR("Replace Child Falure, oldchild not a child\n");
  exit(-1);
};

void chillAST_Node::loseLoopWithLoopVar(char *var) {
  std::vector<chillAST_Node *> dupe = children; // simple enough?
  for (int i = 0; i < dupe.size(); i++) {  // recurse on all children
    dupe[i]->loseLoopWithLoopVar(var);
  }
}

void chillAST_Node::gatherLoopIndeces(std::vector<chillAST_VarDecl *> &indeces) {
  // you can quit when you get to certain nodes

  CHILL_DEBUG_PRINT("%s::gatherLoopIndeces()\n", getTypeString());

  if (isSourceFile() || isFunctionDecl()) return; // end of the line

  if (!parent) return; // should not happen, but be careful

  // for most nodes, this just recurses upwards
  parent->gatherLoopIndeces(indeces);
}

//! recursive walk parent links, looking for loops
chillAST_ForStmt *chillAST_Node::findContainingLoop() {
  CHILL_DEBUG_PRINT("%s::findContainingLoop()   ", getTypeString());
  if (!parent) return NULL;
  if (parent->isForStmt()) return (chillAST_ForStmt *) parent;
  return parent->findContainingLoop(); // recurse upwards
}

chillAST_Node *chillAST_Node::findContainingNonLoop() {
  fprintf(stderr, "%s::findContainingNonLoop()   ", getTypeString());
  if (!parent) return NULL;
  if (parent->isCompoundStmt() && parent->getParent()->isForStmt())
    return parent->getParent()->findContainingNonLoop(); // keep recursing
  if (parent->isForStmt()) return parent->findContainingNonLoop(); // keep recursing
  return (chillAST_Node *) parent; // return non-loop
}

void chillAST_Node::getTopLevelLoops(std::vector<chillAST_ForStmt *> &loops) {
  int n = children.size();
  for (int i = 0; i < n; i++) {
    if (children[i]->isForStmt()) {
      loops.push_back(((chillAST_ForStmt *) (children[i])));
    }
  }
}


void chillAST_Node::repairParentChild() {  // for nodes where all subnodes are children
  int n = children.size();
  for (int i = 0; i < n; i++) {
    if (children[i]->parent != this) {
      fprintf(stderr, "fixing child %s that didn't know its parent\n", children[i]->getTypeString());
      children[i]->parent = this;
    }
  }
}


void chillAST_Node::get_deep_loops(
    std::vector<chillAST_ForStmt *> &loops) { // this is probably broken - returns ALL loops under it
  int n = children.size();
  //fprintf(stderr, "get_deep_loops of a %s with %d children\n", getTypeString(), n);
  for (int i = 0; i < n; i++) {
    //fprintf(stderr, "child %d is a %s\n", i, children[i]->getTypeString());
    children[i]->get_deep_loops(loops);
  }
  //fprintf(stderr, "found %d deep loops\n", loops.size());
}


// generic for chillAST_Node with children
void chillAST_Node::find_deepest_loops(std::vector<chillAST_ForStmt *> &loops) { // returns DEEPEST nesting of loops
  std::vector<chillAST_ForStmt *> deepest; // deepest below here

  int n = children.size();
  //fprintf(stderr, "find_deepest_loops of a %s with %d children\n", getTypeString(), n);
  for (int i = 0; i < n; i++) {
    std::vector<chillAST_ForStmt *> subloops;  // loops below here among a child of mine

    //fprintf(stderr, "child %d is a %s\n", i, children[i]->getTypeString());
    children[i]->find_deepest_loops(subloops);

    if (subloops.size() > deepest.size()) {
      deepest = subloops;
    }
  }

  // append deepest we see at this level to loops
  for (int i = 0; i < deepest.size(); i++) {
    loops.push_back(deepest[i]);
  }

  //fprintf(stderr, "found %d deep loops\n", loops.size());

}

chillAST_SourceFile *chillAST_Node::getSourceFile() {
  if (isSourceFile()) return ((chillAST_SourceFile *) this);
  if (parent != NULL) return parent->getSourceFile();
  CHILL_ERROR("UHOH, getSourceFile() called on node %p %s that does not have a parent and is not a source file\n",
              this, this->getTypeString());
  this->print();
  exit(-1);
}

void chillAST_Node::addVariableToScope(chillAST_VarDecl *vd) {
  CHILL_DEBUG_PRINT("addVariableToScope( %s )\n", vd->varname);
  if (!symbolTable) return;
  symbolTable = addSymbolToTable(symbolTable, vd);
  vd->parent = this;
}

void chillAST_Node::addTypedefToScope(chillAST_TypedefDecl *tdd) {
  if (!typedefTable) return;
  typedefTable = addTypedefToTable(typedefTable, tdd);
  tdd->parent = this;
}

chillAST_TypedefDecl *chillAST_Node::findTypeDecleration(const char *t) {
  fprintf(stderr, " %s \n", t);
  chillAST_TypedefDecl *td = getTypeDeclaration(t);
  if (!td && parent) return parent->findTypeDecleration(t);
  return td; // should not happen
}

chillAST_VarDecl *chillAST_Node::findVariableDecleration(const char *t) {
  fprintf(stderr, " %s \n", t);
  chillAST_VarDecl *td = getVariableDeclaration(t);
  if (!td && parent) return parent->findVariableDecleration(t);
  return td; // should not happen
}

chillAST_VarDecl *chillAST_Node::getVariableDeclaration(const char *t) {
  chillAST_VarDecl *vd = symbolTableFindName(getSymbolTable(), t);
  if (!vd) vd = getParameter(t);
  return vd;
}

chillAST_TypedefDecl *chillAST_Node::getTypeDeclaration(const char *t) {
  return typedefTableFindName(getTypedefTable(), t);
}

void chillAST_Node::addParameter(chillAST_VarDecl *vd) {
  if (!parameters) {
    CHILL_ERROR("Calling addParameter on construct without parameters");
    exit(-1);
  }

  if (symbolTableFindName(getParameters(), vd->varname)) { // NOT recursive. just in FunctionDecl
    CHILL_ERROR("parameter %s already exists?\n", vd->varname);
    return;
  }

  CHILL_DEBUG_PRINT("setting %s isAParameter\n", vd->varname);
  getParameters()->push_back(vd);
  vd->isAParameter = true;
  vd->setParent(this); // this is a combined list!
}

chillAST_VarDecl *chillAST_Node::getParameter(const char *t) {
  return symbolTableFindName(getParameters(), t);
}

void chillAST_Node::dump(int indent, FILE *fp) {
  if (fp == stderr) {
    chill::printer::Dump d;
    d.printErr("", this);
    fprintf(stderr, "\n");
  } else {
    chill::printer::Dump d;
    d.printOut("", this);
    fprintf(stdout, "\n");
  }
}

void chillAST_Node::print(int indent, FILE *fp) {
  if (fp == stderr) {
    chill::printer::CFamily d;
    d.printErr("", this);
    fprintf(stderr, "\n");
  } else {
    chill::printer::CFamily d;
    d.printOut("", this);
    fprintf(stdout, "\n");
  }
}

chillAST_Node* chillAST_Node::constantFold(){
  for (int i = 0;i<getNumChildren();++i) {
    if (getChild(i))
    setChild(i,getChild(i)->constantFold());
  }
  return this;
};

void chillAST_Node::gatherVarDecls(vector<chillAST_VarDecl*> &decls) {
  for (int i = 0;i<getNumChildren();++i) {
    if (getChild(i))
    getChild(i)->gatherVarDecls(decls);
  }
}
void chillAST_Node::gatherArrayVarDecls(vector<chillAST_VarDecl*> &decls) {
  for (int i = 0;i<getNumChildren();++i) {
    if (getChild(i))
    getChild(i)->gatherArrayVarDecls(decls);
  }
}
void chillAST_Node::gatherArrayRefs(vector<chillAST_ArraySubscriptExpr*> &refs, bool writtento) {
  for (int i = 0;i<getNumChildren();++i) {
    if (getChild(i))
    getChild(i)->gatherArrayRefs(refs,writtento);
  }
}
void chillAST_Node::gatherScalarRefs(vector<chillAST_DeclRefExpr*> &refs, bool writtento) {
  for (int i = 0;i<getNumChildren();++i) {
    if (getChild(i))
    getChild(i)->gatherScalarRefs(refs,writtento);
  }
}
void chillAST_Node::gatherDeclRefExprs(vector<chillAST_DeclRefExpr*> &refs) {
  for (int i = 0;i<getNumChildren();++i) {
    if (getChild(i))
    getChild(i)->gatherDeclRefExprs(refs);
  }
}
void chillAST_Node::gatherVarUsage(vector<chillAST_VarDecl*> &decls) {
  for (int i = 0;i<getNumChildren();++i) {
    if (getChild(i))
    getChild(i)->gatherVarUsage(decls);
  }
}
void chillAST_Node::gatherStatements(vector<chillAST_Node*> &statements) {
  for (int i = 0;i<getNumChildren();++i) {
    if (getChild(i))
    getChild(i)->gatherStatements(statements);
  }
}
void chillAST_Node::replaceVarDecls(chillAST_VarDecl* olddecl, chillAST_VarDecl *newdecl) {
  for (int i = 0;i<getNumChildren();++i) {
    if (getChild(i))
    getChild(i)->replaceVarDecls(olddecl,newdecl);
  }
}

void chillAST_Node::gatherScalarVarDecls(vector<chillAST_VarDecl *> &decls) {
  for (int i = 0;i<getNumChildren();++i) {
    if (getChild(i))
      getChild(i)->gatherScalarVarDecls(decls);
  }
}

chillAST_Node* chillAST_Node::findContainingStmt() {
  chillAST_Node* p = getParent();
  if (p->isCompoundStmt()) return this;
  if (p->isForStmt()) return this;
  if (p->isIfStmt()) return this;
  return p->findContainingStmt();
}

chillAST_Node *chillAST_Node::getEnclosingStatement(int level) {  // TODO do for subclasses?
  if (isArraySubscriptExpr()) return parent->getEnclosingStatement(level + 1);

  if (level != 0) {
    if (isBinaryOperator() ||
        isUnaryOperator() ||
        isTernaryOperator() ||
        isReturnStmt() ||
        isCallExpr()
        )
      return this;
    // things that are not endpoints. recurse through parent
    if (isMemberExpr()) return parent->getEnclosingStatement(level + 1);
    if (isImplicitCastExpr()) return parent->getEnclosingStatement(level + 1);
    if (isSizeof()) return parent->getEnclosingStatement(level + 1);
    if (isCStyleCastExpr()) return parent->getEnclosingStatement(level + 1);
    return NULL;
  }
  CHILL_ERROR("level %d type %s, returning NULL\n", level, getTypeString());
  exit(-1);
  return NULL;
}

