

#ifndef _CHILLAST_ASTs_H_
#define _CHILLAST_ASTs_H_

#include "chillAST_def.hh"
#include "chillAST_node.hh"

class chillAST_NULL : public chillAST_Node {  // NOOP?
public:
  virtual CHILLAST_NODE_TYPE getType() { return CHILLAST_NODE_NULL; }

  chillAST_NULL() {
  };
};

// TODO doc and usage
class chillAST_Preprocessing : public chillAST_Node {
private:
  CHILLAST_PREPROCESSING_POSITION position;
  CHILLAST_PREPROCESSING_TYPE pptype;
  char *blurb;
public:
  virtual CHILLAST_NODE_TYPE getType() { return CHILLAST_NODE_PREPROCESSING; }

  chillAST_Preprocessing();

  chillAST_Preprocessing(CHILLAST_PREPROCESSING_POSITION pos, CHILLAST_PREPROCESSING_TYPE t, char *text);
};

/*!
 * \brief A typedef declaration
 *
 * typedef is a keyword in the C and C++ programming languages. The purpose of typedef is to assign alternative names
 * to existing types, most often those whose standard declaration is cumbersome, potentially confusing, or likely to
 * vary from one implementation to another.
 */
class chillAST_TypedefDecl : public chillAST_Node {
private:
  bool isStruct;
  bool isUnion;
  char *structname;  // get rid of this? 

public:
  virtual CHILLAST_NODE_TYPE getType() { return CHILLAST_NODE_TYPEDEFDECL; }

  char *newtype; // the new type name  ??
  char *underlyingtype;  // float, int, "struct bubba" ? 
  char *arraypart;  // string like "[1234][56]"  ?? 

  chillAST_RecordDecl *rd;  // if it's a struct, point to the recorddecl ??
  // TODO what if   "typedef int[10] tenints; " ??
  void setStructInfo(chillAST_RecordDecl *arrdee) { rd = arrdee; };

  chillAST_RecordDecl *getStructDef();

  bool isAStruct() { return isStruct; };

  bool isAUnion() { return isUnion; };

  void setStruct(bool tf) {
    isStruct = tf;
    fprintf(stderr, "%s isStruct %d\n", structname, isStruct);
  };

  void setUnion(bool tf) { isUnion = tf; };

  void setStructName(const char *newname) { structname = strdup(newname); };

  char *getStructName() { return structname; };

  bool nameis(const char *n) { return !strcmp(n, structname); };

  // special for struct/unions     rethink TODO 
  std::vector<chillAST_VarDecl *> subparts;

  chillAST_VarDecl *findSubpart(const char *name);

  chillAST_TypedefDecl();

  chillAST_TypedefDecl(char *t, const char *nt);

  chillAST_TypedefDecl(char *t, const char *nt, char *a);

  const char *getUnderlyingType() {
    return underlyingtype;
  };
};

//! A variable declaration node
class chillAST_VarDecl : public chillAST_Node {
public:
  virtual CHILLAST_NODE_TYPE getType() { return CHILLAST_NODE_VARDECL; }

  char *vartype; // should probably be an enum, except it's used for unnamed structs too

  //! Points to the recorddecl
  chillAST_RecordDecl *vardef;
  //! Typedef info if it is typedefed
  chillAST_TypedefDecl *typedefinition;

  chillAST_RecordDecl *getStructDef(); // TODO make vardef private?

  //bool insideAStruct;  // this variable is itself part of a struct

  char *underlyingtype;
  char *varname;
  char *arraypart;           // [ 12 ] [ 34 ] if that is how it was defined
  char *arraypointerpart;
  char *arraysetpart;

  void splitarraypart();

  int numdimensions;
  int *arraysizes;       // TODO 
  bool knownArraySizes;  //  if this float *a or float a[128] ?  true means we know ALL dimensions
  int cudamallocsize;      // usually specified in lua/python transformation file 

  bool isRestrict;  // C++ __restrict__ 
  bool isShared; // CUDA  __shared__
  bool isDevice; // CUDA  __device__
  bool isStruct;

  int isAParameter;
  bool byreference;

  void setByReference(bool tf) {
    byreference = tf;
    fprintf(stderr, "byref %d\n", tf);
  };

  bool nameis(const char *n) { return !strcmp(n, varname); };
  bool isABuiltin; // if variable is builtin, we don't need to declare it
  void *uniquePtr;  // DO NOT REFERENCE THROUGH THIS! just used to differentiate declarations 
  bool isArray() { return (numdimensions != 0); };

  bool isAStruct() { return (isStruct || (typedefinition && typedefinition->isAStruct())); }

  void setStruct(bool b) { isStruct = b;/*fprintf(stderr,"vardecl %s IS A STRUCT\n",varname);*/ };

  bool isPointer() { return isArray() && !knownArraySizes; }  //

  bool knowAllDimensions() { return knownArraySizes; };

  chillAST_Node *init;

  void setInit(chillAST_Node *i) {
    init = i;
    i->setParent(this);
  };

  bool hasInit() { return init != NULL; };

  chillAST_Node *getInit() { return init; };

  chillAST_VarDecl();

  chillAST_VarDecl(const char *t, const char *n, const char *a);

  chillAST_VarDecl(const char *t, const char *n, const char *a, void *ptr);

  chillAST_VarDecl(chillAST_TypedefDecl *tdd, const char *n, const char *arraypart);

  chillAST_VarDecl(chillAST_RecordDecl *astruct, const char *n, const char *arraypart);

  void printName(int indent = 0, FILE *fp = stderr);

