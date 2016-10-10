
/*****************************************************************************
 Copyright (C) 2008 University of Southern California
 Copyright (C) 2009-2010 University of Utah
 All Rights Reserved.

 Purpose:
   generate chill AST code for omega

 Notes:
 
 History:
   02/01/06 created by Chun Chen

*****************************************************************************/

#include <iostream>
#include <stack>
#include <code_gen/CG_chillBuilder.h>
#include <code_gen/CGdebug.h>
#include <code_gen/codegen_error.h>

namespace omega {
  
  // substitute at chill AST level
  chillAST_Node *substituteChill( const char *oldvar, CG_chillRepr *newvar, chillAST_Node *n, chillAST_Node *parent = NULL ) {
    if (n == NULL) {
      CG_DEBUG_PRINT(" pointer n == NULL\n");
      return NULL;
    }
    
    chillAST_Node *r = n;
    switch (n->getType()) {
      case CHILLAST_NODE_BINARYOPERATOR:
      case CHILLAST_NODE_UNARYOPERATOR:
      case CHILLAST_NODE_FORSTMT:
      case CHILLAST_NODE_IFSTMT:
      case CHILLAST_NODE_COMPOUNDSTMT:
      case CHILLAST_NODE_RETURNSTMT:
      case CHILLAST_NODE_PARENEXPR:
      case CHILLAST_NODE_CALLEXPR:
      case CHILLAST_NODE_IMPLICITCASTEXPR:
      case CHILLAST_NODE_CSTYLECASTEXPR:
      case CHILLAST_NODE_ARRAYSUBSCRIPTEXPR:
      case CHILLAST_NODE_MEMBEREXPR:
        for (int i=0;i<n->getNumChildren();++i)
          n->setChild(i,substituteChill(oldvar, newvar,n->getChild(i)));
        break;
      case CHILLAST_NODE_DECLREFEXPR: {
          chillAST_DeclRefExpr *DRE = (chillAST_DeclRefExpr *) n;
          if (!strcmp( oldvar,  DRE->declarationName)) {
            std::vector<chillAST_Node*> newnodes = newvar->chillnodes;
            chillAST_Node *firstn = newnodes[0]->clone();
            r = firstn;
          }
        }
        break;
      case CHILLAST_NODE_INTEGERLITERAL:
      case CHILLAST_NODE_FLOATINGLITERAL:
        // No op
        break;
      default:
        CG_ERROR("UNHANDLED statement of type %s %s\n",n->getTypeString());
        exit(-1);
    }
    return r;
  }

  CG_chillBuilder::CG_chillBuilder() { 
    toplevel = NULL;
    currentfunction = NULL; // not very useful
    symtab_ = symtab2_ = NULL; 
  }
  
  CG_chillBuilder::CG_chillBuilder(chillAST_SourceFile *top, chillAST_FunctionDecl *func) { 
    toplevel = top;
    currentfunction = func;
    
    symtab_  = currentfunction->getParameters();
    symtab2_ = currentfunction->getBody()->getSymbolTable();
  }
  
  CG_chillBuilder::~CG_chillBuilder() {
    
  }
  
  //-----------------------------------------------------------------------------
  // place holder generation  NOT IN CG_outputBuilder
  //-----------------------------------------------------------------------------
  //
  // FIXME: Function isn't working fully yet
  //
  CG_outputRepr* CG_chillBuilder::CreatePlaceHolder (int indent, 
                                                     CG_outputRepr *stmt,
                                                     Tuple<CG_outputRepr*> &funcList, 
                                                     Tuple<std::string> &loop_vars) const {
    fprintf(stderr, "CG_chillBuilder::CreatePlaceHolder()  TODO \n");
    exit(-1);                   // DFL 
    return NULL; 
    /* 
       if(Expr *expr = static_cast<CG_chillRepr *>(stmt)->GetExpression()) {
       for(int i=1; i<= funcList.size(); ++i) {
       if (funcList[i] == NULL)
       continue;
       
       CG_chillRepr *repr = static_cast<CG_chillRepr*>(funcList[i]);
       Expr* op = repr->GetExpression();
       delete repr;
       
       Expr *exp = expr;
       
       if(isa<BinaryOperator>(exp)) {
       //substitute(static_cast<BinaryOperator*>(exp)->getLHS(), loop_vars[i], op, exp);
       //substitute(static_cast<BinaryOperator*>(exp)->getRHS(), loop_vars[i], op, exp);
       }
       else if(isa<UnaryOperator>(exp))
       //substitute(static_cast<UnaryOperator*>(exp)->getSubExpr(), loop_vars[i], op, exp);
       
       }
       return new CG_chillRepr(expr);
       } else {
       StmtList *tnl = static_cast<CG_chillRepr *>(stmt)->GetCode();
       for(int i=1; i<= funcList.size(); ++i) {
       if (funcList[i] == NULL)
       continue;
       
       CG_chillRepr *repr = static_cast<CG_chillRepr*>(funcList[i]);
       Expr* op = repr->GetExpression();
       delete repr;
       
       for(unsigned j=0; j<tnl->size(); ++j) {
       Expr *exp = static_cast<Expr *>((*tnl)[j]);
       if(isa<BinaryOperator>(exp)) {
       //substitute(static_cast<BinaryOperator*>(exp)->getLHS(), loop_vars[i], op, exp);
       //substitute(static_cast<BinaryOperator*>(exp)->getRHS(), loop_vars[i], op, exp);
       }
       else if(isa<UnaryOperator>(exp))
       //substitute(static_cast<UnaryOperator*>(exp)->getSubExpr(), loop_vars[i], op, exp);
       
       }
       }
       return new CG_chillRepr(*tnl);
       }
    */ 
    
    
  }
  
  
  
  
  //----------------------------------------------------------------------------------------
  CG_outputRepr* CG_chillBuilder::CreateSubstitutedStmt(int indent, 
                                                        CG_outputRepr *stmt,
                                                        const std::vector<std::string> &vars, 
                                                        std::vector<CG_outputRepr*> &subs,
                                                        bool actuallyPrint) const {
    int numvars = vars.size();
    int numsubs = subs.size(); 

    if (numvars != numsubs)
      throw codegen_error("Unequal number of original vars and subs");

    if (numsubs == 0) {
      std::vector<chillAST_Node*> nodes = ((CG_chillRepr *) stmt)->getChillCode();
      // no cloning !!
      return new CG_chillRepr( nodes );
    }

    CG_chillRepr *old = (CG_chillRepr *) stmt; 
    std::vector<chillAST_Node*> oldnodes = old->getChillCode();

    for (int j=0; j<numsubs; j++) { 
      if (subs[j] != NULL) {
        // find the type of thing we'll be using to replace the old variable
        CG_chillRepr *CRSub = (CG_chillRepr *)(subs[j]); 
        std::vector<chillAST_Node*> nodes = CRSub->chillnodes;
        if (1 != nodes.size() )  { // always just one? 
          CG_ERROR("Replacement is not one statement\n");
          exit(-1);
        }
        for (int i=0; i<oldnodes.size(); i++) {
          oldnodes[i] = substituteChill( vars[j].c_str(), CRSub, oldnodes[i]);
        }
      }
    }
    return new CG_chillRepr( oldnodes );
  }

