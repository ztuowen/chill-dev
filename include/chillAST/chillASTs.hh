

#ifndef _CHILLAST_ASTs_H_
#define _CHILLAST_ASTs_H_

#include "chillAST_def.hh"
#include "chillAST_node.hh"

class chillAST_NULL : public chillAST_Node {  // NOOP?
public:
  virtual CHILLAST_NODE_TYPE getType(){return CHILLAST_NODE_NULL;}
  chillAST_NULL() {
  };

  void print(int indent = 0, FILE *fp = stderr) {
    chillindent(indent, fp);
    fprintf(fp, "/* (NULL statement); */ ");
    fflush(fp);
  }

};

class chillAST_Preprocessing : public chillAST_Node {
public:
  virtual CHILLAST_NODE_TYPE getType(){return CHILLAST_NODE_PREPROCESSING;}
  // variables that are special for this type of node
  CHILLAST_PREPROCESSING_POSITION position;
  CHILLAST_PREPROCESSING_TYPE pptype;
  char *blurb;

  // constructors
  chillAST_Preprocessing(); // not sure what this is good for
  chillAST_Preprocessing(CHILLAST_PREPROCESSING_POSITION pos, CHILLAST_PREPROCESSING_TYPE t, char *text);

  // other methods particular to this type of node

  // required methods that I can't seem to get to inherit
  void print(int indent = 0, FILE *fp = stderr);  // print CODE   in chill_ast.cc
  //void dump(  int indent=0,  FILE *fp = stderr );  // print ast    in chill_ast.cc
};

//typedef is a keyword in the C and C++ programming languages. The purpose of typedef is to assign alternative names to existing types, most often those whose standard declaration is cumbersome, potentially confusing, or likely to vary from one implementation to another. 
class chillAST_TypedefDecl : public chillAST_Node {
private:
  bool isStruct;
  bool isUnion;
  char *structname;  // get rid of this? 

public:
  virtual CHILLAST_NODE_TYPE getType(){return CHILLAST_NODE_TYPEDEFDECL;}
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
    fprintf(stderr, "TypedefDecl getUnderLyingType()\n");
    return underlyingtype;
  };

  void print(int indent = 0, FILE *fp = stderr);

};

class chillAST_VarDecl : public chillAST_Node {
public:
  virtual CHILLAST_NODE_TYPE getType(){return CHILLAST_NODE_VARDECL;}
  char *vartype; // should probably be an enum, except it's used for unnamed structs too

  chillAST_RecordDecl *vardef;// the thing that says what the struct looks like
  chillAST_TypedefDecl *typedefinition; // NULL for float, int, etc.
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

  void print(int indent = 0, FILE *fp = stderr);

  void printName(int indent = 0, FILE *fp = stderr);

  bool isParmVarDecl() { return (isAParameter == 1); };

  bool isBuiltin() { return (isABuiltin == 1); };  // designate variable as a builtin
  void setLocation(void *ptr) { uniquePtr = ptr; };


  void gatherVarDecls(std::vector<chillAST_VarDecl *> &decls);

  void gatherVarDeclsMore(std::vector<chillAST_VarDecl *> &decls) { gatherVarDecls(decls); };

  void gatherScalarVarDecls(std::vector<chillAST_VarDecl *> &decls);

  void gatherArrayVarDecls(std::vector<chillAST_VarDecl *> &decls);

  void gatherVarUsage(std::vector<chillAST_VarDecl *> &decls) {}; // does nothing
  void gatherDeclRefExprs(std::vector<chillAST_DeclRefExpr *> &refs) {}; // does nothing
  void replaceVarDecls(chillAST_VarDecl *olddecl, chillAST_VarDecl *newdecl) {};

  bool findLoopIndexesToReplace(chillAST_SymbolTable *symtab,
                                bool forcesync = false) { return false; }; // no loops under here
  const char *getUnderlyingType() {  /* fprintf(stderr, "VarDecl getUnderLyingType()\n"); */return underlyingtype; };

  virtual chillAST_VarDecl *getUnderlyingVarDecl() { return this; };

  chillAST_Node *constantFold();

  chillAST_Node *clone();

};

class chillAST_DeclRefExpr : public chillAST_Node {
public:
  virtual CHILLAST_NODE_TYPE getType(){return CHILLAST_NODE_DECLREFEXPR;}
  // variables that are special for this type of node
  char *declarationType;
  char *declarationName;
  chillAST_Node *decl; // the declaration of this variable or function ... uhoh
  //char *functionparameters;  // TODO probably should split this node into 2 types, one for variables, one for functions

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
  void print(int indent = 0, FILE *fp = stderr);  // print CODE
  char *stringRep(int indent = 0);

  chillAST_Node *constantFold();

  chillAST_Node *clone();

  void gatherArrayRefs(std::vector<chillAST_ArraySubscriptExpr *> &refs, bool writtento) {}; // do nothing
  void gatherScalarRefs(std::vector<chillAST_DeclRefExpr *> &refs, bool writtento);

  // this is the AST node where these 2 differ 
  void gatherVarDecls(
      std::vector<chillAST_VarDecl *> &decls) {};  // does nothing, to get the cvardecl using this method, the actual vardecl must be in the AST
  void gatherVarDeclsMore(
      std::vector<chillAST_VarDecl *> &decls); // returns the decl this declrefexpr references, even if the decl is not in the AST


  void gatherScalarVarDecls(std::vector<chillAST_VarDecl *> &decls);

  void gatherArrayVarDecls(std::vector<chillAST_VarDecl *> &decls);

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

class chillAST_CompoundStmt : public chillAST_Node {
public:
  virtual CHILLAST_NODE_TYPE getType(){return CHILLAST_NODE_COMPOUNDSTMT;}
  // variables that are special for this type of node
  // constructors
  chillAST_CompoundStmt(); // never has any args ???

  // other methods particular to this type of node


  // required methods 
  void replaceChild(chillAST_Node *old, chillAST_Node *newchild);

  void print(int indent = 0, FILE *fp = stderr);

  chillAST_Node *constantFold();

  chillAST_Node *clone();

  void gatherVarDecls(std::vector<chillAST_VarDecl *> &decls);

  void gatherVarDeclsMore(std::vector<chillAST_VarDecl *> &decls) { gatherVarDecls(decls); };

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

class chillAST_RecordDecl : public chillAST_Node {  // declaration of the shape of a struct or union
private:
  char *name;  // could be NULL? for unnamed structs?
  char *originalname;
  bool isStruct;

  bool isUnion;
  std::vector<chillAST_VarDecl *> subparts;

public:
  virtual CHILLAST_NODE_TYPE getType(){return CHILLAST_NODE_RECORDDECL;}
  chillAST_RecordDecl(const char *nam, const char *orig);

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

  void print(int indent = 0, FILE *fp = stderr);

  void printStructure(int indent = 0, FILE *fp = stderr);
};

class chillAST_FunctionDecl : public chillAST_Node {
private:
  chillAST_CompoundStmt *body; // always a compound statement?
  CHILLAST_FUNCTION_TYPE function_type;  // CHILLAST_FUNCTION_CPU or  CHILLAST_FUNCTION_GPU
  bool externfunc;   // function is external 
  bool builtin;      // function is a builtin
  bool forwarddecl;

public:
  virtual CHILLAST_NODE_TYPE getType(){return CHILLAST_NODE_FUNCTIONDECL;}
  char *returnType;
  char *functionName;

  void printParameterTypes(FILE *fp);

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

  void addChild(chillAST_Node *node); // special because inserts into BODY
  void insertChild(int i, chillAST_Node *node); // special because inserts into BODY

