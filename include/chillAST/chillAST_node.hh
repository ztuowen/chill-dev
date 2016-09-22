

#ifndef _CHILLAST_NODE_H_
#define _CHILLAST_NODE_H_

#include "chillAST_def.hh"

// an actual chill ast. 
// nodes based on clang AST which are in turn based on C++ 

class chillAST_node {   // generic node. a tree of these is the AST. this is virtual (can't instantiate)
public: 

  static int chill_scalar_counter;   // for manufactured scalars 
  static int chill_array_counter ;   // for manufactured arrays
  static int chill_pointer_counter ; // for manufactured arrays


  CHILL_ASTNODE_TYPE asttype;
  
  bool isSourceFile()         { return (asttype == CHILLAST_NODETYPE_SOURCEFILE); };
  bool isTypeDefDecl()        { return (asttype == CHILLAST_NODETYPE_TYPEDEFDECL); };
  bool isVarDecl()            { return (asttype == CHILLAST_NODETYPE_VARDECL); }; 
  bool isFunctionDecl()       { return (asttype == CHILLAST_NODETYPE_FUNCTIONDECL); }; 
  bool isRecordDecl()         { return (asttype == CHILLAST_NODETYPE_RECORDDECL); }; 
  bool isMacroDefinition()    { return (asttype == CHILLAST_NODETYPE_MACRODEFINITION); }; 
  bool isCompoundStmt()       { return (asttype == CHILLAST_NODETYPE_COMPOUNDSTMT); }; 
  bool isLoop()               { return (asttype == CHILLAST_NODETYPE_LOOP); };    // AKA ForStmt
  bool isForStmt()            { return (asttype == CHILLAST_NODETYPE_LOOP); };    // AKA Loop
  bool isIfStmt()             { return (asttype == CHILLAST_NODETYPE_IFSTMT); }; 
  bool isTernaryOperator()    { return (asttype == CHILLAST_NODETYPE_TERNARYOPERATOR);}; 
  bool isBinaryOperator()     { return (asttype == CHILLAST_NODETYPE_BINARYOPERATOR); }; 
  bool isUnaryOperator()      { return (asttype == CHILLAST_NODETYPE_UNARYOPERATOR); }; 
  bool isArraySubscriptExpr() { return (asttype == CHILLAST_NODETYPE_ARRAYSUBSCRIPTEXPR); }; 
  bool isMemberExpr()         { return (asttype == CHILLAST_NODETYPE_MEMBEREXPR); }; 
  bool isDeclRefExpr()        { return (asttype == CHILLAST_NODETYPE_DECLREFEXPR); }; 
  bool isIntegerLiteral()     { return (asttype == CHILLAST_NODETYPE_INTEGERLITERAL); }; 
  bool isFloatingLiteral()    { return (asttype == CHILLAST_NODETYPE_FLOATINGLITERAL); }; 
  bool isImplicitCastExpr()   { return (asttype == CHILLAST_NODETYPE_IMPLICITCASTEXPR); }; 
  bool isReturnStmt()         { return (asttype == CHILLAST_NODETYPE_RETURNSTMT); }; 
  bool isCallExpr()           { return (asttype == CHILLAST_NODETYPE_CALLEXPR); }; 
  bool isParenExpr()          { return (asttype == CHILLAST_NODETYPE_PARENEXPR); }; 
  bool isSizeof()             { return (asttype == CHILLAST_NODETYPE_SIZEOF); }; 
  bool isMalloc()             { return (asttype == CHILLAST_NODETYPE_MALLOC); }; 
  bool isFree()               { return (asttype == CHILLAST_NODETYPE_FREE); }; 
  bool isPreprocessing()      { return (asttype == CHILLAST_NODETYPE_PREPROCESSING); }; 
  bool isNoOp()               { return (asttype == CHILLAST_NODETYPE_NOOP); }; 
  bool isNull()               { return (asttype == CHILLAST_NODETYPE_NULL); }; 
  bool isCStyleCastExpr()     { return (asttype == CHILLAST_NODETYPE_CSTYLECASTEXPR); }; 
  bool isCStyleAddressOf()    { return (asttype == CHILLAST_NODETYPE_CSTYLEADDRESSOF); }; 
  bool isCudaMalloc()         { return (asttype == CHILLAST_NODETYPE_CUDAMALLOC); }; 
  bool isCudaFree()           { return (asttype == CHILLAST_NODETYPE_CUDAFREE); }; 
  bool isCudaMemcpy()         { return (asttype == CHILLAST_NODETYPE_CUDAMEMCPY); }; 
  bool isCudaKERNELCALL()     { return (asttype == CHILLAST_NODETYPE_CUDAKERNELCALL); }; 
  bool isCudaSYNCTHREADS()    { return (asttype == CHILLAST_NODETYPE_CUDASYNCTHREADS); }; 

