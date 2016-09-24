

#ifndef _CHILLAST_NODE_H_
#define _CHILLAST_NODE_H_

#include "chillAST_def.hh"

//! generic node of the actual chillAST, a multiway tree node.
class chillAST_Node {
public:
  // TODO decide how to hide some data
  //! this Node's parent
  chillAST_Node *parent;
  //! this node's children the only entity that holds childs/subexpressions
  chillAST_NodeList children;
  //! Symbol Scoping
  chillAST_SymbolTable *symbolTable;
  //! typedef Scoping
  chillAST_TypedefTable *typedefTable;
  //! whether it is from a source file, when false it is from included files
  bool isFromSourceFile;
  //! the name of file this node from
  char *filename;
  //! for compiler internals, formerly a comment
  char *metacomment;
  std::vector<chillAST_Preprocessing *> preprocessinginfo;

  //! for manufactured scalars 
  static int chill_scalar_counter;
  //! for manufactured arrays
  static int chill_array_counter;
  //! for manufactured pointer
  static int chill_pointer_counter;

  //! Base constructor for all inherited class
  chillAST_Node() {
    parent = NULL;
    metacomment = NULL;
    isFromSourceFile = true;
    filename = NULL;
    symbolTable = NULL;
    typedefTable = NULL;
  }
  //! the type of this current node
  virtual CHILLAST_NODE_TYPE getType() {return CHILLAST_NODE_UNKNOWN;};
  //! the precedence of the current node, 0 being the highest
  virtual int getPrec() {return INT8_MAX;}

  bool isSourceFile() { return (getType() == CHILLAST_NODE_SOURCEFILE); };

  bool isTypeDefDecl() { return (getType() == CHILLAST_NODE_TYPEDEFDECL); };

  bool isVarDecl() { return (getType() == CHILLAST_NODE_VARDECL); };

  bool isFunctionDecl() { return (getType() == CHILLAST_NODE_FUNCTIONDECL); };

  bool isRecordDecl() { return (getType() == CHILLAST_NODE_RECORDDECL); };

  bool isMacroDefinition() { return (getType() == CHILLAST_NODE_MACRODEFINITION); };

  bool isCompoundStmt() { return (getType() == CHILLAST_NODE_COMPOUNDSTMT); };

  bool isLoop() { return (getType() == CHILLAST_NODE_LOOP); };    // AKA ForStmt
  bool isForStmt() { return (getType() == CHILLAST_NODE_LOOP); };    // AKA Loop
  bool isIfStmt() { return (getType() == CHILLAST_NODE_IFSTMT); };

  bool isTernaryOperator() { return (getType() == CHILLAST_NODE_TERNARYOPERATOR); };

  bool isBinaryOperator() { return (getType() == CHILLAST_NODE_BINARYOPERATOR); };

  bool isUnaryOperator() { return (getType() == CHILLAST_NODE_UNARYOPERATOR); };

  bool isArraySubscriptExpr() { return (getType() == CHILLAST_NODE_ARRAYSUBSCRIPTEXPR); };

  bool isMemberExpr() { return (getType() == CHILLAST_NODE_MEMBEREXPR); };

  bool isDeclRefExpr() { return (getType() == CHILLAST_NODE_DECLREFEXPR); };

  bool isIntegerLiteral() { return (getType() == CHILLAST_NODE_INTEGERLITERAL); };

  bool isFloatingLiteral() { return (getType() == CHILLAST_NODE_FLOATINGLITERAL); };

  bool isImplicitCastExpr() { return (getType() == CHILLAST_NODE_IMPLICITCASTEXPR); };

  bool isReturnStmt() { return (getType() == CHILLAST_NODE_RETURNSTMT); };

  bool isCallExpr() { return (getType() == CHILLAST_NODE_CALLEXPR); };

  bool isParenExpr() { return (getType() == CHILLAST_NODE_PARENEXPR); };

  bool isSizeof() { return (getType() == CHILLAST_NODE_SIZEOF); };

  bool isMalloc() { return (getType() == CHILLAST_NODE_MALLOC); };

  bool isFree() { return (getType() == CHILLAST_NODE_FREE); };

  bool isPreprocessing() { return (getType() == CHILLAST_NODE_PREPROCESSING); };

  bool isNoOp() { return (getType() == CHILLAST_NODE_NOOP); };

  bool isNull() { return (getType() == CHILLAST_NODE_NULL); };