  CG_outputRepr* CG_chillBuilder::CreateAssignment(int indent,
                                                   CG_outputRepr *lhs,
                                                   CG_outputRepr *rhs) const {
    if(lhs == NULL || rhs == NULL) {
      CG_ERROR("Code generation: Missing lhs or rhs\n");
      return NULL;
    }
    
    CG_chillRepr *clhs = (CG_chillRepr *) lhs;
    CG_chillRepr *crhs = (CG_chillRepr *) rhs;
    chillAST_Node *lAST = clhs->chillnodes[0]; // always just one?
    chillAST_Node *rAST = crhs->chillnodes[0]; // always just one?
    
    chillAST_BinaryOperator *bop = new chillAST_BinaryOperator(lAST->clone(), "=", rAST->clone()); // clone??
    
    delete lhs; delete rhs;
    return new CG_chillRepr(bop);
  }
  
  
  
  
  CG_outputRepr* CG_chillBuilder::CreatePlusAssignment(int indent,               // += 
                                                       CG_outputRepr *lhs,
                                                       CG_outputRepr *rhs) const {
    if(lhs == NULL || rhs == NULL) {
      CG_ERROR("Code generation: Missing lhs or rhs\n");
      return NULL;
    }
    
    CG_chillRepr *clhs = (CG_chillRepr *) lhs;
    CG_chillRepr *crhs = (CG_chillRepr *) rhs;
    chillAST_Node *lAST = clhs->chillnodes[0]; // always just one?
    chillAST_Node *rAST = crhs->chillnodes[0]; // always just one?
    
    chillAST_BinaryOperator *bop = new chillAST_BinaryOperator(lAST->clone(), "+=", rAST->clone()); // clone??
    
    delete lhs; delete rhs;
    return new CG_chillRepr(bop);
  }
  
  
  
  
  //-----------------------------------------------------------------------------
  // function invocation generation
  //-----------------------------------------------------------------------------
  CG_outputRepr* CG_chillBuilder::CreateInvoke(const std::string &fname,
                                               std::vector<CG_outputRepr*> &list) const {
    CG_DEBUG_PRINT("CG_roseBuilder::CreateInvoke( fname %s, ...)\n", fname.c_str());

    if (fname == std::string("max") || fname == std::string("min")) {
      if (list.size() == 0) { return NULL; }
      else if (list.size() == 1) { return list[1]; }
      else {
        int last = list.size()-1;
        CG_outputRepr *CGOR; 
        CG_chillRepr  *CGCR;
        char macroname[32];
        const char * op;
        if (fname == std::string("max"))  op = ">";
        else op = "<";
        chillAST_Node *ternary = minmaxTernary( op,  ((CG_chillRepr*) list[0])->chillnodes[0],
                                                 ((CG_chillRepr*) list[1])->chillnodes[0]);
        CG_chillRepr *repr = new CG_chillRepr( ternary );
        return repr;
      }
    }
    else {
      // try to find the function name, for a function in this file
      const char *name = fname.c_str(); 
      chillAST_SourceFile *src = toplevel; // todo don't be dumb
      
      chillAST_Node *def = src->findCall(name);
      if (!def) {
        CG_ERROR("CG_chillBuilder::CreateInvoke( %s ), can't find a function or macro by that name\n", name);
        exit(-1); 
      }
      
      if (def->isMacroDefinition() || def->isFunctionDecl()) {
        chillAST_CallExpr *CE = new chillAST_CallExpr( def );
        int numparams = list.size();
        for (int i=0; i<numparams; i++) { 
          CG_chillRepr *CR = (CG_chillRepr *) list[i];
          CE->addArg( CR->GetCode() ); 
        }
        return  new CG_chillRepr( CE ); 
      }

      // todo addarg()
      //int numargs;
      //std::vector<class chillAST_Node*> args;
      fprintf(stderr, "Code generation: invoke function io_call not implemented\n");
      return NULL;
    }
  }
  
  
  //-----------------------------------------------------------------------------
  // comment generation - NOTE: Not handled
  //-----------------------------------------------------------------------------
  CG_outputRepr* CG_chillBuilder::CreateComment(int indent, const std::string &commentText) const {
    return NULL;
  }
  
  
  
  CG_outputRepr* CG_chillBuilder::CreateNullStatement() const {
    return new CG_chillRepr(  new chillAST_NoOp() );
  }
  
  
  
  //---------------------------------------------------------------------------
  // Attribute generation
  //---------------------------------------------------------------------------
  CG_outputRepr* CG_chillBuilder::CreateAttribute(CG_outputRepr *control,
                                                  const std::string &commentText) const {
    
    //fprintf(stderr, "in CG_chillBuilder.cc (OMEGA)   CG_chillBuilder::CreateAttribute()\n");
    //fprintf(stderr, "comment = '%s'\n", commentText.c_str()); 
    
    CG_chillRepr *CR = (CG_chillRepr *) control;
    int numnodes = CR->chillnodes.size(); 
    //fprintf(stderr, "%d chill nodes\n", numnodes); 
    if (numnodes > 0) { 
      //fprintf(stderr, "adding a comment to a %s\n", CR->chillnodes[0]->getTypeString()); 
      CR->chillnodes[0]->metacomment = strdup( commentText.c_str()); 
    }
    else { 
      fprintf(stderr, "CG_chillBuilder::CreateAttribute no chillnodes to attach comment to???\n");
    }
    return  static_cast<CG_chillRepr*>(control);
  };
  
  
  
  
  