  bool isDeclStmt()           { return (asttype == CHILLAST_NODETYPE_DECLSTMT); }; // doesn't exist
  
  bool isConstant()           { return (asttype == CHILLAST_NODETYPE_INTEGERLITERAL) || (asttype == CHILLAST_NODETYPE_FLOATINGLITERAL); } 
    

  virtual bool isAssignmentOp() { return false; }; 
  virtual bool isComparisonOp() { return false; }; 
  virtual bool isNotLeaf()      { return false; };
  virtual bool isLeaf()         { return true;  };
  virtual bool isParmVarDecl()  { return false; };  

  virtual bool isPlusOp()       { return false; }; 
  virtual bool isMinusOp()      { return false; }; 
  virtual bool isPlusMinusOp()  { return false; }; 
  virtual bool isMultDivOp()    { return false; }; 

  virtual bool isAStruct() { return false; }; 
  virtual bool isAUnion()  { return false; };

  virtual bool hasSymbolTable() { return false; } ; // most nodes do NOT have a symbol table
  virtual bool hasTypedefTable() { return false; } ; // most nodes do NOT have a typedef table
  virtual chillAST_SymbolTable *getSymbolTable() { return NULL; } // most nodes do NOT have a symbol table

  virtual chillAST_VarDecl *findVariableNamed( const char *name ); // recursive 

  chillAST_RecordDecl *findRecordDeclNamed( const char *name ); // recursive
  
  // void addDecl( chillAST_VarDecl *vd); // recursive, adds to first  symbol table it can find 

  // TODO decide how to hide some data
  chillAST_node *parent; 
  bool isFromSourceFile;  // false = #included 
  char *filename;  // file this node is from

  void segfault() { fprintf(stderr, "segfaulting on purpose\n"); int *i=0; int j = i[0]; }; // seg fault
  int getNumChildren() { return children.size(); }; 
  std::vector<chillAST_node*> children; 
  std::vector<chillAST_node*> getChildren() { return children; } ;  // not usually useful
  void                   setChildren( std::vector<chillAST_node*>&c ) { children = c; } ; // does not set parent. probably should
  chillAST_node *getChild( int which)                    { return children[which]; };
  void           setChild( int which, chillAST_node *n ) { children[which] = n; children[which]->parent = this; } ;
  
  char *metacomment; // for compiler internals, formerly a comment
  void setMetaComment( char *c ) { metacomment = strdup(c); }; 

  std::vector<chillAST_Preprocessing*> preprocessinginfo; 

  virtual void addChild( chillAST_node* c) {
    //if (c->isFunctionDecl()) fprintf(stderr, "addchild FunctionDecl\n"); 
    c->parent = this;
    // check to see if it's already there
    for (int i=0; i<children.size(); i++) { 
      if (c == children[i]) {
        //fprintf(stderr, "addchild ALREADY THERE\n"); 
        return; // already there
      }
    }
    children.push_back(c);
  } ;  // not usually useful

  virtual void insertChild(int i, chillAST_node* node) { 
    //fprintf(stderr, "%s inserting child of type %s at location %d\n", getTypeString(), node->getTypeString(), i); 
    node->parent = this; 
    children.insert( children.begin()+i, node );
  };
  
  virtual void removeChild(int i) { 
    children.erase( children.begin()+i );
  };
  
  int findChild(  chillAST_node *c )  {   
    for (int i=0; i<children.size(); i++) { 
      if (children[i] == c) return i;
    }
    return -1;
  }