  bool isParmVarDecl() { return (isAParameter == 1); };

  bool isBuiltin() { return (isABuiltin == 1); };  // designate variable as a builtin
  void setLocation(void *ptr) { uniquePtr = ptr; };


  void gatherVarDecls(std::vector<chillAST_VarDecl *> &decls);

  void gatherScalarVarDecls(std::vector<chillAST_VarDecl *> &decls);

  void gatherArrayVarDecls(std::vector<chillAST_VarDecl *> &decls);

  bool findLoopIndexesToReplace(chillAST_SymbolTable *symtab,
                                bool forcesync = false) { return false; }; // no loops under here
  const char *getUnderlyingType() {  /* fprintf(stderr, "VarDecl getUnderLyingType()\n"); */return underlyingtype; };

  virtual chillAST_VarDecl *getUnderlyingVarDecl() { return this; };

  chillAST_Node *clone();

};

//! referencing a previously defined node, Function or variable
class chillAST_DeclRefExpr : public chillAST_Node {
public:
  virtual CHILLAST_NODE_TYPE getType() { return CHILLAST_NODE_DECLREFEXPR; }

  // variables that are special for this type of node
  char *declarationType;
  char *declarationName;
  chillAST_Node *decl; // the declaration of this variable or function ... uhoh

  // constructors
  chillAST_DeclRefExpr(const char *vartype, const char *variablename, chillAST_Node *dec);

  chillAST_DeclRefExpr(chillAST_VarDecl *vd);

  chillAST_DeclRefExpr(chillAST_FunctionDecl *fd);

  // other methods particular to this type of node
  bool operator!=(chillAST_DeclRefExpr &other) { return decl != other.decl; };

  bool operator==(chillAST_DeclRefExpr &other) { return decl == other.decl; }; // EXACT SAME VARECL BY ADDRESS

  chillAST_Node *getDecl() { return decl; };

  chillAST_VarDecl *getVarDecl() {
    if (!decl) return NULL; // should never happen 
    if (decl->isVarDecl()) return (chillAST_VarDecl *) decl;
    return NULL;
  };

  chillAST_FunctionDecl *getFunctionDecl() {
    if (!decl) return NULL; // should never happen 
    if (decl->isFunctionDecl()) return (chillAST_FunctionDecl *) decl;
    return NULL;
  };

  // required methods that I can't seem to get to inherit
  chillAST_Node *clone();

  void gatherScalarRefs(std::vector<chillAST_DeclRefExpr *> &refs, bool writtento);

  //! returns the decl this declrefexpr references, even if the decl is not in the AST
  void gatherVarDeclsMore(std::vector<chillAST_VarDecl *> &decls);

  void gatherVarUsage(std::vector<chillAST_VarDecl *> &decls);

  void gatherDeclRefExprs(std::vector<chillAST_DeclRefExpr *> &refs);

  void replaceVarDecls(chillAST_VarDecl *olddecl, chillAST_VarDecl *newdecl);

  bool findLoopIndexesToReplace(chillAST_SymbolTable *symtab,
                                bool forcesync = false) { return false; }; // no loops under here
  chillAST_Node *findref() { return this; }// find the SINGLE constant or data reference at this node or below

  const char *getUnderlyingType() {
    CHILL_DEBUG_PRINT("DeclRefExpr getUnderLyingType()\n");
    return decl->getUnderlyingType();
  };

  virtual chillAST_VarDecl *getUnderlyingVarDecl() { return decl->getUnderlyingVarDecl(); } // functions?? TODO

  chillAST_VarDecl *multibase();

  chillAST_Node *multibase2() { return (chillAST_Node *) multibase(); }
};

//! A basic block consists of multiple statements
class chillAST_CompoundStmt : public chillAST_Node {
public:
  virtual CHILLAST_NODE_TYPE getType() { return CHILLAST_NODE_COMPOUNDSTMT; }

  // variables that are special for this type of node
  // constructors
  chillAST_CompoundStmt(); // never has any args ???

  // required methods
  void replaceChild(chillAST_Node *old, chillAST_Node *newchild);

  chillAST_Node *constantFold();

  chillAST_Node *clone();

  void gatherVarDecls(std::vector<chillAST_VarDecl *> &decls);

  void gatherScalarVarDecls(std::vector<chillAST_VarDecl *> &decls);

  void gatherArrayVarDecls(std::vector<chillAST_VarDecl *> &decls);

  void gatherArrayRefs(std::vector<chillAST_ArraySubscriptExpr *> &refs, bool writtento);

  void gatherScalarRefs(std::vector<chillAST_DeclRefExpr *> &refs, bool writtento);

  void gatherVarUsage(std::vector<chillAST_VarDecl *> &decls);

  void gatherDeclRefExprs(std::vector<chillAST_DeclRefExpr *> &refs);

  void replaceVarDecls(chillAST_VarDecl *olddecl, chillAST_VarDecl *newdecl);

  bool findLoopIndexesToReplace(chillAST_SymbolTable *symtab, bool forcesync = false);

  void loseLoopWithLoopVar(char *var); // special case this for not for debugging

  void gatherStatements(std::vector<chillAST_Node *> &statements);
};

//! Declaration of the shape of a struct or union
class chillAST_RecordDecl : public chillAST_Node {
private:
  char *name;  // could be NULL? for unnamed structs?
  char *originalname;
  bool isStruct;

  bool isUnion;
  std::vector<chillAST_VarDecl *> subparts;

public:
  virtual CHILLAST_NODE_TYPE getType() { return CHILLAST_NODE_RECORDDECL; }