  //-----------------------------------------------------------------------------
  // if stmt gen operations
  //-----------------------------------------------------------------------------
  CG_outputRepr* CG_chillBuilder::CreateIf(int indent, 
                                           CG_outputRepr *guardList,
                                           CG_outputRepr *true_stmtList, 
                                           CG_outputRepr *false_stmtList) const {
    //fprintf(stderr, "CG_chillBuilder::CreateIf()\n"); 
    
    if (true_stmtList == NULL && false_stmtList == NULL) {
      delete guardList;
      return NULL;
    }
    else if (guardList == NULL) {  // this seems odd 
      return StmtListAppend(true_stmtList, false_stmtList);
    }
    
    std::vector<chillAST_Node*> vectorcode =  static_cast<CG_chillRepr*>(guardList)->getChillCode();
    if (vectorcode.size() != 1 ) {
      fprintf(stderr, "CG_chillBuilder.cc IfStmt conditional is multiple statements?\n");
      exit(-1);
    }
    chillAST_Node *conditional = vectorcode[0];
    chillAST_CompoundStmt *then_part = NULL;
    chillAST_CompoundStmt *else_part = NULL;
    
    
    if (true_stmtList != NULL) { 
      then_part = new chillAST_CompoundStmt(  );
      vectorcode =  static_cast<CG_chillRepr*>(true_stmtList)->getChillCode(); 
      for (int i=0; i<vectorcode.size(); i++) then_part->addChild( vectorcode[i] ); 
    }
    
    if (false_stmtList != NULL) {
      else_part = new chillAST_CompoundStmt(  );
      vectorcode =  static_cast<CG_chillRepr*>(false_stmtList)->getChillCode(); 
      for (int i=0; i<vectorcode.size(); i++) else_part->addChild( vectorcode[i] ); 
    }
    
    
    chillAST_IfStmt *if_stmt = new chillAST_IfStmt( conditional, then_part, else_part);
    
    delete guardList;  
    delete true_stmtList;
    delete false_stmtList;
    
    return new CG_chillRepr( if_stmt );
  }
  
  
  //-----------------------------------------------------------------------------
  // inductive variable generation, to be used in CreateLoop as control
  //-----------------------------------------------------------------------------
  CG_outputRepr* CG_chillBuilder::CreateInductive(CG_outputRepr *index,
                                                  CG_outputRepr *lower,
                                                  CG_outputRepr *upper,
                                                  CG_outputRepr *step) const {
    if (index == NULL || lower == NULL || upper == NULL) {
      CG_ERROR("Code generation: invalid arguments to CreateInductive\n");
      return NULL;
    }
    if (step == NULL) {
      chillAST_IntegerLiteral *intlit = new chillAST_IntegerLiteral(1);
      step = new CG_chillRepr(intlit);
    }
    //static_cast<CG_chillRepr*>(index)->printChillNodes();
    //static_cast<CG_chillRepr*>(lower)->printChillNodes(); 
    //static_cast<CG_chillRepr*>(upper)->printChillNodes(); 
    //static_cast<CG_chillRepr*>(step )->printChillNodes(); 
    
    // index should be a DeclRefExpr
    std::vector<chillAST_Node*> nodes = static_cast<CG_chillRepr*>(index)->getChillCode();
    chillAST_Node *indexnode = nodes[0];
    if (strcmp("DeclRefExpr", indexnode->getTypeString())) {
      fprintf(stderr, "CG_chillBuilder::CreateInductive index is not a DeclRefExpr\n"); 
      if (indexnode->isIntegerLiteral()) fprintf(stderr, "isIntegerLiteral()\n"); 

      fprintf(stderr, "index is %s\n", indexnode->getTypeString());
      indexnode->print(); printf("\n");   fflush(stdout);
      indexnode->dump();  printf("\n\n"); fflush(stdout);
      int *i = 0; int j = i[0];
      exit(-1); 
    }
    
    nodes = static_cast<CG_chillRepr*>(lower)->getChillCode();
    chillAST_Node *lowernode = nodes[0];

    nodes = static_cast<CG_chillRepr*>(upper)->getChillCode();
    chillAST_Node *uppernode = nodes[0];

    nodes = static_cast<CG_chillRepr*>(step)->getChillCode();
    chillAST_Node *stepnode = nodes[0];

    // unclear is this will always be the same 
    // TODO error checking  && incr vs decr
    chillAST_BinaryOperator *init = new  chillAST_BinaryOperator( indexnode, "=", lowernode);
    chillAST_BinaryOperator *cond = new  chillAST_BinaryOperator( indexnode, "<=", uppernode);
    
    chillAST_BinaryOperator *incr = new  chillAST_BinaryOperator( indexnode, "+=", stepnode);
    
    chillAST_ForStmt *loop = new chillAST_ForStmt( init, cond, incr, NULL /* NULL BODY DANGER! */);
    
    return new CG_chillRepr(loop); 
    
    /*    
    //vector<chillAST_Node*> indexnodes = static_cast<CG_chillRepr*>(index)->getChillCode();
    chillAST_DeclRefExpr *index_decl
    Expr *lower_bound; //                 = static_cast<CG_chillRepr*>(lower)->getChillCode();
    Expr *upper_bound; //               = static_cast<CG_chillRepr*>(upper)->getChillCode();
    Expr *step_size  ; //                = static_cast<CG_chillRepr*>(step)->getChillCode();
    
    fprintf(stderr, "gonna die in CG_chillBuilder ~line 459\n");
    
    chillAST_BinaryOperator *for_init_stmt =  NULL; // new (astContext_)BinaryOperator(index_decl, lower_bound, BO_Assign, index_decl->getType(), SourceLocation());
    chillAST_BinaryOperator *test = NULL; // new (astContext_)BinaryOperator(index_decl, upper_bound, BO_LT, index_decl->getType(), SourceLocation());
    chillAST_BinaryOperator *increment = NULL; // new (astContext_)BinaryOperator(index_decl, step_size, BO_AddAssign, index_decl->getType(), SourceLocation());
    
    // For Body is null.. Take care of unnecessary parens!
    ForStmt *for_stmt = NULL; // new (astContext_)ForStmt(*astContext_, for_init_stmt, test, static_cast<VarDecl*>(index_decl->getDecl()), increment, NULL, SourceLocation(), SourceLocation(), SourceLocation());
    
    delete index;    
    delete lower;         
    delete upper;
    delete step;
    
    StmtList sl;
    sl.push_back(for_stmt);
    return new CG_chillRepr(sl);
    */     
  }
  
  
  
  //-----------------------------------------------------------------------------
  // Pragma Attribute
  //-----------------------------------------------------------------------------
  CG_outputRepr* CG_chillBuilder::CreatePragmaAttribute(CG_outputRepr *stmt, int looplevel, const std::string &pragmaText) const {
    fprintf(stderr, "CG_chillBuilder::CreatePragmaAttribute()   TODO\n");
    exit(-1); 
    // TODO    effectively a comment? 
    /* 
       SgNode *tnl = static_cast<CG_chillRepr*>(stmt)->tnl_;
       CodeInsertionAttribute* attr = NULL;
       if (!tnl->attributeExists("code_insertion")) {
       attr = new CodeInsertionAttribute();
       tnl->setAttribute("code_insertion", attr);
       }
       else {
       attr = static_cast<CodeInsertionAttribute*>(tnl->getAttribute("code_insertion"));
       }
       attr->add(new PragmaInsertion(looplevel, pragmaText));
    */
    return stmt;
  }
  
  //-----------------------------------------------------------------------------
  // Prefetch Attribute
  //-----------------------------------------------------------------------------
  CG_outputRepr* CG_chillBuilder::CreatePrefetchAttribute(CG_outputRepr* stmt, int looplevel, const std::string &arrName, int hint) const {
    fprintf(stderr, "CG_chillBuilder::CreatePrefetchAttribute()   TODO\n");
    exit(-1); 
    // TODO 
    /* 
       SgNode *tnl = static_cast<CG_chillRepr*>(stmt)->tnl_;
       CodeInsertionAttribute *attr = getOrCreateCodeInsertionAttribute(tnl);
       attr->add(new MMPrefetchInsertion(looplevel, arrName, hint));
    */
    return stmt;
  }
  
  
  
  
  
  
  