  virtual void replaceChild( chillAST_node *old, chillAST_node *newchild ) { 
    fprintf(stderr,"(%s) forgot to implement replaceChild() ... using generic\n" ,Chill_AST_Node_Names[asttype]);
    fprintf(stderr, "%d children\n", children.size());
    for (int i=0; i<children.size(); i++) { 
      if (children[i] == old) { 
        children[i] = newchild;
        newchild->setParent( this );
        return; 
      }
    }
    fprintf(stderr, "%s %p generic replaceChild called with oldchild that was not a child\n",
            getTypeString(), this) ;
    fprintf(stderr, "printing\n"); 
    print(); fprintf(stderr, "\nchild: ");
    if (!old) fprintf(stderr, "oldchild NULL!\n");
    old->print(); fprintf(stderr, "\nnew: "); 
    newchild->print(); fprintf(stderr, "\n"); 
    segfault(); // make easier for gdb
  };
  
  virtual void loseLoopWithLoopVar( char *var ) { 
    // walk tree. If a loop has this loop variable, replace the loop with the loop body, 
    // removing the loop.  The loop will be spread across a bunch of cores that will each
    // calculate their own loop variable.

    // things that can not have loops as substatements can have a null version of this method
    // things that have more complicated sets of "children" will have specialized versions

    // this is the generic version of the method. It just recurses among its children.
    // ForStmt is the only one that can actually remove itself. When it does, it will 
    // potentially change the children vector, which is not the simple array it might appear.
    // so you have to make a copy of the vector to traverse
    
    std::vector<chillAST_node*> dupe = children; // simple enough?
    //fprintf(stderr, "node XXX has %d children\n", dupe.size()); 
    //fprintf(stderr, "generic node %s has %d children\n", getTypeString(), dupe.size()); 
    for (int i=0; i<dupe.size(); i++) {  // recurse on all children
      dupe[i]->loseLoopWithLoopVar( var );
    }
  }

  virtual int evalAsInt() { 
    fprintf(stderr,"(%s) can't be evaluated as an integer??\n", Chill_AST_Node_Names[asttype]);
    print(); fprintf(stderr, "\n"); 
    segfault(); 
  }

  virtual const char* getUnderlyingType() { 
    fprintf(stderr,"(%s) forgot to implement getUnderlyingType()\n", Chill_AST_Node_Names[asttype]);
    dump();
    print();
    fprintf(stderr, "\n\n"); 
    segfault(); 
  }; 

  virtual chillAST_VarDecl* getUnderlyingVarDecl() { 
    fprintf(stderr,"(%s) forgot to implement getUnderlyingVarDecl()\n", Chill_AST_Node_Names[asttype]);
    dump();
    print();
    fprintf(stderr, "\n\n"); 
    segfault();
  }; 


  virtual chillAST_node *findref(){// find the SINGLE constant or data reference at this node or below
    fprintf(stderr,"(%s) forgot to implement findref()\n" ,Chill_AST_Node_Names[asttype]); 
    dump();
    print();
    fprintf(stderr, "\n\n"); 
    segfault();
  };

  virtual void gatherArrayRefs( std::vector<chillAST_ArraySubscriptExpr*> &refs, bool writtento ) {
    fprintf(stderr,"(%s) forgot to implement gatherArrayRefs()\n" ,Chill_AST_Node_Names[asttype]); 
    dump();
    print();
    fprintf(stderr, "\n\n"); 
  };
 
  // TODO we MIGHT want the VarDecl // NOTHING IMPLEMENTS THIS? ??? 
  virtual void gatherScalarRefs( std::vector<chillAST_DeclRefExpr*> &refs, bool writtento ) {
    fprintf(stderr,"(%s) forgot to implement gatherScalarRefs()\n" ,Chill_AST_Node_Names[asttype]); 
    dump();
    print();
    fprintf(stderr, "\n\n"); 
  };
 
  virtual void gatherLoopIndeces( std::vector<chillAST_VarDecl*> &indeces ) { // recursive walk parent links, looking for loops, and grabbing the declRefExpr in the loop init and cond. 
    // you can quit when you get to certain nodes

    //fprintf(stderr, "%s::gatherLoopIndeces()\n", getTypeString()); 
    
    if (isSourceFile() || isFunctionDecl() ) return; // end of the line

    // just for debugging 
    //if (parent) {
    //  fprintf(stderr, "%s has parent of type %s\n", getTypeString(), parent->getTypeString()); 
    //} 
    //else fprintf(stderr, "this %s %p has no parent???\n", getTypeString(), this);


    if (!parent) return; // should not happen, but be careful

    // for most nodes, this just recurses upwards
    //fprintf(stderr, "%s::gatherLoopIndeces() %p recursing up\n", this); 
    parent->gatherLoopIndeces( indeces );
  }