  chillAST_RecordDecl(const char *nam, const char *orig);

  chillAST_SymbolTable *getSubparts() {
    return &subparts;
  }

  void setName(const char *newname) { name = strdup(newname); };

  char *getName() { return name; };

  bool isAUnion() { return isUnion; };

  bool isAStruct() { return isStruct; };
  bool isUnnamed;

  void setUnnamed(bool b) { isUnnamed = b; };


  void setStruct(bool tf) { isStruct = tf; };

  //fprintf(stderr, "%s isStruct %d\n", structname, isStruct);  }; 
  void setUnion(bool tf) { isUnion = tf; };

  chillAST_SymbolTable *addVariableToSymbolTable(chillAST_VarDecl *vd); //  RecordDecl does NOTHING

  int numSubparts() { return subparts.size(); };

  void addSubpart(chillAST_VarDecl *s) { subparts.push_back(s); };

  chillAST_VarDecl *findSubpart(const char *name);

  chillAST_VarDecl *findSubpartByType(const char *typ);

  void printStructure(int indent = 0, FILE *fp = stderr);
};

class chillAST_FunctionDecl : public chillAST_Node {
private:
  CHILLAST_FUNCTION_TYPE function_type;  // CHILLAST_FUNCTION_CPU or  CHILLAST_FUNCTION_GPU
  bool externfunc;   // function is external 
  bool builtin;      // function is a builtin
  bool forwarddecl;

public:
  virtual CHILLAST_NODE_TYPE getType() { return CHILLAST_NODE_FUNCTIONDECL; }

  char *returnType;
  char *functionName;

  void printParameterTypes(FILE *fp);

  CHILLAST_FUNCTION_TYPE getFunctionType() { return function_type; }

  void setName(char *n) { functionName = strdup(n); /* probable memory leak */ };

  void setBuiltin() { builtin = true; }; // designate function as a builtin
  bool isBuiltin() { return builtin; }; // report whether is a builtin

  void setExtern() { externfunc = true; }; // designate function as external 
  bool isExtern() { return externfunc; }; // report whether function is external

  void setForward() { forwarddecl = true; }; // designate function as fwd declaration
  bool isForward() { return forwarddecl; }; // report whether function is external

  bool isFunctionCPU() { return (function_type == CHILLAST_FUNCTION_CPU); };

  bool isFunctionGPU() { return (function_type == CHILLAST_FUNCTION_GPU); };

  void setFunctionCPU() { function_type = CHILLAST_FUNCTION_CPU; };

  void setFunctionGPU() { function_type = CHILLAST_FUNCTION_GPU; };

  void *uniquePtr;  // DO NOT REFERENCE THROUGH THIS! USED AS A UNIQUE ID

  chillAST_FunctionDecl(const char *rt, const char *fname, void *unique);

  void addDecl(chillAST_VarDecl *vd);  // just adds to symbol table?? TODO

  void setBody(chillAST_Node *bod) {
    if (bod->isCompoundStmt()) {
      children[0] = bod;
      bod->setParent(this);
    }
  }

  chillAST_CompoundStmt *getBody() { return (chillAST_CompoundStmt *) children[0]; }

  void gatherVarDecls(std::vector<chillAST_VarDecl *> &decls);

  void gatherScalarVarDecls(std::vector<chillAST_VarDecl *> &decls);

  void gatherArrayVarDecls(std::vector<chillAST_VarDecl *> &decls);

  chillAST_VarDecl *findArrayDecl(const char *name);

  void gatherVarUsage(std::vector<chillAST_VarDecl *> &decls);

  void gatherDeclRefExprs(std::vector<chillAST_DeclRefExpr *> &refs);

  void cleanUpVarDecls();

  //void replaceVarDecls( chillAST_VarDecl *olddecl, chillAST_VarDecl *newdecl);
  bool findLoopIndexesToReplace(chillAST_SymbolTable *symtab, bool forcesync = false);

  chillAST_Node *constantFold();

};  // end FunctionDecl

class chillAST_SourceFile : public chillAST_Node {
  // TODO included source file
public:
  virtual CHILLAST_NODE_TYPE getType() { return CHILLAST_NODE_SOURCEFILE; }

  // constructors
  chillAST_SourceFile(const char *filename);  //  defined in chill_ast.cc

  void printToFile(char *filename = NULL);

  char *SourceFileName; // where this originated
  char *FileToWrite;
  char *frontend;

  void setFileToWrite(char *f) { FileToWrite = strdup(f); };

  void setFrontend(const char *compiler) {
    if (frontend) free(frontend);
    frontend = strdup(compiler);
  }
  // get, set filename ? 

  std::vector<chillAST_FunctionDecl *> functions;  // at top level, or anywhere?
  std::vector<chillAST_MacroDefinition *> macrodefinitions;

  chillAST_MacroDefinition *findMacro(const char *name); // TODO ignores arguments
  chillAST_FunctionDecl *findFunction(const char *name); // TODO ignores arguments
  chillAST_Node *findCall(const char *name);

  void addMacro(chillAST_MacroDefinition *md) {
    macrodefinitions.push_back(md);
  }

  void addFunc(chillAST_FunctionDecl *fd) {
    bool already = false;
    for (int i = 0; i < functions.size(); i++) {
      if (functions[i] == fd) {
        already = true;
      }
    }
    if (!already) functions.push_back(fd);
    addChild((chillAST_Node *) fd);
  }

};

//! A Macro definition
class chillAST_MacroDefinition : public chillAST_Node {
private:
  chillAST_Node *body; // rhs      always a compound statement?
public:
  virtual CHILLAST_NODE_TYPE getType() { return CHILLAST_NODE_MACRODEFINITION; }