  void setBody(chillAST_Node *bod);

  chillAST_CompoundStmt *getBody() { return (body); }

  void print(int indent = 0, FILE *fp = stderr); // in chill_ast.cc

  void gatherVarDecls(std::vector<chillAST_VarDecl *> &decls);

  void gatherVarDeclsMore(std::vector<chillAST_VarDecl *> &decls) { gatherVarDecls(decls); };

  void gatherScalarVarDecls(std::vector<chillAST_VarDecl *> &decls);

  void gatherArrayVarDecls(std::vector<chillAST_VarDecl *> &decls);

  chillAST_VarDecl *findArrayDecl(const char *name);
  //void gatherArrayRefs( std::vector<chillAST_ArraySubscriptExpr*> &refs, bool writtento ); 
  //void gatherScalarRefs( std::vector<chillAST_DeclRefExpr*> &refs, bool writtento ) 

  void gatherVarUsage(std::vector<chillAST_VarDecl *> &decls);

  void gatherDeclRefExprs(std::vector<chillAST_DeclRefExpr *> &refs);

  void cleanUpVarDecls();

  //void replaceVarDecls( chillAST_VarDecl *olddecl, chillAST_VarDecl *newdecl);
  bool findLoopIndexesToReplace(chillAST_SymbolTable *symtab, bool forcesync = false);

  chillAST_Node *constantFold();

  void replaceChild(chillAST_Node *old, chillAST_Node *newchild) {
    body->replaceChild(old, newchild);
  }
};  // end FunctionDecl 

class chillAST_SourceFile : public chillAST_Node {
public:
  virtual CHILLAST_NODE_TYPE getType(){return CHILLAST_NODE_SOURCEFILE;}

  // constructors
  chillAST_SourceFile(const char *filename);  //  defined in chill_ast.cc

  void print(int indent = 0, FILE *fp = stderr);  // print CODE   in chill_ast.cc
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
    //fprintf(stderr, "addMacro(), now %d macros\n", macrodefinitions.size()); 
  }

  void addFunc(chillAST_FunctionDecl *fd) {
    //fprintf(stderr, "chillAST_SourceFile::addFunc( %s %p)\n", fd->functionName, fd);

    bool already = false;
    for (int i = 0; i < functions.size(); i++) {
      //fprintf(stderr, "function %d is %s %p\n", i, functions[i]->functionName, functions[i]); 
      if (functions[i] == fd) {
        //fprintf(stderr, "function %s was already in source functions\n", fd->functionName); 
        already = true;
      }
    }
    if (!already) functions.push_back(fd);

    // PROBABLY fd was created with sourcefile as its parent. Don't add it twice
    addChild((chillAST_Node *) fd);
  }

};

class chillAST_MacroDefinition : public chillAST_Node {
private:
  chillAST_Node *body; // rhs      always a compound statement?
public:
  virtual CHILLAST_NODE_TYPE getType(){return CHILLAST_NODE_MACRODEFINITION;}
  char *macroName;
  char *rhsString;

  // parameters - these will be odd, in that they HAVE NO TYPE
  void setName(char *n) { macroName = strdup(n); /* probable memory leak */ };

  void setRhsString(char *n) { rhsString = strdup(n); /* probable memory leak */ };

  char *getRhsString() { return rhsString; }

  chillAST_MacroDefinition(const char *name, const char *rhs);

  void addChild(chillAST_Node *node); // special because inserts into BODY
  void insertChild(int i, chillAST_Node *node); // special because inserts into BODY

  void setBody(chillAST_Node *bod);

  chillAST_Node *getBody() { return (body); }

  void print(int indent = 0, FILE *fp = stderr); // in chill_ast.cc

  chillAST_Node *clone();

  // none of these make sense for macros 
  void gatherVarDecls(std::vector<chillAST_VarDecl *> &decls) {};

  void gatherVarDeclsMore(std::vector<chillAST_VarDecl *> &decls) { gatherVarDecls(decls); };

  void gatherScalarVarDecls(std::vector<chillAST_VarDecl *> &decls) {};

  void gatherArrayVarDecls(std::vector<chillAST_VarDecl *> &decls) {};

  chillAST_VarDecl *findArrayDecl(const char *name) {};

  void gatherVarUsage(std::vector<chillAST_VarDecl *> &decls) {};

  void gatherDeclRefExprs(std::vector<chillAST_DeclRefExpr *> &refs) {};

  void cleanUpVarDecls();

  bool findLoopIndexesToReplace(chillAST_SymbolTable *symtab, bool forcesync = false) {};

  chillAST_Node *constantFold() {};
};

class chillAST_ForStmt : public chillAST_Node {
public:
  virtual CHILLAST_NODE_TYPE getType(){return CHILLAST_NODE_FORSTMT;}
  // variables that are special for this type of node
  chillAST_Node *init;
  chillAST_Node *cond;
  chillAST_Node *incr;
  chillAST_Node *body; // always a compoundstmt?
  IR_CONDITION_TYPE conditionoperator;  // from ir_code.hh

  bool hasSymbolTable() { return true; };

  // constructors
  chillAST_ForStmt();

  chillAST_ForStmt(chillAST_Node *ini, chillAST_Node *con, chillAST_Node *inc, chillAST_Node *bod);

  // other methods particular to this type of node
  void addSyncs();

  void removeSyncComment();

  chillAST_Node *getInit() { return init; };

  chillAST_Node *getCond() { return cond; };

  chillAST_Node *getInc() { return incr; };

  chillAST_Node *
  getBody() { //fprintf(stderr, "chillAST_ForStmt::getBody(), returning a chillAST_Node of type %s\n", body->getTypeString());
    return body;
  };

  void setBody(chillAST_Node *b) {
    body = b;
    b->parent = this;
  };

  bool isNotLeaf() { return true; };

  bool isLeaf() { return false; };


  // required methods that I can't seem to get to inherit
  void print(int indent = 0, FILE *fp = stderr);  // print CODE   in chill_ast.cc
  void printControl(int indent = 0, FILE *fp = stderr);  // print just for ( ... ) but not body

  chillAST_Node *constantFold();

  chillAST_Node *clone();

  void gatherVarDecls(std::vector<chillAST_VarDecl *> &decls);

  void gatherVarDeclsMore(std::vector<chillAST_VarDecl *> &decls) { gatherVarDecls(decls); };

  void gatherScalarVarDecls(std::vector<chillAST_VarDecl *> &decls);

  void gatherArrayVarDecls(std::vector<chillAST_VarDecl *> &decls);

  void gatherArrayRefs(std::vector<chillAST_ArraySubscriptExpr *> &refs, bool writtento);

  void gatherScalarRefs(std::vector<chillAST_DeclRefExpr *> &refs, bool writtento);

  void gatherVarUsage(std::vector<chillAST_VarDecl *> &decls);

  void gatherDeclRefExprs(std::vector<chillAST_DeclRefExpr *> &refs);

  void replaceVarDecls(chillAST_VarDecl *olddecl, chillAST_VarDecl *newdecl); // will get called on inner loops
  bool findLoopIndexesToReplace(chillAST_SymbolTable *symtab, bool forcesync = false);

  void gatherLoopIndeces(std::vector<chillAST_VarDecl *> &indeces);

  void gatherLoopVars(std::vector<std::string> &loopvars);  // gather as strings ??