  chillAST_ForStmt* findContainingLoop() { // recursive walk parent links, looking for loops
    //fprintf(stderr, "%s::findContainingLoop()   ", getTypeString()); 
    //if (parent) fprintf(stderr, "parents is a %s\n", parent->getTypeString()); 
    //else fprintf(stderr, "no parent\n"); 
    // do not check SELF type, as we may want to find the loop containing a loop
    if (!parent) return NULL;
    if (parent->isForStmt()) return (chillAST_ForStmt*)parent;
    return parent->findContainingLoop(); // recurse upwards
  }

  chillAST_node* findContainingNonLoop() { // recursive walk parent links, avoiding loops
    fprintf(stderr, "%s::findContainingNonLoop()   ", getTypeString()); 
    //if (parent) fprintf(stderr, "parent is a %s\n", parent->getTypeString()); 
    //else fprintf(stderr, "no parent\n"); 
    // do not check SELF type, as we may want to find the loop containing a loop
    if (!parent) return NULL;
    if (parent->isCompoundStmt() && parent->getParent()->isForStmt()) return parent->getParent()->findContainingNonLoop(); // keep recursing
    if (parent->isForStmt()) return parent->findContainingNonLoop(); // keep recursing
    return (chillAST_node*)parent; // return non-loop 
  }

  // TODO gather loop init and cond (and if cond) like gatherloopindeces

  virtual void gatherDeclRefExprs( std::vector<chillAST_DeclRefExpr *>&refs ){  // both scalar and arrays
    fprintf(stderr,"(%s) forgot to implement gatherDeclRefExpr()\n" ,Chill_AST_Node_Names[asttype]); 
  };



  virtual void gatherVarUsage( std::vector<chillAST_VarDecl*> &decls ) { 
    fprintf(stderr,"(%s) forgot to implement gatherVarUsage()\n" ,Chill_AST_Node_Names[asttype]); 
  }; 

  virtual void gatherVarLHSUsage( std::vector<chillAST_VarDecl*> &decls ) { 
    fprintf(stderr,"(%s) forgot to implement gatherVarLHSUsage()\n" ,Chill_AST_Node_Names[asttype]); 
  }; 


  virtual void gatherVarDecls( std::vector<chillAST_VarDecl*> &decls ) {  // ACTUAL Declaration
    fprintf(stderr,"(%s) forgot to implement gatherVarDecls()\n" ,Chill_AST_Node_Names[asttype]); 
  }; 

  
  virtual void gatherVarDeclsMore( std::vector<chillAST_VarDecl*> &decls ) {  // even if the decl itself is not in the ast. 
    fprintf(stderr,"(%s) forgot to implement gatherVarDeclsMore()\n" ,Chill_AST_Node_Names[asttype]); 
  }; 

  virtual void gatherScalarVarDecls( std::vector<chillAST_VarDecl*> &decls ) {  // ACTUAL Declaration
    fprintf(stderr,"(%s) forgot to implement gatherScalarVarDecls()\n" ,Chill_AST_Node_Names[asttype]); 
  }; 

  virtual void gatherArrayVarDecls( std::vector<chillAST_VarDecl*> &decls ) {  // ACTUAL Declaration
    fprintf(stderr,"(%s) forgot to implement gatherArrayVarDecls()\n" ,Chill_AST_Node_Names[asttype]); 
  }; 

  virtual chillAST_VarDecl *findArrayDecl( const char *name ) { // scoping TODO 
    if (!hasSymbolTable()) return parent->findArrayDecl( name ); // most things
    else
      fprintf(stderr,"(%s) forgot to implement gatherArrayVarDecls()\n" ,Chill_AST_Node_Names[asttype]);
  }