  char *macroName;

  void setName(char *n) { macroName = strdup(n); /* probable memory leak */ };

  chillAST_MacroDefinition(const char *name, const char *rhs);

  void setBody(chillAST_Node *bod);

  chillAST_Node *getBody() { return (body); }

  chillAST_Node *clone();

  // none of these make sense for macros 
  chillAST_VarDecl *findArrayDecl(const char *name) {};

  bool findLoopIndexesToReplace(chillAST_SymbolTable *symtab, bool forcesync = false) {};
};

class chillAST_ForStmt : public chillAST_Node {
public:
  virtual CHILLAST_NODE_TYPE getType() { return CHILLAST_NODE_FORSTMT; }

  // variables that are special for this type of node
  IR_CONDITION_TYPE conditionoperator;  // from ir_code.hh

  bool hasSymbolTable() { return true; };

  // constructors
  chillAST_ForStmt();

  chillAST_ForStmt(chillAST_Node *ini, chillAST_Node *con, chillAST_Node *inc, chillAST_Node *bod);

  // other methods particular to this type of node
  void addSyncs();

  void removeSyncComment();

  chillAST_Node *getInit() { return getChild(0); };

  void setInit(chillAST_Node *i) { setChild(0, i); }

  chillAST_Node *getCond() { return getChild(1); };

  void setCond(chillAST_Node *c) { setChild(1, c); }

  chillAST_Node *getInc() { return getChild(2); };

  void setInc(chillAST_Node *i) { setChild(2, i); }

  chillAST_Node *getBody() { return getChild(3); };

  void setBody(chillAST_Node *b) { setChild(3, b); };


  bool isNotLeaf() { return true; };

  bool isLeaf() { return false; };


  // required methods that I can't seem to get to inherit
  void printControl(int indent = 0, FILE *fp = stderr);  // print just for ( ... ) but not body

  chillAST_Node *clone();

  bool findLoopIndexesToReplace(chillAST_SymbolTable *symtab, bool forcesync = false);

  void gatherLoopIndeces(std::vector<chillAST_VarDecl *> &indeces);

  void gatherLoopVars(std::vector<std::string> &loopvars);  // gather as strings ??

  void get_deep_loops(std::vector<chillAST_ForStmt *> &loops) { // chillAST_ForStmt version
    // ADD MYSELF!
    loops.push_back(this);

    int n = getBody()->children.size();
    //fprintf(stderr, "get_deep_loops of a %s with %d children\n", getTypeString(), n); 
    for (int i = 0; i < n; i++) {
      //fprintf(stderr, "child %d is a %s\n", i, body->children[i]->getTypeString());
      getBody()->children[i]->get_deep_loops(loops);
    }
    //fprintf(stderr, "found %d deep loops\n", loops.size()); 
  }


  void find_deepest_loops(std::vector<chillAST_ForStmt *> &loops) {
    std::vector<chillAST_ForStmt *> b; // deepest loops below me

    int n = getBody()->children.size();
    for (int i = 0; i < n; i++) {
      std::vector<chillAST_ForStmt *> l; // deepest loops below one child
      getBody()->children[i]->find_deepest_loops(l);
      if (l.size() > b.size()) { // a deeper nesting than we've seen
        b = l;
      }
    }

    loops.push_back(this); // add myself
    for (int i = 0; i < b.size(); i++) loops.push_back(b[i]);
  }


  void loseLoopWithLoopVar(char *var); // chillAST_ForStmt

  bool lowerBound(int &l);

  bool upperBound(int &u);

};

class chillAST_TernaryOperator : public chillAST_Node {
public:
  virtual CHILLAST_NODE_TYPE getType() { return CHILLAST_NODE_TERNARYOPERATOR; }

  // variables that are special for this type of node
  char *op;            // TODO need enum  so far, only "?" conditional operator

  // constructors
  chillAST_TernaryOperator();

  chillAST_TernaryOperator(const char *op, chillAST_Node *cond, chillAST_Node *lhs, chillAST_Node *rhs);

  // other methods particular to this type of node
  bool isNotLeaf() { return true; };

  bool isLeaf() { return false; };

  char *getOp() { return op; };  // dangerous. could get changed!

  chillAST_Node *getCond() { return getChild(0); };

  chillAST_Node *getRHS() { return getChild(1); };

  chillAST_Node *getLHS() { return getChild(2); };

  void setCond(chillAST_Node *newc) { setChild(0, newc); }

  void setLHS(chillAST_Node *newlhs) { setChild(1, newlhs); }

  void setRHS(chillAST_Node *newrhs) { setChild(2, newrhs); }

  // required methods that I can't seem to get to inherit
  chillAST_Node *constantFold();

  chillAST_Node *clone();

  void gatherVarLHSUsage(std::vector<chillAST_VarDecl *> &decls);

  bool findLoopIndexesToReplace(chillAST_SymbolTable *symtab,
                                bool forcesync = false) { return false; }; // no loops under here
  void loseLoopWithLoopVar(char *var) {}; // ternop can't have loop as child?
};

class chillAST_BinaryOperator : public chillAST_Node {
public:
  virtual CHILLAST_NODE_TYPE getType() { return CHILLAST_NODE_BINARYOPERATOR; }

  // variables that are special for this type of node
  char *op;            // TODO need enum

  // constructors
  chillAST_BinaryOperator();

  chillAST_BinaryOperator(chillAST_Node *lhs, const char *op, chillAST_Node *rhs);