  void get_deep_loops(std::vector<chillAST_ForStmt *> &loops) { // chillAST_ForStmt version
    // ADD MYSELF!
    loops.push_back(this);

    int n = body->children.size();
    //fprintf(stderr, "get_deep_loops of a %s with %d children\n", getTypeString(), n); 
    for (int i = 0; i < n; i++) {
      //fprintf(stderr, "child %d is a %s\n", i, body->children[i]->getTypeString()); 
      body->children[i]->get_deep_loops(loops);
    }
    //fprintf(stderr, "found %d deep loops\n", loops.size()); 
  }


  void find_deepest_loops(std::vector<chillAST_ForStmt *> &loops) {
    std::vector<chillAST_ForStmt *> b; // deepest loops below me

    int n = body->children.size();
    for (int i = 0; i < n; i++) {
      std::vector<chillAST_ForStmt *> l; // deepest loops below one child
      body->children[i]->find_deepest_loops(l);
      if (l.size() > b.size()) { // a deeper nesting than we've seen
        b = l;
      }
    }

    loops.push_back(this); // add myself
    for (int i = 0; i < b.size(); i++) loops.push_back(b[i]);
  }


  void loseLoopWithLoopVar(char *var); // chillAST_ForStmt
  void replaceChild(chillAST_Node *old, chillAST_Node *newchild);

  void gatherStatements(std::vector<chillAST_Node *> &statements);

  bool lowerBound(int &l);

  bool upperBound(int &u);

};

class chillAST_TernaryOperator : public chillAST_Node {
public:
  virtual int getPrec() {return INT8_MAX+15;}
  virtual CHILLAST_NODE_TYPE getType(){return CHILLAST_NODE_TERNARYOPERATOR;}
  // variables that are special for this type of node
  char *op;            // TODO need enum  so far, only "?" conditional operator
  chillAST_Node *condition;
  chillAST_Node *lhs;        // keep name from binary
  chillAST_Node *rhs;


  // constructors
  chillAST_TernaryOperator();

  chillAST_TernaryOperator(const char *op, chillAST_Node *cond, chillAST_Node *lhs, chillAST_Node *rhs);

  // other methods particular to this type of node
  bool isNotLeaf() { return true; };

  bool isLeaf() { return false; };


  char *getOp() { return op; };  // dangerous. could get changed!
  chillAST_Node *getCond() { return condition; };

  chillAST_Node *getRHS() { return rhs; };

  chillAST_Node *getLHS() { return lhs; };

  void setCond(chillAST_Node *newc) {
    condition = newc;
    newc->setParent(this);
  }

  void setLHS(chillAST_Node *newlhs) {
    lhs = newlhs;
    newlhs->setParent(this);
  }

  void setRHS(chillAST_Node *newrhs) {
    rhs = newrhs;
    newrhs->setParent(this);
  }


  // required methods that I can't seem to get to inherit
  void print(int indent = 0, FILE *fp = stderr);  // print CODE   in chill_ast.cc
  void printonly(int indent = 0, FILE *fp = stderr);

  chillAST_Node *constantFold();

  chillAST_Node *clone();

  void replaceChild(chillAST_Node *old, chillAST_Node *newchild);

  void gatherArrayRefs(std::vector<chillAST_ArraySubscriptExpr *> &refs, bool writtento);

  void gatherScalarRefs(std::vector<chillAST_DeclRefExpr *> &refs, bool writtento);

  void gatherVarDecls(std::vector<chillAST_VarDecl *> &decls);

  void gatherVarDeclsMore(std::vector<chillAST_VarDecl *> &decls) { gatherVarDecls(decls); };

  void gatherScalarVarDecls(std::vector<chillAST_VarDecl *> &decls);

  void gatherArrayVarDecls(std::vector<chillAST_VarDecl *> &decls);

  void gatherVarUsage(std::vector<chillAST_VarDecl *> &decls);

  void gatherDeclRefExprs(std::vector<chillAST_DeclRefExpr *> &refs);

  void gatherVarLHSUsage(std::vector<chillAST_VarDecl *> &decls);

  void replaceVarDecls(chillAST_VarDecl *olddecl, chillAST_VarDecl *newdecl);

  bool findLoopIndexesToReplace(chillAST_SymbolTable *symtab,
                                bool forcesync = false) { return false; }; // no loops under here
  void loseLoopWithLoopVar(char *var) {}; // ternop can't have loop as child?
};

class chillAST_BinaryOperator : public chillAST_Node {
public:
  virtual int getPrec();
  virtual CHILLAST_NODE_TYPE getType(){return CHILLAST_NODE_BINARYOPERATOR;}
  // variables that are special for this type of node
  char *op;            // TODO need enum
  chillAST_Node *lhs;
  chillAST_Node *rhs;


  // constructors
  chillAST_BinaryOperator(chillAST_Node *lhs, const char *op, chillAST_Node *rhs);

  // other methods particular to this type of node
  int evalAsInt();

  chillAST_IntegerLiteral *evalAsIntegerLiteral();

  bool isNotLeaf() { return true; };

  bool isLeaf() { return false; };

  chillAST_Node *getRHS() { return rhs; };

  chillAST_Node *getLHS() { return lhs; };

  void setLHS(chillAST_Node *newlhs) {
    lhs = newlhs;
    newlhs->setParent(this);
  }

  void setRHS(chillAST_Node *newrhs) {
    rhs = newrhs;
    newrhs->setParent(this);
  }

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
  void print(int indent = 0, FILE *fp = stderr);  // print CODE   in chill_ast.cc
  void printonly(int indent = 0, FILE *fp = stderr);

  char *stringRep(int indent = 0);

  chillAST_Node *constantFold();

  chillAST_Node *clone();

  void replaceChild(chillAST_Node *old, chillAST_Node *newchild);

  void gatherArrayRefs(std::vector<chillAST_ArraySubscriptExpr *> &refs, bool writtento); // chillAST_BinaryOperator
  void gatherScalarRefs(std::vector<chillAST_DeclRefExpr *> &refs, bool writtento);

  void gatherVarDecls(std::vector<chillAST_VarDecl *> &decls);

  void gatherVarDeclsMore(std::vector<chillAST_VarDecl *> &decls) { gatherVarDecls(decls); };

  void gatherScalarVarDecls(std::vector<chillAST_VarDecl *> &decls);

  void gatherArrayVarDecls(std::vector<chillAST_VarDecl *> &decls);

  void gatherVarUsage(std::vector<chillAST_VarDecl *> &decls);

  void gatherDeclRefExprs(std::vector<chillAST_DeclRefExpr *> &refs);

  void gatherVarLHSUsage(std::vector<chillAST_VarDecl *> &decls);

  void replaceVarDecls(chillAST_VarDecl *olddecl, chillAST_VarDecl *newdecl);

  bool findLoopIndexesToReplace(chillAST_SymbolTable *symtab,
                                bool forcesync = false) { return false; }; // no loops under here
  void loseLoopWithLoopVar(char *var) {}; // binop can't have loop as child?

  void gatherStatements(std::vector<chillAST_Node *> &statements); //

};

class chillAST_ArraySubscriptExpr : public chillAST_Node {
public:
  virtual CHILLAST_NODE_TYPE getType(){return CHILLAST_NODE_ARRAYSUBSCRIPTEXPR;}
  // variables that are special for this type of node

  //! always a decl ref expr? No, for multidimensional array, is another ASE
  chillAST_Node *base;
  chillAST_Node *index;
  bool imwrittento;
  bool imreadfrom; // WARNING: ONLY used when both writtento and readfrom are true  x += 1 and so on
  chillAST_VarDecl *basedecl; // the vardecl that this refers to
  //! making sure its unique through original reference, DO NOT REFERENCE THROUGH THIS!
  void *uniquePtr;