  //-----------------------------------------------------------------------------
  // loop stmt generation
  //-----------------------------------------------------------------------------
  CG_outputRepr* CG_chillBuilder::CreateLoop(int indent, 
                                             CG_outputRepr *control,
                                             CG_outputRepr *stmtList) const {
    //fprintf(stderr, "CG_chillBuilder::CreateLoop( indent %d)\n", indent); 
    
    if (stmtList == NULL) {
      delete control;
      return NULL;
    }
    else if (control == NULL) {
      fprintf(stderr, "Code generation: no inductive for this loop\n");
      return stmtList;    
    }
    
    // We assume the for statement is already created (using CreateInductive)
    std::vector<chillAST_Node*> code = static_cast<CG_chillRepr*>(control)->getChillCode();
    chillAST_ForStmt *forstmt =  (chillAST_ForStmt *)(code[0]);
    
    std::vector<chillAST_Node*> statements = static_cast<CG_chillRepr*>(stmtList)->getChillCode();
    //static_cast<CG_chillRepr*>(stmtList)->printChillNodes(); printf("\n"); fflush(stdout);
    
    chillAST_CompoundStmt *cs = new chillAST_CompoundStmt();
    for (int i=0; i<statements.size(); i++) {
      cs->addChild( statements[i] );
    }
    
    forstmt->setBody(cs);
    
    delete stmtList;
    return control;
  }
  
  //---------------------------------------------------------------------------
  // copy operation, NULL parameter allowed. this function makes pointer
  // handling uniform regardless NULL status
  //---------------------------------------------------------------------------
  /*
    virtual CG_outputRepr* CG_chillBuilder::CreateCopy(CG_outputRepr *original) const {
    if (original == NULL)
    return NULL;
    else
    return original->clone();
    }
  */
  
  //-----------------------------------------------------------------------------
  // basic int, identifier gen operations
  //-----------------------------------------------------------------------------
  CG_outputRepr* CG_chillBuilder::CreateInt(int i) const {
    chillAST_IntegerLiteral *il = new chillAST_IntegerLiteral(i);
    return new CG_chillRepr(il);
  }
  CG_outputRepr* CG_chillBuilder::CreateFloat(float f) const {
    chillAST_FloatingLiteral *fl = new chillAST_FloatingLiteral(f, 1, NULL);
    return new CG_chillRepr(fl);
  }
  CG_outputRepr* CG_chillBuilder::CreateDouble(double d) const {
    chillAST_FloatingLiteral *dl = new chillAST_FloatingLiteral(d, 2, NULL);
    return new CG_chillRepr(dl);
  }
  
  
  //----------------------------------------------------------------------------------------
  bool CG_chillBuilder::isInteger(CG_outputRepr *op) const{
    CG_chillRepr *cr = (CG_chillRepr *)op;
    return cr->chillnodes[0]->isIntegerLiteral(); 
  }
  
  
  //----------------------------------------------------------------------------------------
  CG_outputRepr* CG_chillBuilder::CreateIdent(const std::string &_s) const {
    chillAST_VarDecl* already_parameter = symbolTableFindName(symtab_,  _s.c_str());
    chillAST_VarDecl* already_internal  = symbolTableFindName(symtab2_, _s.c_str());
    if ( already_parameter )
      CG_DEBUG_PRINT("%s was already a parameter??\n",  _s.c_str());
    if ( already_internal )
      CG_DEBUG_PRINT("%s was already defined in the function body\n",  _s.c_str());

    if ( (!already_parameter) && (! already_internal)) {  
      CG_DEBUG_PRINT("adding symbol %s to symtab2_ because it was not already there\n", _s.c_str());
      // this is copying roseBuilder, but is probably wrong. it is assuming
      // that the ident is a direct child of the current function 
      
      chillAST_VarDecl *vd = new chillAST_VarDecl( "int", _s.c_str(), "", currentfunction->getBody());
      currentfunction->addVariableToScope( vd ); // use symtab2_  ?
    
      
      chillAST_DeclRefExpr *dre = new chillAST_DeclRefExpr( "int", _s.c_str(), (chillAST_Node*)vd); // parent not available
      return new CG_chillRepr( dre );
    }
    // variable was already defined as either a parameter or internal variable to the function.

    // NOW WHAT??  gotta return something
    chillAST_VarDecl *vd = currentfunction->getVariableDeclaration( _s.c_str() );
    //fprintf(stderr, "vd %p\n", vd); 

    chillAST_DeclRefExpr *dre = new chillAST_DeclRefExpr( "int", _s.c_str(), (chillAST_Node*)vd); // parent not available
    return new CG_chillRepr( dre );
  }
  



  
  //-----------------------------------------------------------------------------
  // binary arithmetic operations
  //-----------------------------------------------------------------------------
  CG_outputRepr* CG_chillBuilder::CreatePlus(CG_outputRepr *lop,
                                             CG_outputRepr *rop) const {
    if (rop == NULL) return lop;
    else if(lop == NULL) return rop;
    
    chillAST_Node *left  = ((CG_chillRepr*)lop)->chillnodes[0];
    chillAST_Node *right = ((CG_chillRepr*)rop)->chillnodes[0];
    chillAST_BinaryOperator *bop = new chillAST_BinaryOperator( left, "+", right);
    return new CG_chillRepr( bop );
  }
  
  //-----------------------------------------------------------------------------  
  CG_outputRepr* CG_chillBuilder::CreateMinus(CG_outputRepr *lop,
                                              CG_outputRepr *rop) const {
    if(rop == NULL) {
      CG_ERROR("right side is NULL\n");
      return lop;
    }
    
    CG_chillRepr *clop = (CG_chillRepr *) lop;
    CG_chillRepr *crop = (CG_chillRepr *) rop;
    
    if(clop == NULL) {
      chillAST_Node *rAST = crop->chillnodes[0];
      chillAST_UnaryOperator *ins = new chillAST_UnaryOperator("-", true, rAST->clone());
      delete crop;
      return new CG_chillRepr(ins);
    } else {
      chillAST_Node *lAST = clop->chillnodes[0];
      chillAST_Node *rAST = crop->chillnodes[0];

      chillAST_BinaryOperator *bop = new chillAST_BinaryOperator(lAST->clone(), "-", rAST->clone());
      
      delete clop; delete crop;
      return new CG_chillRepr(bop);
    }
  }
  
  
  //-----------------------------------------------------------------------------
  CG_outputRepr* CG_chillBuilder::CreateTimes(CG_outputRepr *lop,
                                              CG_outputRepr *rop) const {
    if (rop == NULL || lop == NULL) {
      CG_ERROR("Operand for times is null\n");
      if (rop != NULL) {
        rop->clear();
        delete rop;
      }
      if (lop != NULL) {
        lop->clear();
        delete lop;
      }                 
      return NULL;
    }             
    
    CG_chillRepr *clop = (CG_chillRepr *) lop;
    CG_chillRepr *crop = (CG_chillRepr *) rop;
    
    chillAST_Node *lAST = clop->chillnodes[0];
    chillAST_Node *rAST = crop->chillnodes[0];

    chillAST_BinaryOperator *binop = new chillAST_BinaryOperator( lAST, "*", rAST);
    delete lop; delete rop;
    return new CG_chillRepr( binop );
  }
  
  
  