  // other methods particular to this type of node
  int evalAsInt();

  chillAST_IntegerLiteral *evalAsIntegerLiteral();

  bool isNotLeaf() { return true; };

  bool isLeaf() { return false; };

  chillAST_Node *getLHS() { return getChild(0); };

  chillAST_Node *getRHS() { return getChild(1); };

  void setLHS(chillAST_Node *newlhs) { setChild(0, newlhs); }

  void setRHS(chillAST_Node *newrhs) { setChild(1, newrhs); }

  char *getOp() { return op; };  // dangerous. could get changed!
  bool isAugmentedAssignmentOp() {
    return
        (!strcmp(op, "*=")) || // BO_MulAssign,
        (!strcmp(op, "/=")) || // BO_DivAssign
        (!strcmp(op, "%=")) || // BO_RemAssign
        (!strcmp(op, "+=")) || // BO_AddAssign
        (!strcmp(op, "-=")) || // BO_SubAssign

        (!strcmp(op, "<<=")) || // BO_ShlAssign
        (!strcmp(op, ">>=")) || // BO_ShrAssign
        (!strcmp(op, "&&=")) || // BO_AndAssign
        (!strcmp(op, "||=")) || // BO_OrAssign

        (!strcmp(op, "^="))    // BO_XorAssign
        ;
  }

  bool isAssignmentOp() {
    return ((!strcmp(op, "=")) ||  // BO_Assign,
            isAugmentedAssignmentOp());
  };

  bool isComparisonOp() {
    return (!strcmp(op, "<")) || (!strcmp(op, ">")) || (!strcmp(op, "<=")) || (!strcmp(op, ">="));
  };

  bool isPlusOp() { return (!strcmp(op, "+")); };

  bool isMinusOp() { return (!strcmp(op, "-")); };

  bool isPlusMinusOp() { return (!strcmp(op, "+")) || (!strcmp(op, "-")); };

  bool isMultDivOp() { return (!strcmp(op, "*")) || (!strcmp(op, "/")); };

  bool isStructOp() { return (!strcmp(op, ".")) || (!strcmp(op, "->")); };

  // required methods that I can't seem to get to inherit
  chillAST_Node *constantFold();

  chillAST_Node *clone();

  void gatherArrayRefs(std::vector<chillAST_ArraySubscriptExpr *> &refs, bool writtento); // chillAST_BinaryOperator
  void gatherScalarRefs(std::vector<chillAST_DeclRefExpr *> &refs, bool writtento);

  void gatherVarLHSUsage(std::vector<chillAST_VarDecl *> &decls);

  bool findLoopIndexesToReplace(chillAST_SymbolTable *symtab,
                                bool forcesync = false) { return false; }; // no loops under here
  void loseLoopWithLoopVar(char *var) {}; // binop can't have loop as child?

};

class chillAST_ArraySubscriptExpr : public chillAST_Node {
public:
  virtual CHILLAST_NODE_TYPE getType() { return CHILLAST_NODE_ARRAYSUBSCRIPTEXPR; }
  // variables that are special for this type of node
  bool imwrittento;
  bool imreadfrom; // WARNING: ONLY used when both writtento and readfrom are true  x += 1 and so on
  chillAST_VarDecl *basedecl; // the vardecl that this refers to
  //! making sure its unique through original reference, DO NOT REFERENCE THROUGH THIS!
  void *uniquePtr;

  // constructors
  chillAST_ArraySubscriptExpr();

  chillAST_ArraySubscriptExpr(chillAST_Node *bas, chillAST_Node *indx, bool writtento, void *unique);

  chillAST_ArraySubscriptExpr(chillAST_VarDecl *v, std::vector<chillAST_Node *> indeces);

  chillAST_Node *getBase() { return getChild(0); }
  void setBase(chillAST_Node* b) { setChild(0, b); }
  chillAST_Node *getIndex() { return getChild(1); }
  void setIndex(chillAST_Node* i) { setChild(1, i); }

  // other methods particular to this type of node
  bool operator!=(const chillAST_ArraySubscriptExpr &);

  bool operator==(const chillAST_ArraySubscriptExpr &);

  //! Method for finding the basedecl, retursn the VARDECL of the thing the subscript is an index into
  chillAST_VarDecl *multibase();

  chillAST_Node *multibase2() { return getBase()->multibase2(); }

  chillAST_Node *getIndex(int dim);

  void gatherIndeces(std::vector<chillAST_Node *> &ind);

  // required methods that I can't seem to get to inherit
  chillAST_Node *clone();

  chillAST_Node *findref() { return this; }// find the SINGLE constant or data reference at this node or below
  void gatherArrayRefs(std::vector<chillAST_ArraySubscriptExpr *> &refs, bool writtento);

  void gatherScalarRefs(std::vector<chillAST_DeclRefExpr *> &refs, bool writtento);

  bool findLoopIndexesToReplace(chillAST_SymbolTable *symtab,
                                bool forcesync = false) { return false; }; // no loops under here

  const char *getUnderlyingType() {
    return getBase()->getUnderlyingType();
  };

  virtual chillAST_VarDecl *getUnderlyingVarDecl() { return getBase()->getUnderlyingVarDecl(); };

};

class chillAST_MemberExpr : public chillAST_Node {
public:
  virtual CHILLAST_NODE_TYPE getType() { return CHILLAST_NODE_MEMBEREXPR; }

  // variables that are special for this type of node
  char *member;
  char *printstring;

  chillAST_VarDecl *basedecl; // the vardecl that this refers to
  void *uniquePtr;  // DO NOT REFERENCE THROUGH THIS!