  // constructors
  chillAST_ArraySubscriptExpr(chillAST_Node *bas, chillAST_Node *indx, bool writtento, void *unique);

  chillAST_ArraySubscriptExpr(chillAST_VarDecl *v, std::vector<chillAST_Node *> indeces);

  // other methods particular to this type of node
  bool operator!=(const chillAST_ArraySubscriptExpr &);

  bool operator==(const chillAST_ArraySubscriptExpr &);

  //! Method for finding the basedecl, retursn the VARDECL of the thing the subscript is an index into
  chillAST_VarDecl *multibase();
  chillAST_Node *multibase2() { return base->multibase2(); }

  chillAST_Node *getIndex(int dim);

  void gatherIndeces(std::vector<chillAST_Node *> &ind);

  void replaceChild(chillAST_Node *old, chillAST_Node *newchild); // will examine index

  // required methods that I can't seem to get to inherit
  void print(int indent = 0, FILE *fp = stderr);  // print CODE   in chill_ast.cc
  void printonly(int indent = 0, FILE *fp = stderr);

  void print(int indent = 0, FILE *fp = stderr) const;  // print CODE   in chill_ast.cc
  char *stringRep(int indent = 0);

  chillAST_Node *constantFold();

  chillAST_Node *clone();

  chillAST_Node *findref() { return this; }// find the SINGLE constant or data reference at this node or below
  void gatherArrayRefs(std::vector<chillAST_ArraySubscriptExpr *> &refs, bool writtento);

  void gatherScalarRefs(std::vector<chillAST_DeclRefExpr *> &refs, bool writtento);

  void gatherVarDecls(std::vector<chillAST_VarDecl *> &decls);

  void gatherVarDeclsMore(std::vector<chillAST_VarDecl *> &decls) { gatherVarDecls(decls); };

  void gatherScalarVarDecls(std::vector<chillAST_VarDecl *> &decls);

  void gatherArrayVarDecls(std::vector<chillAST_VarDecl *> &decls);

  void gatherVarUsage(std::vector<chillAST_VarDecl *> &decls);

  void gatherDeclRefExprs(std::vector<chillAST_DeclRefExpr *> &refs);

  void replaceVarDecls(chillAST_VarDecl *olddecl, chillAST_VarDecl *newdecl);

  bool findLoopIndexesToReplace(chillAST_SymbolTable *symtab,
                                bool forcesync = false) { return false; }; // no loops under here

  const char *getUnderlyingType() {
    //fprintf(stderr, "ASE getUnderlyingType() base of type %s\n", base->getTypeString()); base->print(); printf("\n"); fflush(stderr); 
    return base->getUnderlyingType();
  };

  virtual chillAST_VarDecl *getUnderlyingVarDecl() { return base->getUnderlyingVarDecl(); };

};

class chillAST_MemberExpr : public chillAST_Node {
public:
  virtual CHILLAST_NODE_TYPE getType(){return CHILLAST_NODE_MEMBEREXPR;}
  // variables that are special for this type of node
  chillAST_Node *base;  // always a decl ref expr? No, can be Array Subscript Expr
  char *member;
  char *printstring;

  chillAST_VarDecl *basedecl; // the vardecl that this refers to
  void *uniquePtr;  // DO NOT REFERENCE THROUGH THIS!

  CHILLAST_MEMBER_EXP_TYPE exptype;


  // constructors
  chillAST_MemberExpr(chillAST_Node *bas, const char *mem, void *unique,
                      CHILLAST_MEMBER_EXP_TYPE t = CHILLAST_MEMBER_EXP_DOT);

  // other methods particular to this type of node
  bool operator!=(const chillAST_MemberExpr &);

  bool operator==(const chillAST_MemberExpr &);

  // required methods that I can't seem to get to inherit
  void print(int indent = 0, FILE *fp = stderr);  // print CODE   in chill_ast.cc
  void printonly(int indent = 0, FILE *fp = stderr);

  void print(int indent = 0, FILE *fp = stderr) const;  // print CODE   in chill_ast.cc
  char *stringRep(int indent = 0);

  chillAST_Node *constantFold();

  chillAST_Node *clone();

  void gatherArrayRefs(std::vector<chillAST_ArraySubscriptExpr *> &refs, bool writtento);

  void gatherScalarRefs(std::vector<chillAST_DeclRefExpr *> &refs, bool writtento);

  void gatherVarDecls(std::vector<chillAST_VarDecl *> &decls);

  void gatherVarDeclsMore(std::vector<chillAST_VarDecl *> &decls) { gatherVarDecls(decls); };

  void gatherScalarVarDecls(std::vector<chillAST_VarDecl *> &decls);

  void gatherArrayVarDecls(std::vector<chillAST_VarDecl *> &decls);

  void gatherVarUsage(std::vector<chillAST_VarDecl *> &decls);

  void gatherDeclRefExprs(std::vector<chillAST_DeclRefExpr *> &refs);

  void replaceVarDecls(chillAST_VarDecl *olddecl, chillAST_VarDecl *newdecl);

  bool findLoopIndexesToReplace(chillAST_SymbolTable *symtab,
                                bool forcesync = false) { return false; }; // no loops under here

  chillAST_VarDecl *getUnderlyingVarDecl();

  void replaceChild(chillAST_Node *old, chillAST_Node *newchild);

  void setType(CHILLAST_MEMBER_EXP_TYPE t) { exptype = t; };

  CHILLAST_MEMBER_EXP_TYPE getType(CHILLAST_MEMBER_EXP_TYPE t) { return exptype; };

  chillAST_VarDecl *multibase();   // this one will return the member decl
  chillAST_Node *multibase2();  // this one will return the member expression
};

class chillAST_IntegerLiteral : public chillAST_Node {
public:
  virtual CHILLAST_NODE_TYPE getType(){return CHILLAST_NODE_INTEGERLITERAL;}
  // variables that are special for this type of node
  int value;

  // constructors
  chillAST_IntegerLiteral(int val);

  // other methods particular to this type of node
  int evalAsInt() { return value; }

  // required methods that I can't seem to get to inherit
  void print(int indent = 0, FILE *fp = stderr);  // print CODE   in chill_ast.cc
  chillAST_Node *constantFold();

  chillAST_Node *clone();

  void gatherArrayRefs(std::vector<chillAST_ArraySubscriptExpr *> &refs, bool w) {}; // does nothing
  void gatherScalarRefs(std::vector<chillAST_DeclRefExpr *> &refs, bool writtento) {}; // does nothing

  void gatherVarDecls(std::vector<chillAST_VarDecl *> &decls) {}; // does nothing
  void gatherVarDeclsMore(std::vector<chillAST_VarDecl *> &decls) { gatherVarDecls(decls); };

  void gatherScalarVarDecls(std::vector<chillAST_VarDecl *> &decls) {}; // does nothing
  void gatherArrayVarDecls(std::vector<chillAST_VarDecl *> &decls) {}; // does nothing

  void gatherVarUsage(std::vector<chillAST_VarDecl *> &decls) {}; // does nothing
  void gatherDeclRefExprs(std::vector<chillAST_DeclRefExpr *> &refs) {};  // does nothing
  void replaceVarDecls(chillAST_VarDecl *olddecl, chillAST_VarDecl *newdecl) {};

  bool findLoopIndexesToReplace(chillAST_SymbolTable *symtab,
                                bool forcesync = false) { return false; }; // no loops under here

