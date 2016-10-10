

#ifndef _CHILLAST_NODE_H_
#define _CHILLAST_NODE_H_

#include "chillAST_def.hh"
#include <stack>
#include <map>

//! generic node of the actual chillAST, a multiway tree node.
class chillAST_Node {
protected:
  /*!
   * Add typedef to containing scope
   * @param tdt the containing scope
   */
  void gatherTypedef(chillAST_TypedefTable* tdt);
  /*!
   * Add symboldefinition to containing scope
   * @param st the containing scope
   */
  void gatherSymbol(chillAST_SymbolTable* st);
  /*!
   * Add struct definition to containing scope
   * @param rt the containing scope
   */
  void gatherRecord(chillAST_RecordTable *rt);
  /*!
   * Add function declaration & definition to the source file
   * @param s
   */
  void gatherFunctionDecl(std::vector<chillAST_FunctionDecl *> *s);
  /*!
   * Add macro definition to the source file
   * @param s
   */
  void gatherMacroDecl(std::vector<chillAST_MacroDefinition *> *s);
  /*!
   * Fix the pointer reference in a source file
   * @param nameMap a mapping from string to the corresponding declarations, stack is used to mimic scoping
   */
  void fixReference(std::map<std::string, std::stack<chillAST_Node*> > nameMap);
  //! this Node's parent
  chillAST_Node *parent;
  //! this node's children the only entity that holds childs/subexpressions
  chillAST_NodeList children;
  //! The parameters that this nodes accept, which every elements is in the scope, but they are not defined in children
  chillAST_SymbolTable *parameters;
  //! Symbol Scoping
  chillAST_SymbolTable *symbolTable;
  //! typedef Scoping
  chillAST_TypedefTable *typedefTable;
  //! recordDecl scoping
  std::vector<chillAST_RecordDecl*> *recordTable;
public:
  // TODO decide how to hide some data
  //! whether it is from a source file, when false it is from included files
  bool isFromSourceFile;
  //! the name of file this node from
  char *filename;
  //! for compiler internals, formerly a comment
  char *metacomment;

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
    parameters = NULL;
  }

  //! the type of this current node
  virtual CHILLAST_NODE_TYPE getType() { return CHILLAST_NODE_UNKNOWN; };

  //! Get the human readable type name
  const char *getTypeString();

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

  //! Get the symbol table in the contained scope
  virtual chillAST_SymbolTable *getSymbolTable() { return symbolTable; }

  //! Get the typedef table in the contained scope
  virtual chillAST_TypedefTable *getTypedefTable() { return typedefTable; }

  /*!
   * @brief Add a variable declaration to the contained scope
   *
   * This will not automatically link the variable declaration as a child
   *
   * @param vd
   */
  virtual void addVariableToScope(chillAST_VarDecl *vd);

  /*!
   * @brief Add a typedef to the contained scope
   *
   * This will not automatically link the typedef as a child
   *
   * @param tdd
   */
  virtual void addTypedefToScope(chillAST_TypedefDecl *tdd);

  //! Non recursive version that tries to find the declaration in this node
  chillAST_VarDecl *getVariableDeclaration(const char *vn);

  //! Non recursive version that tries to find the declaration in this node
  chillAST_TypedefDecl *getTypeDeclaration(const char *tn);

  //! Recursive version that will go to parent node if not finding in this
  chillAST_VarDecl *findVariableDecleration(const char *t);

  //! Recursive version that will go the parent node if not finding in this
  chillAST_TypedefDecl *findTypeDecleration(const char *t);

  /*!
   * \brief Get the number of subexpressions
   *
   * This will get multiplexed for different operation. Subexpression appear in the sourcefile order.
   * @return The number of subexpressions
   */
  virtual int getNumChildren() { return children.size(); };

  //! Deprecating, return the children as a list not eligible for multiplexing
  chillAST_NodeList *getChildren() { return &children; };
  /*!
   * \brief Get the which-th child
   *
   * This will get multiplexed in the same manner as \sa getNumChildren
   * @param which
   * @return The which-th node
   */
  chillAST_Node *getChild(int which) { return children[which]; };

  void setChild(int which, chillAST_Node *n) {
    children[which] = n;
    if (n) children[which]->parent = this;
  };

  void setMetaComment(const char *c) { metacomment = strdup(c); };

  virtual void addChild(chillAST_Node *c);

  virtual void addChildren(const chillAST_NodeList &c);

  virtual void insertChild(int i, chillAST_Node *node);

  virtual void removeChild(int i);

  int findChild(chillAST_Node *c);

  virtual void replaceChild(chillAST_Node *old, chillAST_Node *newchild);

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
  virtual void loseLoopWithLoopVar(char *var);

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

  virtual void gatherArrayRefs(std::vector<chillAST_ArraySubscriptExpr *> &refs, bool writtento);

  // TODO we MIGHT want the VarDecl // NOTHING IMPLEMENTS THIS? ??? 
  virtual void gatherScalarRefs(std::vector<chillAST_DeclRefExpr *> &refs, bool writtento);

  //! recursive walk parent links, looking for loops, and grabbing the declRefExpr in the loop init and cond.
  virtual void gatherLoopIndeces(std::vector<chillAST_VarDecl *> &indeces);

  //! recursive walk parent links, looking for loops
  chillAST_ForStmt *findContainingLoop();

  //! recursive walk parent links, avoiding loops
  chillAST_Node *findContainingNonLoop();

  // TODO gather loop init and cond (and if cond) like gatherloopindeces
  //! gather both scalar and array references
  virtual void gatherDeclRefExprs(std::vector<chillAST_DeclRefExpr *> &refs);

  virtual void gatherVarUsage(std::vector<chillAST_VarDecl *> &decls);

  //! Gather all variable that is used as a lefthand side operand
  virtual void gatherVarLHSUsage(std::vector<chillAST_VarDecl *> &decls) {
    fprintf(stderr, "(%s) forgot to implement gatherVarLHSUsage()\n", getTypeString());
  };

  //! ACTUAL Declaration
  virtual void gatherVarDecls(std::vector<chillAST_VarDecl *> &decls);

  virtual void
  gatherVarDeclsMore(std::vector<chillAST_VarDecl *> &decls) {  // even if the decl itself is not in the ast.
    gatherVarDecls(decls);
  };

  //! Actual declaration
  virtual void gatherScalarVarDecls(std::vector<chillAST_VarDecl *> &decls);

  //! Actual declaration
  virtual void gatherArrayVarDecls(std::vector<chillAST_VarDecl *> &decls);

  virtual chillAST_VarDecl *findArrayDecl(const char *name) { // scoping TODO
    if (!getSymbolTable()) return parent->findArrayDecl(name); // most things
    else
      fprintf(stderr, "(%s) forgot to implement gatherArrayVarDecls()\n", getTypeString());
  }


  virtual void replaceVarDecls(chillAST_VarDecl *olddecl, chillAST_VarDecl *newdecl);

  //! this just looks for ForStmts with preferred index metacomment attached
  virtual bool findLoopIndexesToReplace(chillAST_SymbolTable *symtab, bool forcesync = false) {
    fprintf(stderr, "(%s) forgot to implement findLoopIndexesToReplace()\n", getTypeString());
    return false;
  }

  //! Folding constant, to some degree.
  /*!
   * We shoud need to delegate this to the backend compiler
   * @return This node
   */
  virtual chillAST_Node *constantFold();

  virtual chillAST_Node *clone() {   // makes a deep COPY (?)
    CHILL_ERROR("(%s) forgot to implement clone()\n", getTypeString());
    exit(-1);;
  };

  virtual void getTopLevelLoops(std::vector<chillAST_ForStmt *> &loops);

  virtual void repairParentChild();

  virtual void get_deep_loops(std::vector<chillAST_ForStmt *> &loops);


  // generic for chillAST_Node with children
  virtual void find_deepest_loops(std::vector<chillAST_ForStmt *> &loops);

  void setParent(chillAST_Node *p) { parent = p; };

  chillAST_Node *getParent() { return parent; };

  //! This will be ideally replaced by call at to the top level
  chillAST_SourceFile *getSourceFile();

  // TODO DOC
  virtual chillAST_Node *getEnclosingStatement(int level = 0);

  // TODO DOC
  virtual chillAST_VarDecl *multibase() {
    fprintf(stderr, "(%s) forgot to implement multibase()\n", getTypeString());
    exit(-1);
  }

  // TODO DOC
  virtual chillAST_Node *multibase2() {
    fprintf(stderr, "(%s) forgot to implement multibase2()\n", getTypeString());
    exit(-1);
  }


  //! Get a vector of statements
  virtual void gatherStatements(std::vector<chillAST_Node *> &statements);

  virtual chillAST_SymbolTable *getParameters() { return parameters; }

  virtual chillAST_VarDecl *getParameter(const char *name);

  virtual void addParameter(chillAST_VarDecl *name);

  //! Emulation of the old dump function but using printer instead of hardcoded heuritics
  void dump(int indent = 0, FILE *fp = stderr);

  //! Emulation of the old print function to print C-family like syntax but using printer
  void print(int ident = 0, FILE *fp = stderr);

  //! Public interface for fixing child info, which can be called from anynode.
  void fixChildInfo();

  chillAST_Node* findContainingStmt();
};


#endif