  bool isCStyleCastExpr() { return (getType() == CHILLAST_NODE_CSTYLECASTEXPR); };

  bool isCStyleAddressOf() { return (getType() == CHILLAST_NODE_CSTYLEADDRESSOF); };

  bool isCudaMalloc() { return (getType() == CHILLAST_NODE_CUDAMALLOC); };

  bool isCudaFree() { return (getType() == CHILLAST_NODE_CUDAFREE); };

  bool isCudaMemcpy() { return (getType() == CHILLAST_NODE_CUDAMEMCPY); };

  bool isCudaKERNELCALL() { return (getType() == CHILLAST_NODE_CUDAKERNELCALL); };

  bool isCudaSYNCTHREADS() { return (getType() == CHILLAST_NODE_CUDASYNCTHREADS); };

  bool isDeclStmt() { return (getType() == CHILLAST_NODE_DECLSTMT); }; // doesn't exist

  bool isConstant() {
    return (getType() == CHILLAST_NODE_INTEGERLITERAL) || (getType() == CHILLAST_NODE_FLOATINGLITERAL);
  }


  virtual bool isAssignmentOp() { return false; };

  virtual bool isComparisonOp() { return false; };

  virtual bool isNotLeaf() { return false; };

  virtual bool isLeaf() { return true; };

  virtual bool isParmVarDecl() { return false; };

  virtual bool isPlusOp() { return false; };

  virtual bool isMinusOp() { return false; };

  virtual bool isPlusMinusOp() { return false; };

  virtual bool isMultDivOp() { return false; };

  virtual bool isAStruct() { return false; };

  virtual bool isAUnion() { return false; };

  virtual bool hasSymbolTable() { return false; }; // most nodes do NOT have a symbol table
  virtual bool hasTypedefTable() { return false; }; // most nodes do NOT have a typedef table
  virtual chillAST_SymbolTable *getSymbolTable() { return NULL; } // most nodes do NOT have a symbol table

  virtual chillAST_VarDecl *findVariableNamed(const char *name); // recursive

  chillAST_RecordDecl *findRecordDeclNamed(const char *name); // recursive

  // void addDecl( chillAST_VarDecl *vd); // recursive, adds to first  symbol table it can find 


  int getNumChildren() { return children.size(); };

  chillAST_NodeList& getChildren() { return children; };  // not usually useful
  void setChildren(chillAST_NodeList &c) { children = c; }; // does not set parent. probably should
  chillAST_Node *getChild(int which) { return children[which]; };

  void setChild(int which, chillAST_Node *n) {
    children[which] = n;
    children[which]->parent = this;
  };

  void setMetaComment(const char *c) { metacomment = strdup(c); };

  virtual void chillMergeChildInfo(chillAST_Node){
    // TODO  if (par) par->getSourceFile()->addFunc(this); for FuncDecl
    // TODO if (par) par->getSourceFile()->addMacro(this); For MacroDecl
    // TODO if (parent) parent->addVariableToSymbolTable(this); // should percolate up until something has a symbol table
  }

  virtual void addChild(chillAST_Node *c) {
    c->parent = this;
    // check to see if it's already there
    for (int i = 0; i < children.size(); i++) {
      if (c == children[i]) {
        CHILL_ERROR("addchild ALREADY THERE\n");
        return; // already there
      }
    }
    children.push_back(c);
  };  // not usually useful

  virtual void addChildren(const chillAST_NodeList &c){
    for (int i =0;i<c.size();++i){
      addChild(c[i]);
    }
  }

  virtual void insertChild(int i, chillAST_Node *node) {
    //fprintf(stderr, "%s inserting child of type %s at location %d\n", getTypeString(), node->getTypeString(), i); 
    node->parent = this;
    children.insert(children.begin() + i, node);
  };

  virtual void removeChild(int i) {
    children.erase(children.begin() + i);
  };

  int findChild(chillAST_Node *c) {
    for (int i = 0; i < children.size(); i++) {
      if (children[i] == c) return i;
    }
    return -1;
  }