  CHILLAST_MEMBER_EXP_TYPE exptype;

  // constructors
  chillAST_MemberExpr();
  chillAST_MemberExpr(chillAST_Node *bas, const char *mem, void *unique,
                      CHILLAST_MEMBER_EXP_TYPE t = CHILLAST_MEMBER_EXP_DOT);

  chillAST_Node *getBase() { return getChild(0); }
  void setBase(chillAST_Node *b) { setChild(0,b); }

  // other methods particular to this type of node
  bool operator!=(const chillAST_MemberExpr &);

  bool operator==(const chillAST_MemberExpr &);

  // required methods that I can't seem to get to inherit
  chillAST_Node *clone();

  bool findLoopIndexesToReplace(chillAST_SymbolTable *symtab,
                                bool forcesync = false) { return false; }; // no loops under here

  chillAST_VarDecl *getUnderlyingVarDecl();

  void setType(CHILLAST_MEMBER_EXP_TYPE t) { exptype = t; };

  CHILLAST_MEMBER_EXP_TYPE getType(CHILLAST_MEMBER_EXP_TYPE t) { return exptype; };

  chillAST_VarDecl *multibase();   // this one will return the member decl
  chillAST_Node *multibase2();  // this one will return the member expression
};

class chillAST_IntegerLiteral : public chillAST_Node {
public:
  virtual CHILLAST_NODE_TYPE getType() { return CHILLAST_NODE_INTEGERLITERAL; }

  // variables that are special for this type of node
  int value;

  // constructors
  chillAST_IntegerLiteral(int val);

  // other methods particular to this type of node
  int evalAsInt() { return value; }

  // required methods that I can't seem to get to inherit
  chillAST_Node *constantFold();

  chillAST_Node *clone();

  bool findLoopIndexesToReplace(chillAST_SymbolTable *symtab,
                                bool forcesync = false) { return false; }; // no loops under here

  chillAST_Node *findref() { return this; }// find the SINGLE constant or data reference at this node or below
};

class chillAST_FloatingLiteral : public chillAST_Node {
public:
  virtual CHILLAST_NODE_TYPE getType() { return CHILLAST_NODE_FLOATINGLITERAL; }

  // variables that are special for this type of node
  double value;

  char *allthedigits; // if not NULL, use this as printable representation
  //! Control the precision, float == 1, double == 2
  int precision;

  // constructors
  chillAST_FloatingLiteral(double val, int pre, const char *printable);

  chillAST_FloatingLiteral(chillAST_FloatingLiteral *old);

  // other methods particular to this type of node
  void setPrecision(int precis) { precision = precis; };

  int getPrecision() { return precision; }

  // required methods that I can't seem to get to inherit
  chillAST_Node *constantFold();

  chillAST_Node *clone();

  bool findLoopIndexesToReplace(chillAST_SymbolTable *symtab,
                                bool forcesync = false) { return false; }; // no loops under here
  chillAST_Node *findref() { return this; };// find the SINGLE constant or data reference at this node or below

};

class chillAST_UnaryOperator : public chillAST_Node {
public:
  virtual CHILLAST_NODE_TYPE getType() { return CHILLAST_NODE_UNARYOPERATOR; }

  // variables that are special for this type of node
  //! String representing the operator
  char *op;
  //! true for prefix unary operator
  bool prefix;

  // constructors
  chillAST_UnaryOperator();

  chillAST_UnaryOperator(const char *oper, bool pre, chillAST_Node *sub);

  // other methods particular to this type of node
  bool isAssignmentOp() {
    return ((!strcmp(op, "++")) ||
            (!strcmp(op, "--")));   // are there more ???  TODO
  }

  chillAST_Node *getSubExpr() { return getChild(0); }

  void setSubExpr(chillAST_Node *sub) { setChild(0, sub); }

  // required methods that I can't seem to get to inherit
  chillAST_Node *constantFold();

  chillAST_Node *clone();

  void gatherArrayRefs(std::vector<chillAST_ArraySubscriptExpr *> &refs, bool writtento); // chillAST_UnaryOperator

  void gatherVarLHSUsage(std::vector<chillAST_VarDecl *> &decls);

  //void replaceVarDecls( chillAST_VarDecl *olddecl, chillAST_VarDecl *newdecl);
  bool findLoopIndexesToReplace(chillAST_SymbolTable *symtab,
                                bool forcesync = false) { return false; }; // no loops under here

  int evalAsInt();

};

class chillAST_ImplicitCastExpr : public chillAST_Node {
public:
  virtual CHILLAST_NODE_TYPE getType() { return CHILLAST_NODE_IMPLICITCASTEXPR; }

  // constructors
  chillAST_ImplicitCastExpr();

  chillAST_ImplicitCastExpr(chillAST_Node *sub);

  // other methods particular to this type of node
  bool isNotLeaf() { return true; };

  bool isLeaf() { return false; };

  chillAST_Node *getSubExpr() { return getChild(0); }

  void setSubExpr(chillAST_Node *sub) { return setChild(0, sub); }

  // required methods that I can't seem to get to inherit
  chillAST_Node *clone();

  //void replaceVarDecls( chillAST_VarDecl *olddecl, chillAST_VarDecl *newdecl);
  bool findLoopIndexesToReplace(chillAST_SymbolTable *symtab,
                                bool forcesync = false) { return false; }; // no loops under here
  chillAST_VarDecl *multibase(); // just recurse on subexpr

};