  //-----------------------------------------------------------------------------
  //  CG_outputRepr *CG_chillBuilder::CreateDivide(CG_outputRepr *lop, CG_outputRepr *rop) const {
  //    return CreateIntegerFloor(lop, rop);
  //  }
  
  
  
  //-----------------------------------------------------------------------------
  CG_outputRepr* CG_chillBuilder::CreateIntegerDivide(CG_outputRepr *lop,
                                                      CG_outputRepr *rop) const {
    if (rop == NULL) {
      CG_ERROR("divide by NULL\n");
      return NULL;
    }
    else if ( lop == NULL ) {
      delete rop;
      return NULL;
    }
    
    CG_chillRepr *clop = (CG_chillRepr *) lop;
    CG_chillRepr *crop = (CG_chillRepr *) rop;
    
    chillAST_Node *lAST = clop->chillnodes[0];
    chillAST_Node *rAST = crop->chillnodes[0];

    chillAST_BinaryOperator *binop = new chillAST_BinaryOperator( lAST, "/", rAST);
    delete lop; delete rop; // ?? 
    return new CG_chillRepr( binop );
  }
  
  
  //-----------------------------------------------------------------------------
  CG_outputRepr* CG_chillBuilder::CreateIntegerFloor(CG_outputRepr* lop, CG_outputRepr* rop) const { 
    CG_chillRepr *clop = (CG_chillRepr *) lop;
    CG_chillRepr *crop = (CG_chillRepr *) rop;
    
    chillAST_Node *lAST = clop->chillnodes[0];
    chillAST_Node *rAST = crop->chillnodes[0];

    chillAST_BinaryOperator *binop = new chillAST_BinaryOperator( lAST, "/", rAST);
    return new CG_chillRepr( binop );
  }
  
  
  
  //-----------------------------------------------------------------------------
  CG_outputRepr* CG_chillBuilder::CreateIntegerMod(CG_outputRepr *lop,
                                                   CG_outputRepr *rop) const {
    CG_chillRepr *l = (CG_chillRepr *) lop;
    CG_chillRepr *r = (CG_chillRepr *) rop; 
    
    chillAST_Node *lhs = l->GetCode();
    chillAST_Node *rhs = r->GetCode();
    
    chillAST_BinaryOperator *BO = new  chillAST_BinaryOperator(lhs, "%", rhs );
    return new CG_chillRepr(BO);
  }

  CG_outputRepr *CG_chillBuilder::CreateIntegerCeil(CG_outputRepr *lop, CG_outputRepr *rop) const {
    return CreateMinus(NULL, CreateIntegerFloor(CreateMinus(NULL, lop), rop));
  }

  CG_outputRepr* CG_chillBuilder::CreateAnd(CG_outputRepr *lop,
                                            CG_outputRepr *rop) const {
    if (rop == NULL)
      return lop;
    else if (lop == NULL)
      return rop;

    CG_chillRepr *clop = (CG_chillRepr *) lop;
    CG_chillRepr *crop = (CG_chillRepr *) rop;
    
    chillAST_Node *lAST = clop->chillnodes[0];
    chillAST_Node *rAST = crop->chillnodes[0];

    chillAST_BinaryOperator *binop = new chillAST_BinaryOperator( lAST, "&&", rAST);
    return new CG_chillRepr( binop );
  }