  chillAST_Node *findref() { return this; }// find the SINGLE constant or data reference at this node or below
};

class chillAST_FloatingLiteral : public chillAST_Node {
public:
  virtual CHILLAST_NODE_TYPE getType(){return CHILLAST_NODE_FLOATINGLITERAL;}
  // variables that are special for this type of node
  float value;
  double doublevalue;
  int float0double1;

  char *allthedigits; // if not NULL, use this as printable representation
  //! Control the precision, float == 1, double == 2
  int precision;

  // constructors
  chillAST_FloatingLiteral(float val);

  chillAST_FloatingLiteral(double val);

  chillAST_FloatingLiteral(float val, int pre);

  chillAST_FloatingLiteral(double val, int pre);

  chillAST_FloatingLiteral(float val, const char *printable);

  chillAST_FloatingLiteral(float val, int pre, const char *printable);

  chillAST_FloatingLiteral(chillAST_FloatingLiteral *old);

  // other methods particular to this type of node
  void setPrecision(int precis) { precision = precis; };

  int getPrecision() { return precision; }

  // required methods that I can't seem to get to inherit
  void print(int indent = 0, FILE *fp = stderr);  // print CODE   in chill_ast.cc
  chillAST_Node *constantFold();

  chillAST_Node *clone();

  void gatherArrayRefs(std::vector<chillAST_ArraySubscriptExpr *> &refs, bool w) {}; // does nothing
  void gatherScalarRefs(std::vector<chillAST_DeclRefExpr *> &refs, bool writtento) {}; // does nothing

  void gatherVarDecls(std::vector<chillAST_VarDecl *> &decls) {}; // does nothing
  void gatherVarDeclsMore(std::vector<chillAST_VarDecl *> &decls) { gatherVarDecls(decls); };

  void gatherScalarVarDecls(std::vector<chillAST_VarDecl *> &decls) {}; // does nothing ;
  void gatherArrayVarDecls(std::vector<chillAST_VarDecl *> &decls) {}; // does nothing ;

  void gatherVarUsage(std::vector<chillAST_VarDecl *> &decls) {}; // does nothing
  void gatherDeclRefExprs(std::vector<chillAST_DeclRefExpr *> &refs) {}; // does nothing
  void replaceVarDecls(chillAST_VarDecl *olddecl, chillAST_VarDecl *newdecl) {};

  bool findLoopIndexesToReplace(chillAST_SymbolTable *symtab,
                                bool forcesync = false) { return false; }; // no loops under here
  chillAST_Node *findref() { return this; };// find the SINGLE constant or data reference at this node or below

};

class chillAST_UnaryOperator : public chillAST_Node {
public:
  virtual CHILLAST_NODE_TYPE getType(){return CHILLAST_NODE_UNARYOPERATOR;}
  virtual int getPrec();
  // variables that are special for this type of node
  //! String representing the operator
  char *op;
  //! true for prefix unary operator
  bool prefix;
  chillAST_Node *subexpr;

  // constructors
  chillAST_UnaryOperator(const char *oper, bool pre, chillAST_Node *sub);

  // other methods particular to this type of node
  bool isAssignmentOp() {
    return ((!strcmp(op, "++")) ||
            (!strcmp(op, "--")));   // are there more ???  TODO
  }

  // required methods that I can't seem to get to inherit
  void print(int indent = 0, FILE *fp = stderr);  // print CODE   in chill_ast.cc
  chillAST_Node *constantFold();

  chillAST_Node *clone();

  void gatherVarDecls(std::vector<chillAST_VarDecl *> &decls);

  void gatherVarDeclsMore(std::vector<chillAST_VarDecl *> &decls) { gatherVarDecls(decls); };

  void gatherScalarVarDecls(std::vector<chillAST_VarDecl *> &decls);

  void gatherArrayVarDecls(std::vector<chillAST_VarDecl *> &decls);

  void gatherArrayRefs(std::vector<chillAST_ArraySubscriptExpr *> &refs, bool writtento); // chillAST_UnaryOperator

  void gatherVarLHSUsage(std::vector<chillAST_VarDecl *> &decls);

  void replaceVarDecls(chillAST_VarDecl *olddecl, chillAST_VarDecl *newdecl);

  void gatherVarUsage(std::vector<chillAST_VarDecl *> &decls);

  void gatherDeclRefExprs(std::vector<chillAST_DeclRefExpr *> &refs);

  //void replaceVarDecls( chillAST_VarDecl *olddecl, chillAST_VarDecl *newdecl);
  bool findLoopIndexesToReplace(chillAST_SymbolTable *symtab,
                                bool forcesync = false) { return false; }; // no loops under here

  int evalAsInt();

};

class chillAST_ImplicitCastExpr : public chillAST_Node {
public:
  virtual CHILLAST_NODE_TYPE getType(){return CHILLAST_NODE_IMPLICITCASTEXPR;}
  // variables that are special for this type of node
  chillAST_Node *subexpr;

  // constructors
  chillAST_ImplicitCastExpr(chillAST_Node *sub);

  // other methods particular to this type of node
  bool isNotLeaf() { return true; };

  bool isLeaf() { return false; };

  // required methods that I can't seem to get to inherit
  void replaceChild(chillAST_Node *old, chillAST_Node *newchild);

  void print(int indent = 0, FILE *fp = stderr);  // print CODE   in chill_ast.cc
  void printonly(int indent = 0, FILE *fp = stderr);  // print CODE   in chill_ast.cc
  chillAST_Node *constantFold();

  chillAST_Node *clone();

  void gatherArrayRefs(std::vector<chillAST_ArraySubscriptExpr *> &refs, bool writtento);

  void gatherScalarRefs(std::vector<chillAST_DeclRefExpr *> &refs, bool writtento);

  void gatherVarDecls(std::vector<chillAST_VarDecl *> &decls);

  void gatherVarDeclsMore(std::vector<chillAST_VarDecl *> &decls) { gatherVarDecls(decls); };

  void gatherScalarVarDecls(std::vector<chillAST_VarDecl *> &decls);

  void gatherArrayVarDecls(std::vector<chillAST_VarDecl *> &decls);

  void gatherVarUsage(std::vector<chillAST_VarDecl *> &decls);

  void gatherDeclRefExprs(std::vector<chillAST_DeclRefExpr *> &refs);

  //void replaceVarDecls( chillAST_VarDecl *olddecl, chillAST_VarDecl *newdecl);
  bool findLoopIndexesToReplace(chillAST_SymbolTable *symtab,
                                bool forcesync = false) { return false; }; // no loops under here
  chillAST_VarDecl *multibase(); // just recurse on subexpr

};

class chillAST_CStyleCastExpr : public chillAST_Node {
public:
  virtual CHILLAST_NODE_TYPE getType(){return CHILLAST_NODE_CSTYLECASTEXPR;}
  virtual int getPrec() {return INT8_MAX+3;}
  // variables that are special for this type of node
  //! String representing the type it casts to
  char *towhat;
  chillAST_Node *subexpr;

  // constructors
  chillAST_CStyleCastExpr(const char *to, chillAST_Node *sub);

  // other methods particular to this type of node


  // required methods that I can't seem to get to inherit
  void replaceChild(chillAST_Node *old, chillAST_Node *newchild);

  void print(int indent = 0, FILE *fp = stderr);  // print CODE   in chill_ast.cc
  chillAST_Node *constantFold();

  chillAST_Node *clone();

  void gatherArrayRefs(std::vector<chillAST_ArraySubscriptExpr *> &refs, bool writtento);

  void gatherScalarRefs(std::vector<chillAST_DeclRefExpr *> &refs, bool writtento);