class chillAST_CStyleCastExpr : public chillAST_Node {
public:
  virtual CHILLAST_NODE_TYPE getType() { return CHILLAST_NODE_CSTYLECASTEXPR; }

  // variables that are special for this type of node
  //! String representing the type it casts to
  char *towhat;

  // constructors
  chillAST_CStyleCastExpr();

  chillAST_CStyleCastExpr(const char *to, chillAST_Node *sub);

  // other methods particular to this type of node


  // required methods that I can't seem to get to inherit
  chillAST_Node *getSubExpr() { return getChild(0); }

  void setSubExpr(chillAST_Node *sub) { return setChild(0, sub); }

  chillAST_Node *clone();

  bool findLoopIndexesToReplace(chillAST_SymbolTable *symtab,
                                bool forcesync = false) { return false; }; // no loops under here
  chillAST_Node *findref() { return getSubExpr(); };// find the SINGLE constant or data reference at this node or below

};

class chillAST_CStyleAddressOf : public chillAST_Node {
public:
  virtual CHILLAST_NODE_TYPE getType() { return CHILLAST_NODE_CSTYLEADDRESSOF; }

  // variables that are special for this type of node

  // constructors
  chillAST_CStyleAddressOf();

  chillAST_CStyleAddressOf(chillAST_Node *sub);

  // other methods particular to this type of node

  chillAST_Node *getSubExpr() { return getChild(0); }

  void setSubExpr(chillAST_Node *sub) { return setChild(0, sub); }

  // required methods that I can't seem to get to inherit
  chillAST_Node *clone();

  //void replaceVarDecls( chillAST_VarDecl *olddecl, chillAST_VarDecl *newdecl);
  bool findLoopIndexesToReplace(chillAST_SymbolTable *symtab,
                                bool forcesync = false) { return false; }; // no loops under here


};

class chillAST_CudaMalloc : public chillAST_Node {
public:
  virtual CHILLAST_NODE_TYPE getType() { return CHILLAST_NODE_CUDAMALLOC; }

  // constructors
  chillAST_CudaMalloc();

  chillAST_CudaMalloc(chillAST_Node *devmemptr, chillAST_Node *size);

  // other methods particular to this type of node
  chillAST_Node *getDevPtr() { return getChild(0); }

  void setDevPtr(chillAST_Node *devptr) { setChild(0, devptr); }

  chillAST_Node *getSize() { return getChild(1); }

  void setSize(chillAST_Node *size) { setChild(1, size); }


  // required methods that I can't seem to get to inherit
  chillAST_Node *clone();

  void gatherArrayRefs(std::vector<chillAST_ArraySubscriptExpr *> &refs, bool writtento);

  void gatherScalarRefs(std::vector<chillAST_DeclRefExpr *> &refs, bool writtento);

  bool findLoopIndexesToReplace(chillAST_SymbolTable *symtab,
                                bool forcesync = false) { return false; }; // no loops under here

};

//! CudaFree
class chillAST_CudaFree : public chillAST_Node {
public:
  virtual CHILLAST_NODE_TYPE getType() { return CHILLAST_NODE_CUDAFREE; }

  // constructors
  chillAST_CudaFree();

  chillAST_CudaFree(chillAST_Node *var);

  // other methods particular to this type of node
  chillAST_Node *getPointer() { return getChild(0); }

  void setPointer(chillAST_Node *ptr) { setChild(0, ptr); }


  // required methods that I can't seem to get to inherit
  chillAST_Node *clone();

  bool findLoopIndexesToReplace(chillAST_SymbolTable *symtab,
                                bool forcesync = false) { return false; }; // no loops under here

};

class chillAST_Malloc : public chillAST_Node {   // malloc( sizeof(int) * 2048 );
public:
  virtual CHILLAST_NODE_TYPE getType() { return CHILLAST_NODE_MALLOC; }

  // variables that are special for this type of node

  // constructors
  chillAST_Malloc();

  chillAST_Malloc(chillAST_Node *numthings); // malloc (sizeof(int) *1024)

  // other methods particular to this type of node
  chillAST_Node *getSize() { return getChild(0); }

  void setSize(chillAST_Node *size) { setChild(0, size); }

  // required methods that I can't seem to get to inherit
  chillAST_Node *clone();

  bool findLoopIndexesToReplace(chillAST_SymbolTable *symtab,
                                bool forcesync = false) { return false; }; // no loops under here

};

class chillAST_Free : public chillAST_Node {
public:
  virtual CHILLAST_NODE_TYPE getType() { return CHILLAST_NODE_FREE; }

};

class chillAST_CudaMemcpy : public chillAST_Node {
public:
  virtual CHILLAST_NODE_TYPE getType() { return CHILLAST_NODE_CUDAMEMCPY; }

  // variables that are special for this type of node
  char *cudaMemcpyKind;  // could use the actual enum

  // constructors
  chillAST_CudaMemcpy();
  chillAST_CudaMemcpy(chillAST_Node *d, chillAST_Node *s, chillAST_Node *size, char *kind);

  // other methods particular to this type of node
  chillAST_Node *getDest() {return getChild(0);}
  void setDest(chillAST_Node* dest) { setChild(0,dest); }
  chillAST_Node *getSrc() {return getChild(1);}
  void setSrc(chillAST_Node* src) { setChild(1,src); }
  chillAST_Node *getSize() {return getChild(2);}
  void setSize(chillAST_Node* size) { setChild(2,size); }


  // required methods that I can't seem to get to inherit
  chillAST_Node *clone();

  void gatherArrayRefs(std::vector<chillAST_ArraySubscriptExpr *> &refs, bool writtento);