  CG_outputRepr* CG_chillBuilder::CreateLE(CG_outputRepr *lop,
                                           CG_outputRepr *rop) const {
    if (rop == NULL || lop == NULL) {
      return NULL;           
    }            
    
    CG_chillRepr *clop = (CG_chillRepr *) lop;
    CG_chillRepr *crop = (CG_chillRepr *) rop;
    
    chillAST_Node *lAST = clop->chillnodes[0];
    chillAST_Node *rAST = crop->chillnodes[0];

    chillAST_BinaryOperator *binop = new chillAST_BinaryOperator( lAST, "<=", rAST);
    delete lop; delete rop; // ?? 
    return new CG_chillRepr( binop );
  }
  
  
  //-----------------------------------------------------------------------------
  CG_outputRepr* CG_chillBuilder::CreateEQ(CG_outputRepr *lop,
                                           CG_outputRepr *rop) const {
    //fprintf(stderr, "CG_chillBuilder::CreateEQ()\n");  
    if (rop == NULL || lop == NULL) {
      return NULL;           
    }            
    
    CG_chillRepr *clop = (CG_chillRepr *) lop;
    CG_chillRepr *crop = (CG_chillRepr *) rop;
    
    chillAST_Node *lAST = clop->chillnodes[0]; // always just one?
    chillAST_Node *rAST = crop->chillnodes[0]; // always just one?
    
    //fprintf(stderr, "building "); 
    //lAST->print(0, stderr); 
    //fprintf(stderr, " = ");
    //rAST->print(0, stderr);
    //fprintf(stderr, "  ??\n"); 
    
    chillAST_BinaryOperator *binop = new chillAST_BinaryOperator( lAST, "==", rAST);
    delete lop; delete rop; // ?? 
    return new CG_chillRepr( binop );
  }
  
  
  
  
  CG_outputRepr* CG_chillBuilder::CreateNEQ(CG_outputRepr *lop,
                                            CG_outputRepr *rop) const {
    //fprintf(stderr, "CG_chillBuilder::CreateNEQ()\n");  
    if (rop == NULL || lop == NULL) {
      return NULL;           
    }            
    
    CG_chillRepr *clop = (CG_chillRepr *) lop;
    CG_chillRepr *crop = (CG_chillRepr *) rop;
    
    chillAST_Node *lAST = clop->chillnodes[0]; // always just one?
    chillAST_Node *rAST = crop->chillnodes[0]; // always just one?
    
    //fprintf(stderr, "building "); 
    //lAST->print(0, stderr); 
    //fprintf(stderr, " != ");
    //rAST->print(0, stderr);
    //fprintf(stderr, "  ??\n"); 
    
    chillAST_BinaryOperator *binop = new chillAST_BinaryOperator( lAST, "!=", rAST);
    delete lop; delete rop; // ?? 
    return new CG_chillRepr( binop );
  }
  
  
  CG_outputRepr* CG_chillBuilder::CreateDotExpression(CG_outputRepr *lop,
                                                      CG_outputRepr *rop) const {
    //fprintf(stderr, "\nCG_chillBuilder::CreateDotExpression()\n");  
    if (rop == NULL || lop == NULL) {
      return NULL;           
    }            
    
    CG_chillRepr *clop = (CG_chillRepr *) lop;
    CG_chillRepr *crop = (CG_chillRepr *) rop;
    
    chillAST_Node *lAST = clop->chillnodes[0]; // always just one?
    chillAST_Node *rAST = crop->chillnodes[0]; // always just one?
    //fprintf(stderr, "left is %s,  right is %s\n", lAST->getTypeString(), rAST->getTypeString()); 
    
    if ( !rAST->isVarDecl()) { 
      fprintf(stderr, "CG_chillBuilder::CreateDotExpression() right is a %s, not a vardecl\n",
              rAST->getTypeString());
      exit(-1); 
    }
    chillAST_VarDecl *rvd = (chillAST_VarDecl *)rAST;
    //fprintf(stderr, "building "); 
    //lAST->print(0, stderr); 
    //fprintf(stderr, ".");
    //rAST->print(0, stderr);
    //fprintf(stderr, "  ??\n"); 
    
    //chillAST_BinaryOperator *binop = new chillAST_BinaryOperator( lAST, ".", rAST, NULL);
    
    
    // MemberExpr should be a DeclRefExpr on the left?
    chillAST_DeclRefExpr *DRE = NULL;
    if (lAST->isDeclRefExpr()) DRE = (chillAST_DeclRefExpr *)lAST; 
    if (lAST->isVarDecl()) { 
      // make a DeclRefExpr ?  probably an error upstream of here in this case
      DRE = new chillAST_DeclRefExpr( (chillAST_VarDecl *)lAST ); 
    }
    if (!DRE) { 
      fprintf(stderr, "CG_chillBuilder::CreateDotExpression(), can't create base\n");
      exit(-1); 
    }
    chillAST_MemberExpr *memexpr = new chillAST_MemberExpr( DRE, rvd->varname, NULL, CHILLAST_MEMBER_EXP_DOT );
    
    
    //delete lop; delete rop; // ??  
    return new CG_chillRepr( memexpr );
  }
  
  
  //-----------------------------------------------------------------------------
  // stmt list gen operations
  //-----------------------------------------------------------------------------
  CG_outputRepr* CG_chillBuilder::CreateStmtList(CG_outputRepr *singleton) const {
    //fprintf(stderr, "CG_chillBuilder::CreateStmtList()\n");  
    if(singleton == NULL) return NULL;
    
    exit(-1);                  // DFL 
    return( NULL ); 
    /* 
       StmtList *tnl = static_cast<CG_chillRepr *>(singleton)->GetCode();
       
       if(tnl->empty()) {
       StmtList foo;
       fprintf(stderr, "gonna die soon  CG_chillBuilder::CreateStmtList()\n");  exit(-1); 
       //foo.push_back(static_cast<CG_chillRepr*>(singleton)->op_);
       return new CG_chillRepr(foo);
       }
       delete singleton;
       return new CG_chillRepr(*tnl);
    */
  }
  
  
  //-----------------------------------------------------------------------------
  CG_outputRepr* CG_chillBuilder::StmtListInsertLast(CG_outputRepr *list, 
                                                     CG_outputRepr *node) const {
    return StmtListAppend(list, node);
  }
  
  
  //-----------------------------------------------------------------------------
  CG_outputRepr* CG_chillBuilder::StmtListAppend(CG_outputRepr *list1, 
                                                 CG_outputRepr *list2) const {
    
    //fprintf(stderr, "CG_chillBuilder::StmtListAppend()\n"); 
    
    if(list1 == NULL) return list2;
    else if(list2 == NULL) return list1;
    
    CG_chillRepr *cr1 = (CG_chillRepr *)list1;
    CG_chillRepr *cr2 = (CG_chillRepr *)list2;
    
    int numtoadd = cr2->chillnodes.size();
    //fprintf(stderr, "before: %d nodes and %d nodes\n", cr1->chillnodes.size(), numtoadd ); 
    for (int i=0; i<numtoadd; i++){
      (cr1->chillnodes).push_back(cr2->chillnodes[i] );
    }
    //fprintf(stderr, "after %d nodes\n", cr1->chillnodes.size() ); 
    
    delete list2;
    return list1;
    
  }
  
  
  bool CG_chillBuilder::QueryInspectorType(const std::string &varName) const {
    fprintf(stderr, "CG_chillBuilder::QueryInspectorType( %s )\n", varName.c_str()); 
    int *i=0; int j= i[0]; 
    return false;
  }
  
  
  CG_outputRepr* CG_chillBuilder::CreateArrayRefExpression(const std::string &_s,
                                                           CG_outputRepr *rop) const {
    fprintf(stderr, "CG_chillBuilder::CreateArrayRefExpression()  DIE\n");
    fprintf(stderr, "string s  '%s'\n", _s.c_str());
    rop->dump(); 

    int *i=0; int j = i[0]; 
    exit(-1);
  }
  
  
  CG_outputRepr* CG_chillBuilder::CreateArrayRefExpression(CG_outputRepr*left, 
                                                           CG_outputRepr*right) const{
    
    chillAST_Node *l = ((CG_chillRepr *)left)->GetCode();
    chillAST_Node *r = ((CG_chillRepr *)right)->GetCode();
    
    chillAST_Node *base = NULL;
    
    if (l->isDeclRefExpr()) base = l;
    if (l->isMemberExpr()) base = l;
    if (l->isVarDecl()) { // ?? 
      // make a declRefExpr that uses VarDecl l
      base = (chillAST_Node *) new chillAST_DeclRefExpr( (chillAST_VarDecl *)l );
    }
    
    if (!base)  {
      fprintf(stderr, "CG_chillBuilder::CreateArrayRefExpression(), left is %s\n", l->getTypeString()); 
      
      exit(-1);
    }
    
    
    
    chillAST_ArraySubscriptExpr *ASE = new chillAST_ArraySubscriptExpr( base, r, NULL, 0); // unique TODO 
    return new CG_chillRepr( ASE ); 
  }
  
  
  CG_outputRepr* CG_chillBuilder::ObtainInspectorData(const std::string &_s, const std::string &member_name) const{
    fprintf(stderr, "CG_chillBuilder::ObtainInspectorData( %s, %s)\n", 
            _s.c_str(), member_name.c_str());
    
    //WTF 

    return ObtainInspectorRange( _s, member_name ); 
  }
  
  
  CG_outputRepr *CG_chillBuilder::CreateAddressOf(CG_outputRepr* op) const {
    fprintf(stderr, "CG_chillBuilder::CreateAddressOf()\n");
    exit(-1);
  }
  