  virtual void replaceVarDecls( chillAST_VarDecl *olddecl, chillAST_VarDecl *newdecl) { 
    fprintf(stderr,"(%s) forgot to implement replaceVarDecls()\n" ,Chill_AST_Node_Names[asttype]); 
  }; 

  virtual bool findLoopIndexesToReplace( chillAST_SymbolTable *symtab, bool forcesync=false ) { 
    // this just looks for ForStmts with preferred index metacomment attached 
    fprintf(stderr,"(%s) forgot to implement findLoopIndexesToReplace()\n" ,Chill_AST_Node_Names[asttype]); 
    return false; 
  }

  
  virtual chillAST_node* constantFold() {  // hacky. TODO. make nice
    fprintf(stderr,"(%s) forgot to implement constantFold()\n" ,Chill_AST_Node_Names[asttype]); 
    exit(-1); ; 
  };

  virtual chillAST_node* clone() {   // makes a deep COPY (?)
    fprintf(stderr,"(%s) forgot to implement clone()\n" ,Chill_AST_Node_Names[asttype]); 
    exit(-1); ; 
  };
  virtual void dump(  int indent=0,  FILE *fp = stderr ) { 
    fflush(fp); 
    fprintf(fp,"(%s) forgot to implement dump()\n" ,Chill_AST_Node_Names[asttype]); };// print ast
  
  virtual void print( int indent=0,  FILE *fp = stderr ) { 
    fflush(fp); 
    //fprintf(stderr, "generic chillAST_node::print() called!\n"); 
    //fprintf(stderr, "asttype is %d\n", asttype); 
    fprintf(fp, "\n");
    chillindent(indent, fp); 
    fprintf(fp,"(%s) forgot to implement print()\n" ,Chill_AST_Node_Names[asttype]); 
  };// print CODE 
  
  virtual void printName( int indent=0,  FILE *fp = stderr ) { 
    fflush(fp); 
    //fprintf(stderr, "generic chillAST_node::printName() called!\n"); 
    //fprintf(stderr, "asttype is %d\n", asttype); 
    fprintf(fp, "\n");
    chillindent(indent, fp); 
    fprintf(fp,"(%s) forgot to implement printName()\n" ,Chill_AST_Node_Names[asttype]); 
  };// print CODE 

  virtual char *stringRep(int indent=0 ) {  // the ast's print version
    fflush(stdout);
    // chillindent(indent, fp);  TODO 
    fprintf(stderr,"(%s) forgot to implement stringRep()\n" ,Chill_AST_Node_Names[asttype]);
    segfault(); 
  }


  virtual void printonly( int indent=0,  FILE *fp = stderr ) { print( indent, fp); }; 

  //virtual void printString( std::string &s ) { 
  //  fprintf(stderr,"(%s) forgot to implement printString()\n" ,Chill_AST_Node_Names[asttype]);
  //}


  virtual void get_top_level_loops( std::vector<chillAST_ForStmt *> &loops) {
    int n = children.size(); 
    //fprintf(stderr, "get_top_level_loops of a %s with %d children\n", getTypeString(), n); 
    for (int i=0; i<n; i++) { 
      //fprintf(stderr, "child %d is a %s\n", i, children[i]->getTypeString()); 
      if (children[i]->isForStmt()) {
        loops.push_back( ((chillAST_ForStmt *)(children[i])) );
      }
    }
    //fprintf(stderr, "found %d top level loops\n", loops.size()); 
  }


  virtual void repairParentChild() {  // for nodes where all subnodes are children
    int n = children.size(); 
    for (int i=0; i<n; i++) { 
      if (children[i]->parent != this) { 
        fprintf(stderr, "fixing child %s that didn't know its parent\n", children[i]->getTypeString()); 
        children[i]->parent = this; 
      }
    }
  }



  virtual void get_deep_loops( std::vector<chillAST_ForStmt *> &loops) { // this is probably broken - returns ALL loops under it
    int n = children.size(); 
    //fprintf(stderr, "get_deep_loops of a %s with %d children\n", getTypeString(), n); 
    for (int i=0; i<n; i++) { 
      //fprintf(stderr, "child %d is a %s\n", i, children[i]->getTypeString()); 
      children[i]->get_deep_loops( loops ); 
    }
    //fprintf(stderr, "found %d deep loops\n", loops.size()); 
  }