  void gatherScalarRefs(std::vector<chillAST_DeclRefExpr *> &refs, bool writtento);

  bool findLoopIndexesToReplace(chillAST_SymbolTable *symtab,
                                bool forcesync = false) { return false; }; // no loops under here

};

class chillAST_CudaSyncthreads : public chillAST_Node {
public:
  virtual CHILLAST_NODE_TYPE getType() { return CHILLAST_NODE_CUDASYNCTHREADS; }

  // constructors
  chillAST_CudaSyncthreads();
};

class chillAST_ReturnStmt : public chillAST_Node {
public:
  virtual CHILLAST_NODE_TYPE getType() { return CHILLAST_NODE_RETURNSTMT; }

  // constructors
  chillAST_ReturnStmt();

  chillAST_ReturnStmt(chillAST_Node *retval);

  // other methods particular to this type of node
  chillAST_Node *getRetVal() { return getChild(0); }

  void setRetVal(chillAST_Node *ret) { setChild(0, ret); }

  // required methods that I can't seem to get to inherit
  chillAST_Node *clone();

  //void replaceVarDecls( chillAST_VarDecl *olddecl, chillAST_VarDecl *newdecl);
  bool findLoopIndexesToReplace(chillAST_SymbolTable *symtab,
                                bool forcesync = false) { return false; }; // no loops under here

};

class chillAST_CallExpr : public chillAST_Node {  // a function call
  // Is macro call without pointer a part of this
public:
  virtual CHILLAST_NODE_TYPE getType() { return CHILLAST_NODE_CALLEXPR; }

  // variables that are special for this type of node
  chillAST_VarDecl *grid;
  chillAST_VarDecl *block;

  // constructors
  chillAST_CallExpr();
  chillAST_CallExpr(chillAST_Node *function);

  void addArg(chillAST_Node *newarg) { addChild(newarg); }

  // other methods particular to this type of node
  // TODO get/set grid, block
  chillAST_Node *getCallee() { return getChild(0); }

  void setCallee(chillAST_Node *c) { setChild(0, c); }

  // required methods that I can't seem to get to inherit
  bool findLoopIndexesToReplace(chillAST_SymbolTable *symtab,
                                bool forcesync = false) { return false; }; // no loops under here
  chillAST_Node *clone();
};

class chillAST_ParenExpr : public chillAST_Node {
public:
  virtual CHILLAST_NODE_TYPE getType() { return CHILLAST_NODE_PARENEXPR; }

  // constructors
  chillAST_ParenExpr();

  chillAST_ParenExpr(chillAST_Node *sub);

  // other methods particular to this type of node
  chillAST_Node *getSubExpr() { return getChild(0); }

  void setSubExpr(chillAST_Node *sub) { setChild(0, sub); }

  // required methods that I can't seem to get to inherit
  chillAST_Node *clone();

  bool findLoopIndexesToReplace(chillAST_SymbolTable *symtab,
                                bool forcesync = false) { return false; }; // no loops under here

};

class chillAST_Sizeof : public chillAST_Node {
public:
  virtual CHILLAST_NODE_TYPE getType() { return CHILLAST_NODE_SIZEOF; }

  // variables that are special for this type of node
  //! the object of sizeof function
  char *thing;

  // constructors
  chillAST_Sizeof(char *t);

  // other methods particular to this type of node


  // required methods that I can't seem to get to inherit
  chillAST_Node *clone();

  bool findLoopIndexesToReplace(chillAST_SymbolTable *symtab,
                                bool forcesync = false) { return false; }; // no loops under here

};

class chillAST_NoOp : public chillAST_Node {
public:
  virtual CHILLAST_NODE_TYPE getType() { return CHILLAST_NODE_NOOP; }

  chillAST_NoOp();

  // required methods that I can't seem to get to inherit
  chillAST_Node *clone() {
    chillAST_Node *n = new chillAST_NoOp();
    n->setParent(parent);
    return n;
  }; // ??

  bool findLoopIndexesToReplace(chillAST_SymbolTable *symtab,
                                bool forcesync = false) { return false; };//no loops under here
};

class chillAST_IfStmt : public chillAST_Node {
public:
  virtual CHILLAST_NODE_TYPE getType() { return CHILLAST_NODE_IFSTMT; }

  // variables that are special for this type of node
  IR_CONDITION_TYPE conditionoperator;  // from ir_code.hh

  // constructors
  chillAST_IfStmt();

  chillAST_IfStmt(chillAST_Node *c, chillAST_Node *t, chillAST_Node *e);

  // other methods particular to this type of node
  chillAST_Node *getCond() { return getChild(0); };

  chillAST_Node *getThen() { return getChild(1); };

  chillAST_Node *getElse() { return getChild(2); };

  void setCond(chillAST_Node *b) { setChild(0, b); };

  void setThen(chillAST_Node *b) { setChild(1, b); };

  void setElse(chillAST_Node *b) { setChild(2, b); };

  // required methods that I can't seem to get to inherit
  chillAST_Node *clone();

  void gatherArrayRefs(std::vector<chillAST_ArraySubscriptExpr *> &refs, bool writtento);

  void gatherScalarRefs(std::vector<chillAST_DeclRefExpr *> &refs, bool writtento);

  bool findLoopIndexesToReplace(chillAST_SymbolTable *symtab, bool forcesync = false);

};

class chillAST_something : public chillAST_Node {
public:
  virtual CHILLAST_NODE_TYPE getType() { return CHILLAST_NODE_UNKNOWN; }

  // constructors
  chillAST_something();
};


#endif