  virtual void replaceChild(chillAST_Node *old, chillAST_Node *newchild) {
    CHILL_DEBUG_PRINT("(%s) forgot to implement replaceChild() ... using generic\n", getTypeString());
    CHILL_DEBUG_PRINT("%d children\n", children.size());
    for (int i = 0; i < children.size(); i++) {
      if (children[i] == old) {
        children[i] = newchild;
        newchild->setParent(this);
        return;
      }
    }
    CHILL_ERROR("%s %p generic replaceChild called with oldchild that was not a child\n",
            getTypeString(), this);
    CHILL_DEBUG_BEGIN
      fprintf(stderr, "printing\n");
      print();
      fprintf(stderr, "\nchild: ");
      if (!old) fprintf(stderr, "oldchild NULL!\n");
      old->print();
      fprintf(stderr, "\nnew: ");
      newchild->print();
      fprintf(stderr, "\n");
    CHILL_DEBUG_END
    exit(-1);
  };

  //! Spread the loop across a bunch of cores that will each calculate its own loop variable.
  /*!
   * If a loop has this loop variable, replace the loop with the loop body, calculate their own loop variable.
   *
   * things that can not have loops as substatements can have a null version of this method
   * things that have more complicated sets of "children" will have specialized versions
   *
   * this is the generic version of the method. It just recurses among its children.
   * ForStmt is the only one that can actually remove itself. When it does, it will
   * potentially change the children vector, which is not the simple array it might appear.
   * so you have to make a copy of the vector to traverse
   *
   * @param var
   */
  virtual void loseLoopWithLoopVar(char *var) {
    std::vector<chillAST_Node *> dupe = children; // simple enough?
    for (int i = 0; i < dupe.size(); i++) {  // recurse on all children
      dupe[i]->loseLoopWithLoopVar(var);
    }
  }

  virtual int evalAsInt() {
    CHILL_ERROR("(%s) can't be evaluated as an integer??\n", getTypeString());
    print();
    exit(-1);
  }

  virtual const char *getUnderlyingType() {
    CHILL_ERROR("(%s) forgot to implement getUnderlyingType()\n", getTypeString());
    dump();
    print();
    exit(-1);
  };

  virtual chillAST_VarDecl *getUnderlyingVarDecl() {
    CHILL_ERROR("(%s) forgot to implement getUnderlyingVarDecl()\n", getTypeString());
    dump();
    print();
    exit(-1);
  };


  //! find the SINGLE constant or data reference at this node or below
  virtual chillAST_Node *findref() {
    CHILL_ERROR("(%s) forgot to implement findref()\n", getTypeString());
    dump();
    print();
    exit(-1);
  };

  virtual void gatherArrayRefs(std::vector<chillAST_ArraySubscriptExpr *> &refs, bool writtento) {
    CHILL_ERROR("(%s) forgot to implement gatherArrayRefs()\n", getTypeString());
    dump();
    print();
    exit(-1);
  };

  // TODO we MIGHT want the VarDecl // NOTHING IMPLEMENTS THIS? ??? 
  virtual void gatherScalarRefs(std::vector<chillAST_DeclRefExpr *> &refs, bool writtento) {
    CHILL_ERROR("(%s) forgot to implement gatherScalarRefs()\n", getTypeString());
    dump();
    print();
    exit(-1);
  };

  //! recursive walk parent links, looking for loops, and grabbing the declRefExpr in the loop init and cond.
  virtual void gatherLoopIndeces(
      std::vector<chillAST_VarDecl *> &indeces) {
    // you can quit when you get to certain nodes

    CHILL_DEBUG_PRINT("%s::gatherLoopIndeces()\n", getTypeString());

    if (isSourceFile() || isFunctionDecl()) return; // end of the line

    if (!parent) return; // should not happen, but be careful

    // for most nodes, this just recurses upwards
    parent->gatherLoopIndeces(indeces);
  }

  //! recursive walk parent links, looking for loops
  chillAST_ForStmt *findContainingLoop() {
    CHILL_DEBUG_PRINT("%s::findContainingLoop()   ", getTypeString());
    if (!parent) return NULL;
    if (parent->isForStmt()) return (chillAST_ForStmt *) parent;
    return parent->findContainingLoop(); // recurse upwards
  }

  //! recursive walk parent links, avoiding loops
  chillAST_Node *findContainingNonLoop() {
    fprintf(stderr, "%s::findContainingNonLoop()   ", getTypeString());
    if (!parent) return NULL;
    if (parent->isCompoundStmt() && parent->getParent()->isForStmt())
      return parent->getParent()->findContainingNonLoop(); // keep recursing
    if (parent->isForStmt()) return parent->findContainingNonLoop(); // keep recursing
    return (chillAST_Node *) parent; // return non-loop
  }