  void replaceVarDecls(chillAST_VarDecl *olddecl, chillAST_VarDecl *newdecl);

  void gatherVarDecls(std::vector<chillAST_VarDecl *> &decls);

  void gatherVarDeclsMore(std::vector<chillAST_VarDecl *> &decls) { gatherVarDecls(decls); };

  void gatherScalarVarDecls(std::vector<chillAST_VarDecl *> &decls);

  void gatherArrayVarDecls(std::vector<chillAST_VarDecl *> &decls);

  void gatherVarUsage(std::vector<chillAST_VarDecl *> &decls);

  void gatherDeclRefExprs(std::vector<chillAST_DeclRefExpr *> &refs);

  bool findLoopIndexesToReplace(chillAST_SymbolTable *symtab,
                                bool forcesync = false) { return false; }; // no loops under here
  chillAST_Node *findref() { return subexpr; };// find the SINGLE constant or data reference at this node or below

};

class chillAST_CStyleAddressOf : public chillAST_Node {
public:
  virtual CHILLAST_NODE_TYPE getType(){return CHILLAST_NODE_CSTYLEADDRESSOF;}
  virtual int getPrec() {return INT8_MAX+3;}
  // variables that are special for this type of node
  chillAST_Node *subexpr;

  // constructors
  chillAST_CStyleAddressOf(chillAST_Node *sub);

  // other methods particular to this type of node


  // required methods that I can't seem to get to inherit
  void print(int indent = 0, FILE *fp = stderr);  // print CODE   in chill_ast.cc
  chillAST_Node *constantFold();

  chillAST_Node *clone();

  void gatherArrayRefs(std::vector<chillAST_ArraySubscriptExpr *> &refs, bool writtento);

  void gatherScalarRefs(std::vector<chillAST_DeclRefExpr *> &refs, bool writtento);

  void gatherVarDecls(std::vector<chillAST_VarDecl *> &decls);

  void gatherVarDeclsMore(std::vector<chillAST_VarDecl *> &decls) { gatherVarDecls(decls); };

  void gatherScalarVarDecls(std::vector<chillAST_VarDecl *> &decls);

  void gatherArrayVarDecls(std::vector<chillAST_VarDecl *> &decls);

  void gatherVarUsage(std::vector<chillAST_VarDecl *> &decls);

  void gatherDeclRefExprs(std::vector<chillAST_DeclRefExpr *> &refs);

  //void replaceVarDecls( chillAST_VarDecl *olddecl, chillAST_VarDecl *newdecl);
  bool findLoopIndexesToReplace(chillAST_SymbolTable *symtab,
                                bool forcesync = false) { return false; }; // no loops under here


};

class chillAST_CudaMalloc : public chillAST_Node {
public:
  virtual CHILLAST_NODE_TYPE getType(){return CHILLAST_NODE_CUDAMALLOC;}
  // variables that are special for this type of node
  chillAST_Node *devPtr;  // Pointer to allocated device memory
  chillAST_Node *sizeinbytes;

  // constructors
  chillAST_CudaMalloc(chillAST_Node *devmemptr, chillAST_Node *size);

  // other methods particular to this type of node


  // required methods that I can't seem to get to inherit
  void print(int indent = 0, FILE *fp = stderr);  // print CODE   in chill_ast.cc
  chillAST_Node *constantFold();

  chillAST_Node *clone();

  void gatherArrayRefs(std::vector<chillAST_ArraySubscriptExpr *> &refs, bool writtento);

  void gatherScalarRefs(std::vector<chillAST_DeclRefExpr *> &refs, bool writtento);

  void gatherVarDecls(std::vector<chillAST_VarDecl *> &decls);

  void gatherVarDeclsMore(std::vector<chillAST_VarDecl *> &decls) { gatherVarDecls(decls); };

  void gatherScalarVarDecls(std::vector<chillAST_VarDecl *> &decls);

  void gatherArrayVarDecls(std::vector<chillAST_VarDecl *> &decls);

  void gatherVarUsage(std::vector<chillAST_VarDecl *> &decls);

  //void gatherDeclRefExprs( std::vector<chillAST_DeclRefExpr *>&refs );
  //void replaceVarDecls( chillAST_VarDecl *olddecl, chillAST_VarDecl *newdecl);
  bool findLoopIndexesToReplace(chillAST_SymbolTable *symtab,
                                bool forcesync = false) { return false; }; // no loops under here

};

class chillAST_CudaFree : public chillAST_Node {
public:
  virtual CHILLAST_NODE_TYPE getType(){return CHILLAST_NODE_CUDAFREE;}
  // variables that are special for this type of node
  chillAST_VarDecl *variable;

  // constructors
  chillAST_CudaFree(chillAST_VarDecl *var);

  // other methods particular to this type of node


  // required methods that I can't seem to get to inherit
  void print(int indent = 0, FILE *fp = stderr);  // print CODE   in chill_ast.cc
  chillAST_Node *constantFold();

  chillAST_Node *clone();

  void gatherArrayRefs(std::vector<chillAST_ArraySubscriptExpr *> &refs, bool writtento);

  void gatherScalarRefs(std::vector<chillAST_DeclRefExpr *> &refs, bool writtento);

  void gatherVarDecls(std::vector<chillAST_VarDecl *> &decls);

  void gatherVarDeclsMore(std::vector<chillAST_VarDecl *> &decls) { gatherVarDecls(decls); };

  void gatherScalarVarDecls(std::vector<chillAST_VarDecl *> &decls);

  void gatherArrayVarDecls(std::vector<chillAST_VarDecl *> &decls);

  void gatherVarUsage(std::vector<chillAST_VarDecl *> &decls);

  //void gatherDeclRefExprs( std::vector<chillAST_DeclRefExpr *>&refs );
  //void replaceVarDecls( chillAST_VarDecl *olddecl, chillAST_VarDecl *newdecl);
  bool findLoopIndexesToReplace(chillAST_SymbolTable *symtab,
                                bool forcesync = false) { return false; }; // no loops under here

};

class chillAST_Malloc : public chillAST_Node {   // malloc( sizeof(int) * 2048 );
public:
  virtual CHILLAST_NODE_TYPE getType(){return CHILLAST_NODE_MALLOC;}
  // variables that are special for this type of node
  //! to void if this is null  ,  sizeof(thing) if it is not
  char *thing;
  //! The subexpression calculating bytes
  chillAST_Node *sizeexpr; // bytes

  // constructors
  chillAST_Malloc(char *thething, chillAST_Node *numthings); // malloc (sizeof(int) *1024)

  // other methods particular to this type of node


  // required methods that I can't seem to get to inherit
  void print(int indent = 0, FILE *fp = stderr);  // print CODE   in chill_ast.cc
  chillAST_Node *constantFold();

  chillAST_Node *clone();

  void gatherArrayRefs(std::vector<chillAST_ArraySubscriptExpr *> &refs, bool writtento);

  void gatherScalarRefs(std::vector<chillAST_DeclRefExpr *> &refs, bool writtento);

  void gatherVarDecls(std::vector<chillAST_VarDecl *> &decls);

  void gatherVarDeclsMore(std::vector<chillAST_VarDecl *> &decls) { gatherVarDecls(decls); };

  void gatherScalarVarDecls(std::vector<chillAST_VarDecl *> &decls);

  void gatherArrayVarDecls(std::vector<chillAST_VarDecl *> &decls);

  void gatherVarUsage(std::vector<chillAST_VarDecl *> &decls);