  // generic for chillAST_node with children
  virtual void find_deepest_loops( std::vector<chillAST_ForStmt *> &loops) { // returns DEEPEST nesting of loops 
    std::vector<chillAST_ForStmt *>deepest; // deepest below here 
    
    int n = children.size(); 
    //fprintf(stderr, "find_deepest_loops of a %s with %d children\n", getTypeString(), n); 
    for (int i=0; i<n; i++) { 
      std::vector<chillAST_ForStmt *> subloops;  // loops below here among a child of mine 
      
      //fprintf(stderr, "child %d is a %s\n", i, children[i]->getTypeString()); 
      children[i]->find_deepest_loops( subloops );
      
      if (subloops.size() > deepest.size()) { 
        deepest = subloops;
      }
    }
    
    // append deepest we see at this level to loops 
    for ( int i=0; i<deepest.size(); i++) { 
      loops.push_back( deepest[i] );
    }

    //fprintf(stderr, "found %d deep loops\n", loops.size()); 
    
  }




  const char *getTypeString() { return Chill_AST_Node_Names[asttype]; } ; 
  int  getType() { return asttype; }; 
  void setParent( chillAST_node *p) { parent = p; } ;
  chillAST_node  *getParent() { return parent; } ;
  
  chillAST_SourceFile *getSourceFile() { 
    if (isSourceFile()) return ((chillAST_SourceFile *)this);
    if (parent != NULL) return parent->getSourceFile(); 
    fprintf(stderr, "UHOH, getSourceFile() called on node %p %s that does not have a parent and is not a source file\n", this, this->getTypeString());
    this->print(); printf("\n\n"); fflush(stdout); 
    exit(-1);
  }
  
  virtual chillAST_node *findDatatype( char *t ) { 
    fprintf(stderr, "%s looking for datatype %s\n", getTypeString(), t); 
    if (parent != NULL) return parent->findDatatype(t); // most nodes do this
    return NULL; 
  }


  virtual chillAST_SymbolTable *addVariableToSymbolTable( chillAST_VarDecl *vd ) { 
    if (!parent) { 
      fprintf(stderr, "%s with no parent addVariableToSymbolTable()\n", getTypeString()); 
      exit(-1);
    }
    //fprintf(stderr, "%s::addVariableToSymbolTable() (default) headed up\n",  getTypeString()); 
    return parent->addVariableToSymbolTable( vd ); // default, defer to parent 
  }

  virtual void addTypedefToTypedefTable( chillAST_TypedefDecl *tdd ) { 
    parent->addTypedefToTypedefTable( tdd ); // default, defer to parent 
  }

  void walk_parents() { 
    fprintf(stderr, "wp: (%s)  ", getTypeString()); 
    print(); printf("\n");  fflush(stdout); 
    if (isSourceFile()) { fprintf(stderr, "(top sourcefile)\n\n"); return;}

    if (parent) parent->walk_parents();
    else fprintf(stderr, "UHOH, %s has no parent??\n", getTypeString());
    return; 
  }

  virtual chillAST_node *getEnclosingStatement( int level = 0 );
  virtual chillAST_VarDecl *multibase() { 
    fprintf(stderr,"(%s) forgot to implement multibase()\n", Chill_AST_Node_Names[asttype]);
    exit(-1);
  }
  virtual chillAST_node *multibase2() {  
    fprintf(stderr,"(%s) forgot to implement multibase2()\n", Chill_AST_Node_Names[asttype]);
    exit(-1);
  }

  
  virtual void gatherStatements( std::vector<chillAST_node*> &statements ) { 
    fprintf(stderr,"(%s) forgot to implement gatherStatements()\n" ,Chill_AST_Node_Names[asttype]); 
    dump();fflush(stdout); 
    print();
    fprintf(stderr, "\n\n"); 
  }


  virtual bool isSameAs( chillAST_node *other ){  // for tree comparison 
    fprintf(stderr,"(%s) forgot to implement isSameAs()\n" ,Chill_AST_Node_Names[asttype]); 
    dump(); fflush(stdout); 
    print();
    fprintf(stderr, "\n\n");   }

  void printPreprocBEFORE( int indent, FILE *fp );
  void printPreprocAFTER( int indent, FILE *fp );



};


#endif