  // TODO gather loop init and cond (and if cond) like gatherloopindeces

  virtual void gatherDeclRefExprs(std::vector<chillAST_DeclRefExpr *> &refs) {  // both scalar and arrays
    fprintf(stderr, "(%s) forgot to implement gatherDeclRefExpr()\n", getTypeString());
  };


  virtual void gatherVarUsage(std::vector<chillAST_VarDecl *> &decls) {
    fprintf(stderr, "(%s) forgot to implement gatherVarUsage()\n", getTypeString());
  };

  virtual void gatherVarLHSUsage(std::vector<chillAST_VarDecl *> &decls) {
    fprintf(stderr, "(%s) forgot to implement gatherVarLHSUsage()\n", getTypeString());
  };

  //! ACTUAL Declaration
  virtual void gatherVarDecls(std::vector<chillAST_VarDecl *> &decls) {
    fprintf(stderr, "(%s) forgot to implement gatherVarDecls()\n", getTypeString());
  };


  virtual void
  gatherVarDeclsMore(std::vector<chillAST_VarDecl *> &decls) {  // even if the decl itself is not in the ast.
    fprintf(stderr, "(%s) forgot to implement gatherVarDeclsMore()\n", getTypeString());
  };

  virtual void gatherScalarVarDecls(std::vector<chillAST_VarDecl *> &decls) {  // ACTUAL Declaration
    fprintf(stderr, "(%s) forgot to implement gatherScalarVarDecls()\n", getTypeString());
  };

  virtual void gatherArrayVarDecls(std::vector<chillAST_VarDecl *> &decls) {  // ACTUAL Declaration
    fprintf(stderr, "(%s) forgot to implement gatherArrayVarDecls()\n", getTypeString());
  };

  virtual chillAST_VarDecl *findArrayDecl(const char *name) { // scoping TODO
    if (!hasSymbolTable()) return parent->findArrayDecl(name); // most things
    else
      fprintf(stderr, "(%s) forgot to implement gatherArrayVarDecls()\n", getTypeString());
  }


  virtual void replaceVarDecls(chillAST_VarDecl *olddecl, chillAST_VarDecl *newdecl) {
    fprintf(stderr, "(%s) forgot to implement replaceVarDecls()\n", getTypeString());
  };

  //! this just looks for ForStmts with preferred index metacomment attached
  virtual bool findLoopIndexesToReplace(chillAST_SymbolTable *symtab, bool forcesync = false) {
    fprintf(stderr, "(%s) forgot to implement findLoopIndexesToReplace()\n", getTypeString());
    return false;
  }


  virtual chillAST_Node *constantFold() {  // hacky. TODO. make nice
    CHILL_ERROR("(%s) forgot to implement constantFold()\n", getTypeString());
    exit(-1);;
  };

  virtual chillAST_Node *clone() {   // makes a deep COPY (?)
    CHILL_ERROR("(%s) forgot to implement clone()\n", getTypeString());
    exit(-1);;
  };

  //! Print AST
  virtual void dump(int indent = 0, FILE *fp = stderr) {
    fflush(fp);
    CHILL_ERROR("(%s) forgot to implement dump()\n", getTypeString());
  };// print ast

  // TODO We might want to print the code a bit differently, This can be only a generic dump
  //! Print CODE
  virtual void print(int indent = 0, FILE *fp = stderr) {
    fflush(fp);
    fprintf(fp, "\n");
    chillindent(indent, fp);
    fprintf(fp, "(%s) forgot to implement print()\n", getTypeString());
  };

  virtual void printName(int indent = 0, FILE *fp = stderr) {
    fflush(fp);
    fprintf(fp, "\n");
    chillindent(indent, fp);
    fprintf(fp, "(%s) forgot to implement printName()\n", getTypeString());
  };// print CODE 

  //! The AST's print version
  virtual char *stringRep(int indent = 0) {
    fflush(stdout);
    // TODO chillindent(indent, fp);
    CHILL_ERROR("(%s) forgot to implement stringRep()\n", getTypeString());
    exit(-1);
  }


  virtual void printonly(int indent = 0, FILE *fp = stderr) { print(indent, fp); };