  //void gatherDeclRefExprs( std::vector<chillAST_DeclRefExpr *>&refs );
  //void replaceVarDecls( chillAST_VarDecl *olddecl, chillAST_VarDecl *newdecl);
  bool findLoopIndexesToReplace(chillAST_SymbolTable *symtab,
                                bool forcesync = false) { return false; }; // no loops under here

};

class chillAST_Free : public chillAST_Node {
public:
  virtual CHILLAST_NODE_TYPE getType(){return CHILLAST_NODE_FREE;}

};

class chillAST_CudaMemcpy : public chillAST_Node {
public:
  virtual CHILLAST_NODE_TYPE getType(){return CHILLAST_NODE_CUDAMEMCPY;}
  // variables that are special for this type of node
  chillAST_VarDecl *dest;  // Pointer to allocated device memory 
  chillAST_VarDecl *src;
  chillAST_Node *size;
  char *cudaMemcpyKind;  // could use the actual enum

  // constructors
  chillAST_CudaMemcpy(chillAST_VarDecl *d, chillAST_VarDecl *s, chillAST_Node *size, char *kind);

  // other methods particular to this type of node


  // required methods that I can't seem to get to inherit
  void print(int indent = 0, FILE *fp = stderr);  // print CODE   in chill_ast.cc
  chillAST_Node *constantFold();

  chillAST_Node *clone();

  void gatherArrayRefs(std::vector<chillAST_ArraySubscriptExpr *> &refs, bool writtento);

  void gatherScalarRefs(std::vector<chillAST_DeclRefExpr *> &refs, bool writtento);

  void gatherVarDecls(std::vector<chillAST_VarDecl *> &decls);

  void gatherVarDeclsMore(std::vector<chillAST_VarDecl *> &decls) { gatherVarDecls(decls); };

  void gatherScalarVarDecls(std::vector<chillAST_VarDecl *> &decls);

  void gatherArrayVarDecls(std::vector<chillAST_VarDecl *> &decls);

  void gatherVarUsage(std::vector<chillAST_VarDecl *> &decls);

  //void gatherDeclRefExprs( std::vector<chillAST_DeclRefExpr *>&refs );
  //void replaceVarDecls( chillAST_VarDecl *olddecl, chillAST_VarDecl *newdecl);
  bool findLoopIndexesToReplace(chillAST_SymbolTable *symtab,
                                bool forcesync = false) { return false; }; // no loops under here

};

class chillAST_CudaSyncthreads : public chillAST_Node {
public:
  virtual CHILLAST_NODE_TYPE getType(){return CHILLAST_NODE_CUDASYNCTHREADS;}
  // variables that are special for this type of node

  // constructors
  chillAST_CudaSyncthreads();

  // other methods particular to this type of node


  // required methods that I can't seem to get to inherit
  void print(int indent = 0, FILE *fp = stderr);  // print CODE   in chill_ast.cc
  //chillAST_Node* constantFold() {};
  //chillAST_Node* clone();
  //void gatherArrayRefs( std::vector<chillAST_ArraySubscriptExpr*> &refs, bool writtento ){};
  //void gatherScalarRefs( std::vector<chillAST_DeclRefExpr*> &refs, bool writtento ) ;

  void gatherVarDecls(std::vector<chillAST_VarDecl *> &decls) {}; // does nothing
  void gatherVarDeclsMore(std::vector<chillAST_VarDecl *> &decls) { gatherVarDecls(decls); };

  void gatherScalarVarDecls(std::vector<chillAST_VarDecl *> &decls) {}; // does nothing
  void gatherArrayVarDecls(std::vector<chillAST_VarDecl *> &decls) {}; // does nothing

  void gatherVarUsage(std::vector<chillAST_VarDecl *> &decls) {}; // does nothing
  //void gatherDeclRefExprs( std::vector<chillAST_DeclRefExpr *>&refs );
  //void replaceVarDecls( chillAST_VarDecl *olddecl, chillAST_VarDecl *newdecl);
  //bool findLoopIndexesToReplace(  chillAST_SymbolTable *symtab, bool forcesync=false ){ return false; }; 

};

class chillAST_ReturnStmt : public chillAST_Node {
public:
  virtual CHILLAST_NODE_TYPE getType(){return CHILLAST_NODE_RETURNSTMT;}
  // variables that are special for this type of node
  //! expression to return
  chillAST_Node *returnvalue;

  // constructors
  chillAST_ReturnStmt(chillAST_Node *retval);

  // other methods particular to this type of node


  // required methods that I can't seem to get to inherit
  void print(int indent = 0, FILE *fp = stderr);  // print CODE   in chill_ast.cc
  chillAST_Node *constantFold();

  chillAST_Node *clone();

  void gatherVarDecls(std::vector<chillAST_VarDecl *> &decls);

  void gatherVarDeclsMore(std::vector<chillAST_VarDecl *> &decls) { gatherVarDecls(decls); };

  void gatherScalarVarDecls(std::vector<chillAST_VarDecl *> &decls);

  void gatherArrayVarDecls(std::vector<chillAST_VarDecl *> &decls);

  void gatherVarUsage(std::vector<chillAST_VarDecl *> &decls);

  void gatherDeclRefExprs(std::vector<chillAST_DeclRefExpr *> &refs);

  //void replaceVarDecls( chillAST_VarDecl *olddecl, chillAST_VarDecl *newdecl);
  bool findLoopIndexesToReplace(chillAST_SymbolTable *symtab,
                                bool forcesync = false) { return false; }; // no loops under here

};

class chillAST_CallExpr : public chillAST_Node {  // a function call
public:
  virtual CHILLAST_NODE_TYPE getType(){return CHILLAST_NODE_CALLEXPR;}
  // variables that are special for this type of node
  chillAST_Node *callee;   // the function declaration (what about builtins?)
  int numargs;
  std::vector<class chillAST_Node *> args;
  chillAST_VarDecl *grid;
  chillAST_VarDecl *block;

  // constructors
  chillAST_CallExpr(chillAST_Node *function);

  void addArg(chillAST_Node *newarg);

  // other methods particular to this type of node
  // TODO get/set grid, block

  // required methods that I can't seem to get to inherit
  chillAST_Node *constantFold();

  void print(int indent = 0, FILE *fp = stderr);  // print CODE   in chill_ast.cc
  void gatherArrayRefs(std::vector<chillAST_ArraySubscriptExpr *> &refs, bool writtento);

  void gatherScalarRefs(std::vector<chillAST_DeclRefExpr *> &refs, bool writtento);

  void gatherVarDecls(std::vector<chillAST_VarDecl *> &decls);

  void gatherVarDeclsMore(std::vector<chillAST_VarDecl *> &decls) { gatherVarDecls(decls); };

  void gatherScalarVarDecls(std::vector<chillAST_VarDecl *> &decls);

  void gatherArrayVarDecls(std::vector<chillAST_VarDecl *> &decls);

  void gatherVarUsage(std::vector<chillAST_VarDecl *> &decls);

  void gatherDeclRefExprs(std::vector<chillAST_DeclRefExpr *> &refs);

  void replaceVarDecls(chillAST_VarDecl *olddecl, chillAST_VarDecl *newdecl);

  bool findLoopIndexesToReplace(chillAST_SymbolTable *symtab,
                                bool forcesync = false) { return false; }; // no loops under here
  chillAST_Node *clone();
};

class chillAST_ParenExpr : public chillAST_Node {
public:
  virtual CHILLAST_NODE_TYPE getType(){return CHILLAST_NODE_PARENEXPR;}
  // variables that are special for this type of node
  chillAST_Node *subexpr;