  CG_outputRepr* CG_chillBuilder::CreateBreakStatement() const { 
    fprintf(stderr, "CG_chillBuilder::CreateBreakStatement()\n");
    exit(-1);
  }
  
  
  CG_outputRepr *CG_chillBuilder::CreateStatementFromExpression(CG_outputRepr *exp) const { 
    fprintf(stderr, "CG_chillBuilder::CreateStatementFromExpression()\n");
    exit(-1);
  }
  
  


  CG_outputRepr *CG_chillBuilder::CreateStruct(const std::string struct_name,
                                               std::vector<std::string> data_members,
                                               std::vector<CG_outputRepr *> data_types)
  { 
    
    fprintf(stderr, "\nCG_chillBuilder::CreateStruct( %s )\n", struct_name.c_str()); 
    
/* WRONG - a typedef 
    // NEED TO ADD TYPEDEF TO ... SOMETHING 
    
    chillAST_TypedefDecl *tdd = new chillAST_TypedefDecl( ) ;
    
    tdd->setStructName(struct_name.c_str()); 
    tdd->setStruct( true ); 
    int n_memb = data_members.size();
    int n_data_types = data_types.size();
    for (int i=0; i<n_memb; i++) { 
      chillAST_VarDecl *vd;
      fprintf(stderr, "member %s type ", data_members[i].c_str()); 
      if (i <n_data_types) {
        vd = (chillAST_VarDecl *) ((CG_chillRepr *)data_types[i])->GetCode(); 
        vd->varname = strdup(  data_members[i].c_str() ); 
        bool simplepointer = (vd->numdimensions == 1 && !vd->knownArraySizes);
        if (simplepointer) fprintf(stderr, "pointer to "); 
        fprintf(stderr, "%s\n", vd->vartype );
        if (vd->numdimensions > 0 && vd->knownArraySizes) {
          for (int k=0; k<vd->numdimensions; k++) fprintf(stderr, "[%d]", vd->arraysizes[k]);
        }
      }
      else { 
        fprintf(stderr, "type int BY DEFAULT (bad idea)\n");
        vd = new chillAST_VarDecl( "int", data_members[i].c_str(), "", NULL);
      }
      // add vd to suparts of the struct typedef 
      tdd->subparts.push_back( vd ); 
      
      fprintf(stderr, "\n"); 
    }
    
    // put the typedef in the top level ... for now   TODO 
    toplevel->insertChild( 0, tdd); 
    return new CG_chillRepr( tdd ); 
*/


    chillAST_RecordDecl *rd = new chillAST_RecordDecl(struct_name.c_str(), NULL);
    rd->setParent(toplevel);
    rd->setStruct( true ); 
    // SO FAR, struct has no members! 

    toplevel->insertChild(0, rd);  // inserts at beginning of file, (after defines?)
    // note: parent at top level so far   TODO 
    //toplevel->print(); printf("\n\n");  fflush(stdout); 

    int n_memb       = data_members.size();
    int n_data_types = data_types.size();
    // add struct members
    for (int i=0; i<n_memb; i++) { 
      chillAST_VarDecl *vd = NULL;
      //fprintf(stderr, "%d member %s type ", i, data_members[i].c_str()); 
      if (i < n_data_types) { 
        // this should always happen, formerly, if no data type was 
        // specified, it was an int. bad idea
        vd = (chillAST_VarDecl *) ((CG_chillRepr *)data_types[i])->GetCode(); 

        // vd did not have a name before 
        vd->varname = strdup(  data_members[i].c_str() ); 

        vd->setParent(rd);  // ??

        bool simplepointer = (vd->numdimensions == 1 && !vd->knownArraySizes);
        if (simplepointer) {  
          fprintf(stderr, "struct member %s is pointer to %s\n", vd->varname, vd->vartype);
          vd->arraypointerpart = strdup("*"); // ?? 
        }
        else { 
          //fprintf(stderr, "struct member %s is not a pointer TODO!\n", vd->varname); 
          fprintf(stderr, "struct member %s is %s\n", vd->varname, vd->vartype); 
          
          // it should be good to go ??? 
        }
        //vd->print(); printf("\n"); fflush(stdout); 
        //fprintf(stderr, "%s\n", vd->vartype );
        //if (vd->numdimensions > 0 && vd->knownArraySizes) {
        //  for (int k=0; k<vd->numdimensions; k++) fprintf(stderr, "[%d]", vd->arraysizes[k]);
        //} 
      }
      else { 
        fprintf(stderr, "int BY DEFAULT (bad idea) FIXME\n"); // TODO 
        vd = new chillAST_VarDecl( "int", data_members[i].c_str(), "", NULL);
      }
      rd->addSubpart( vd );
      //fprintf(stderr, "\n"); 
    }
    fprintf(stderr, "\n"); 
    return new CG_chillRepr( rd ); 
  }
  
  
  
  CG_outputRepr *CG_chillBuilder::CreateClassInstance(std::string name ,  // TODO can't make array
                                                      CG_outputRepr *class_def){
    fprintf(stderr, "CG_chillBuilder::CreateClassInstance( %s )\n", name.c_str()); 
    
    CG_chillRepr *CD = (CG_chillRepr *)class_def; 
    chillAST_Node *n = CD->GetCode();
    //fprintf(stderr, "class def is of type %s\n", n->getTypeString());
    //n->print(); printf("\n"); fflush(stdout); 

    if (n->isTypeDefDecl()) { 
      chillAST_TypedefDecl *tdd = (chillAST_TypedefDecl *)n;
      //tdd->print(); printf("\n"); fflush(stdout);
      
      chillAST_VarDecl *vd = new chillAST_VarDecl( tdd, name.c_str(), "");
      
      // we need to add this to function ??  TODO 
      //fprintf(stderr, "adding typedef instance to symbolTable\n");
      chillAST_SymbolTable *st =  currentfunction->getBody()->getSymbolTable();
      //printSymbolTable(st); 

      currentfunction->getBody()->addVariableToScope( vd ); // TODO
      currentfunction->getBody()->insertChild(0, vd);  // TODO 
      //printSymbolTable(st); 
      
      return new CG_chillRepr( vd ); 
    }
    if  (n->isRecordDecl()) { 
      fprintf(stderr, "a RecordDecl\n"); 

      chillAST_RecordDecl *rd = (chillAST_RecordDecl *) n;
      rd->print(); printf("\n"); fflush(stdout);
      rd->dump(); printf("\n");  fflush(stdout);
      
      chillAST_VarDecl *vd = new chillAST_VarDecl( rd, name.c_str(), "");

      //fprintf(stderr, "CG_chillBuilder.cc, adding struct instance to body of function's symbolTable\n");


      // we need to add this to function ??  TODO 
      currentfunction->getBody()->addVariableToScope( vd );
      currentfunction->getBody()->insertChild(0, vd);
      //printf("\nafter adding vardecl, source is:\n");
      currentfunction->getBody()->print(); fflush(stdout);

      //printf("\nafter adding vardecl, symbol table is:\n"); 
      chillAST_SymbolTable *st =  currentfunction->getBody()->getSymbolTable();
      //printSymbolTable(st); fflush(stdout); 
      
      return new CG_chillRepr( vd ); 
    }

    fprintf(stderr, "ERROR: CG_chillBuilder::CreateClassInstance() not sent a class or struct\n"); 
    int *i=0; int j = i[0]; 
    return NULL; 
  }
  
  
  