  virtual void get_top_level_loops(std::vector<chillAST_ForStmt *> &loops) {
    int n = children.size();
    //fprintf(stderr, "get_top_level_loops of a %s with %d children\n", getTypeString(), n); 
    for (int i = 0; i < n; i++) {
      //fprintf(stderr, "child %d is a %s\n", i, children[i]->getTypeString()); 
      if (children[i]->isForStmt()) {
        loops.push_back(((chillAST_ForStmt *) (children[i])));
      }
    }
    //fprintf(stderr, "found %d top level loops\n", loops.size()); 
  }


  virtual void repairParentChild() {  // for nodes where all subnodes are children
    int n = children.size();
    for (int i = 0; i < n; i++) {
      if (children[i]->parent != this) {
        fprintf(stderr, "fixing child %s that didn't know its parent\n", children[i]->getTypeString());
        children[i]->parent = this;
      }
    }
  }


  virtual void
  get_deep_loops(std::vector<chillAST_ForStmt *> &loops) { // this is probably broken - returns ALL loops under it
    int n = children.size();
    //fprintf(stderr, "get_deep_loops of a %s with %d children\n", getTypeString(), n); 
    for (int i = 0; i < n; i++) {
      //fprintf(stderr, "child %d is a %s\n", i, children[i]->getTypeString()); 
      children[i]->get_deep_loops(loops);
    }
    //fprintf(stderr, "found %d deep loops\n", loops.size()); 
  }


  // generic for chillAST_Node with children
  virtual void find_deepest_loops(std::vector<chillAST_ForStmt *> &loops) { // returns DEEPEST nesting of loops
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


  const char *getTypeString() { return ChillAST_Node_Names[getType()]; };

  void setParent(chillAST_Node *p) { parent = p; };

  chillAST_Node *getParent() { return parent; };

  //! This will be ideally replaced by call at to the top level
  chillAST_SourceFile *getSourceFile() {
    if (isSourceFile()) return ((chillAST_SourceFile *) this);
    if (parent != NULL) return parent->getSourceFile();
    fprintf(stderr, "UHOH, getSourceFile() called on node %p %s that does not have a parent and is not a source file\n",
            this, this->getTypeString());
    this->print();
    printf("\n\n");
    fflush(stdout);
    exit(-1);
  }

  virtual chillAST_Node *findDatatype(char *t) {
    fprintf(stderr, "%s looking for datatype %s\n", getTypeString(), t);
    if (parent != NULL) return parent->findDatatype(t); // most nodes do this
    return NULL;
  }


  virtual chillAST_SymbolTable *addVariableToSymbolTable(chillAST_VarDecl *vd) {
    if (!parent) {
      fprintf(stderr, "%s with no parent addVariableToSymbolTable()\n", getTypeString());
      exit(-1);
    }
    //fprintf(stderr, "%s::addVariableToSymbolTable() (default) headed up\n",  getTypeString()); 
    return parent->addVariableToSymbolTable(vd); // default, defer to parent
  }

  virtual void addTypedefToTypedefTable(chillAST_TypedefDecl *tdd) {
    parent->addTypedefToTypedefTable(tdd); // default, defer to parent
  }

  void walk_parents() {
    fprintf(stderr, "wp: (%s)  ", getTypeString());
    print();
    printf("\n");
    fflush(stdout);
    if (isSourceFile()) {
      fprintf(stderr, "(top sourcefile)\n\n");
      return;
    }

    if (parent) parent->walk_parents();
    else fprintf(stderr, "UHOH, %s has no parent??\n", getTypeString());
    return;
  }

  virtual chillAST_Node *getEnclosingStatement(int level = 0);

  virtual chillAST_VarDecl *multibase() {
    fprintf(stderr, "(%s) forgot to implement multibase()\n", getTypeString());
    exit(-1);
  }

  virtual chillAST_Node *multibase2() {
    fprintf(stderr, "(%s) forgot to implement multibase2()\n", getTypeString());
    exit(-1);
  }


  virtual void gatherStatements(std::vector<chillAST_Node *> &statements) {
    fprintf(stderr, "(%s) forgot to implement gatherStatements()\n", getTypeString());
    dump();
    fflush(stdout);
    print();
    fprintf(stderr, "\n\n");
  }


  virtual bool isSameAs(chillAST_Node *other) {  // for tree comparison
    fprintf(stderr, "(%s) forgot to implement isSameAs()\n", getTypeString());
    dump();
    fflush(stdout);
    print();
    fprintf(stderr, "\n\n");
  }

  void printPreprocBEFORE(int indent, FILE *fp);

  void printPreprocAFTER(int indent, FILE *fp);


};


#endif