  // constructors
  chillAST_ParenExpr(chillAST_Node *sub);

  // other methods particular to this type of node


  // required methods that I can't seem to get to inherit
  void print(int indent = 0, FILE *fp = stderr);  // print CODE   in chill_ast.cc
  chillAST_Node *constantFold();

  chillAST_Node *clone();

  void gatherArrayRefs(std::vector<chillAST_ArraySubscriptExpr *> &refs, bool writtento);

  void gatherScalarRefs(std::vector<chillAST_DeclRefExpr *> &refs, bool writtento);

  void gatherVarDecls(std::vector<chillAST_VarDecl *> &decls);

  void gatherVarDeclsMore(std::vector<chillAST_VarDecl *> &decls) { gatherVarDecls(decls); };

  void gatherScalarVarDecls(std::vector<chillAST_VarDecl *> &decls);

  void gatherArrayVarDecls(std::vector<chillAST_VarDecl *> &decls);

  void gatherVarUsage(std::vector<chillAST_VarDecl *> &decls);

  void gatherDeclRefExprs(std::vector<chillAST_DeclRefExpr *> &refs);

  void replaceVarDecls(chillAST_VarDecl *olddecl, chillAST_VarDecl *newdecl);

  bool findLoopIndexesToReplace(chillAST_SymbolTable *symtab,
                                bool forcesync = false) { return false; }; // no loops under here

};

class chillAST_Sizeof : public chillAST_Node {
public:
  virtual CHILLAST_NODE_TYPE getType(){return CHILLAST_NODE_SIZEOF;}
  virtual int getPrec() {return INT8_MAX+3;}
  // variables that are special for this type of node
  //! the object of sizeof function
  char *thing;

  // constructors
  chillAST_Sizeof(char *t);

  // other methods particular to this type of node


  // required methods that I can't seem to get to inherit
  void print(int indent = 0, FILE *fp = stderr);  // print CODE   in chill_ast.cc
  chillAST_Node *constantFold();

  chillAST_Node *clone();

  void gatherArrayRefs(std::vector<chillAST_ArraySubscriptExpr *> &refs, bool writtento);

  void gatherScalarRefs(std::vector<chillAST_DeclRefExpr *> &refs, bool writtento);

  void gatherVarDecls(std::vector<chillAST_VarDecl *> &decls);

  void gatherVarDeclsMore(std::vector<chillAST_VarDecl *> &decls) { gatherVarDecls(decls); };

  void gatherScalarVarDecls(std::vector<chillAST_VarDecl *> &decls);

  void gatherArrayVarDecls(std::vector<chillAST_VarDecl *> &decls);

  void gatherVarUsage(std::vector<chillAST_VarDecl *> &decls);

  void gatherDeclRefExprs(std::vector<chillAST_DeclRefExpr *> &refs);

  void replaceVarDecls(chillAST_VarDecl *olddecl, chillAST_VarDecl *newdecl) {};

  bool findLoopIndexesToReplace(chillAST_SymbolTable *symtab,
                                bool forcesync = false) { return false; }; // no loops under here

};

class chillAST_NoOp : public chillAST_Node {
public:
  virtual CHILLAST_NODE_TYPE getType(){return CHILLAST_NODE_NOOP;}
  chillAST_NoOp(); //  { parent = p; };

  // required methods that I can't seem to get to inherit
  void print(int indent = 0, FILE *fp = stderr) {};  // print CODE   in chill_ast.cc
  chillAST_Node *constantFold() {};

  chillAST_Node *clone() { chillAST_Node* n = new chillAST_NoOp(); n->setParent(parent); return n; }; // ??

  void gatherArrayRefs(std::vector<chillAST_ArraySubscriptExpr *> &refs, bool writtento) {};

  void gatherScalarRefs(std::vector<chillAST_DeclRefExpr *> &refs, bool writtento) {};

  void gatherVarDecls(std::vector<chillAST_VarDecl *> &decls) {};

  void gatherVarDeclsMore(std::vector<chillAST_VarDecl *> &decls) { gatherVarDecls(decls); };

  void gatherScalarVarDecls(std::vector<chillAST_VarDecl *> &decls) {};

  void gatherArrayVarDecls(std::vector<chillAST_VarDecl *> &decls) {};

  void gatherVarUsage(std::vector<chillAST_VarDecl *> &decls) {};

  void gatherDeclRefExprs(std::vector<chillAST_DeclRefExpr *> &refs) {};

  void replaceVarDecls(chillAST_VarDecl *olddecl, chillAST_VarDecl *newdecl) {};

  bool findLoopIndexesToReplace(chillAST_SymbolTable *symtab,
                                bool forcesync = false) { return false; };//no loops under here
};

class chillAST_IfStmt : public chillAST_Node {
public:
  virtual CHILLAST_NODE_TYPE getType(){return CHILLAST_NODE_IFSTMT;}
  // variables that are special for this type of node
  chillAST_Node *cond;
  chillAST_Node *thenpart;
  chillAST_Node *elsepart;
  IR_CONDITION_TYPE conditionoperator;  // from ir_code.hh

  // constructors
  chillAST_IfStmt();

  chillAST_IfStmt(chillAST_Node *c, chillAST_Node *t, chillAST_Node *e);

  // other methods particular to this type of node
  chillAST_Node *getCond() { return cond; };

  chillAST_Node *getThen() { return thenpart; };

  chillAST_Node *getElse() { return elsepart; };

  void setCond(chillAST_Node *b) {
    cond = b;
    if (cond) cond->parent = this;
  };

  void setThen(chillAST_Node *b) {
    thenpart = b;
    if (thenpart) thenpart->parent = this;
  };

  void setElse(chillAST_Node *b) {
    elsepart = b;
    if (elsepart) elsepart->parent = this;
  };

  // required methods that I can't seem to get to inherit
  void print(int indent = 0, FILE *fp = stderr);

  chillAST_Node *constantFold();

  chillAST_Node *clone();

  void gatherVarDecls(std::vector<chillAST_VarDecl *> &decls);

  void gatherVarDeclsMore(std::vector<chillAST_VarDecl *> &decls) { gatherVarDecls(decls); };

  void gatherScalarVarDecls(std::vector<chillAST_VarDecl *> &decls);

  void gatherArrayVarDecls(std::vector<chillAST_VarDecl *> &decls);

  void gatherArrayRefs(std::vector<chillAST_ArraySubscriptExpr *> &refs, bool writtento);

  void gatherScalarRefs(std::vector<chillAST_DeclRefExpr *> &refs, bool writtento);

  void gatherVarUsage(std::vector<chillAST_VarDecl *> &decls);

  void gatherDeclRefExprs(std::vector<chillAST_DeclRefExpr *> &refs);

  //void replaceVarDecls( chillAST_VarDecl *olddecl, chillAST_VarDecl *newdecl);
  bool findLoopIndexesToReplace(chillAST_SymbolTable *symtab, bool forcesync = false);

  void gatherStatements(std::vector<chillAST_Node *> &statements);

};

class chillAST_something : public chillAST_Node {
public:
  virtual CHILLAST_NODE_TYPE getType(){return CHILLAST_NODE_UNKNOWN;}
  // variables that are special for this type of node

  // constructors
  chillAST_something();

  // other methods particular to this type of node


  // required methods that I can't seem to get to inherit
  void print(int indent = 0, FILE *fp = stderr);  // print CODE   in chill_ast.cc
  //void dump(  int indent=0,  FILE *fp = stderr );  // print ast    in chill_ast.cc
};


#endif