  CG_outputRepr *CG_chillBuilder::lookup_member_data(CG_outputRepr* classtype, 
                                                     std::string varName, 
                                                     CG_outputRepr *instance) {
    
    
    //fprintf(stderr, "CG_chillBuilder::lookup_member_data( %s )\n", varName.c_str()); 
    
    chillAST_VarDecl* sub = NULL;

    CG_chillRepr *CR = (CG_chillRepr *)classtype;
    chillAST_Node *classnode = CR->GetCode();
    //fprintf(stderr, "classnode is %s\n", classnode->getTypeString()); classnode->print(); printf("\n"); fflush(stdout); 
    if (! ( classnode->isTypeDefDecl() || 
            classnode->isRecordDecl() )) { 
      fprintf(stderr, "ERROR: CG_chillBuilder::lookup_member_data(), classnode is not a TypeDefDecl or a RecordDecl\n"); 
      exit(-1); 
    }


    CG_chillRepr *CI = (CG_chillRepr *)instance; 

    chillAST_Node *in = CI->GetCode();
    //fprintf(stderr, "instance is %s\n", in->getTypeString()); 
    //in->print(); printf("\n"); fflush(stdout); 
    
    if ( !in->isVarDecl() ) { // error, instance needs to be a vardecl
      fprintf(stderr, "ERROR: CG_chillBuilder::lookup_member_data() instance needs to be a VarDecl, not a %s", in->getTypeString());
      exit(-1);
    }
    chillAST_VarDecl *vd = (chillAST_VarDecl *)in;
    if (vd->typedefinition != classnode && 
      vd->vardef != classnode) { 
      fprintf(stderr, "vd: typedef %p  vardev %p    classnode %p\n", vd->typedefinition, vd->vardef, classnode); 
      fprintf(stderr, "CG_chillBuilder::lookup_member_data(), instance is not of correct class \n");
      
      exit(-1);
    }
    
    

    if (classnode->isTypeDefDecl()){ 
      chillAST_TypedefDecl *tdd = (chillAST_TypedefDecl *)classnode;
      if ( !tdd->isAStruct() ) { 
        fprintf(stderr, "ERROR: CG_chillBuilder::lookup_member_data() instance must be a struct or class\n");
        exit(-1);
      }
      
      sub = tdd->findSubpart( varName.c_str() ); 
    }

    if (classnode->isRecordDecl()){ 
      chillAST_RecordDecl *rd = (chillAST_RecordDecl *)classnode;
      if ( !rd->isAStruct() ) { 
        fprintf(stderr, "ERROR: CG_chillBuilder::lookup_member_data() instance must be a struct or class\n");
        exit(-1);
      }
      
      //fprintf(stderr, "looking for member (subpart) %s in RecordDecl\n",  varName.c_str()); 
      sub = rd->findSubpart( varName.c_str() ); 
    }   

    if (!sub) {
      fprintf(stderr, "CG_chillBuilder::lookup_member_data(), variable %s is not submember of class/struct\n"); 
      exit(-1);
    }
    
    //fprintf(stderr, "subpart (member) %s is\n", varName.c_str()); sub->print(); printf("\n"); fflush(stdout);

    return( new CG_chillRepr( sub ) ); // the vardecl inside the struct typedef 
  }
  
  
  CG_outputRepr* CG_chillBuilder::CreatePointer(std::string  &name) const { 
    //fprintf(stderr, "CG_chillBuilder::CreatePointer( %s )\n", name.c_str()); 
    
    chillAST_VarDecl *vd = new chillAST_VarDecl( "int", name.c_str(), "*", currentfunction->getBody());
    //vd->print(); printf("\n"); fflush(stdout); 
    //vd->dump(); printf("\n"); fflush(stdout); 
    
    //printSymbolTable( currentfunction->getBody()->getSymbolTable() ); 

    chillAST_DeclRefExpr *dre = new chillAST_DeclRefExpr( vd ); // ?? 
    return new CG_chillRepr( dre );  // need a declrefexpr? 
  }
  

  CG_outputRepr* CG_chillBuilder::ObtainInspectorRange(const std::string &structname, const std::string &member) const {
    //fprintf(stderr, "CG_chillBuilder::ObtainInspectorRange(%s,  %s )\n", structname.c_str(), member.c_str()); 
    
    // find a struct/class with name structname and member member
    // return a Member access (or binary dot op )
    // seems like you need to know where (scoping) to look for the struct definition
    
    std::vector<chillAST_VarDecl*> decls;
    currentfunction->gatherVarDecls( decls );
    //fprintf(stderr, "\nfunc has %d vardecls  (looking for %s)\n", decls.size(), structname.c_str()); 
    
    chillAST_VarDecl *thestructvd = NULL;
    for (int i=0; i<decls.size(); i++) { 
      chillAST_VarDecl *vd = decls[i];
      //vd->print(); printf("\n"); fflush(stdout); 
      
      if (structname == vd->varname) { 
        //fprintf(stderr, "found it!\n"); 
        thestructvd = vd;
        break;
      }
    }
    
    if (!thestructvd) { 
      fprintf(stderr, "CG_chillBuilder::ObtainInspectorRange could not find variable named %s in current function\n", structname.c_str()); 
      exit(-1); 
    }
    
    // make sure the variable is a struct with a member with the correct name
    chillAST_RecordDecl *rd = thestructvd->getStructDef(); 
    if ( !rd ) { 
      fprintf(stderr, "CG_chillBuilder::ObtainInspectorRange(), variable %s is not a struct/class\n",  structname.c_str()); 
      exit(-1);
    }
    
    
    chillAST_VarDecl *sub = rd->findSubpart( member.c_str() ); 
    if (!sub) { 
      fprintf(stderr, "CG_chillBuilder::ObtainInspectorRange(), struct/class %s has no member named %s\n",  structname.c_str(), member.c_str()); 
      exit(-1); 
    }
    
    
    // build up a member expression  (or a binop with dot operation?? )
    // make a declrefexpr that refers to this variable definition
    chillAST_DeclRefExpr *DRE = new chillAST_DeclRefExpr( thestructvd ); 
    chillAST_MemberExpr *ME = new chillAST_MemberExpr( DRE, member.c_str(), NULL); // uniq TODO
    
    return new CG_chillRepr( ME ); 
  }
  
} // namespace


