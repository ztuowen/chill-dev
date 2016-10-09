/*****************************************************************************
  Copyright (C) 2009-2010 University of Utah
  All Rights Reserved.

Purpose:
CHiLL's CLANG interface.
convert from CLANG AST to chill AST

Notes:
Array supports mixed pointer and array type in a single declaration.

History:
12/10/2010 LLVM/CLANG Interface created by Saurav Muralidharan.
 *****************************************************************************/

#include <typeinfo>
#include <sstream>
#include "ir_clang.hh"
#include "loop.hh"
#include "chill_error.hh"

#include "clang/Frontend/FrontendActions.h"
#include <clang/CodeGen/CodeGenAction.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/CompilerInvocation.h>
#include <clang/Basic/DiagnosticOptions.h>
#include <clang/Frontend/TextDiagnosticPrinter.h>
#include <clang/AST/ASTContext.h>
#include <clang/AST/RecordLayout.h>
#include <clang/AST/Decl.h>
#include <clang/Parse/ParseAST.h>
#include <clang/Basic/TargetInfo.h>

#include <llvm/ADT/IntrusiveRefCntPtr.h>
#include <llvm/Support/Host.h>

#include "code_gen/CG_chillRepr.h"
#include "code_gen/CG_chillBuilder.h"
#include <vector>
#include <chilldebug.h>

#include "chillAST.h"

#define NL_RET(x) {chillAST_NodeList *ret = new chillAST_NodeList(); \
                    ret->push_back(x); \
                    return ret;}

// TODO move to ir_clang.hh

chillAST_NodeList* ConvertTypeDefDecl(clang::TypedefDecl *TDD);

chillAST_NodeList* ConvertRecordDecl(clang::RecordDecl *D);

chillAST_NodeList* ConvertDeclStmt(clang::DeclStmt *clangDS);

chillAST_NodeList* ConvertCompoundStmt(clang::CompoundStmt *clangCS);

chillAST_NodeList* ConvertFunctionDecl(clang::FunctionDecl *D);

chillAST_NodeList* ConvertForStmt(clang::ForStmt *clangFS);

chillAST_NodeList* ConvertUnaryOperator(clang::UnaryOperator *clangU);

chillAST_NodeList* ConvertBinaryOperator(clang::BinaryOperator *clangBO);

chillAST_NodeList* ConvertArraySubscriptExpr(clang::ArraySubscriptExpr *clangASE);

chillAST_NodeList* ConvertDeclRefExpr(clang::DeclRefExpr *clangDRE);

chillAST_NodeList* ConvertIntegerLiteral(clang::IntegerLiteral *clangIL);

chillAST_NodeList* ConvertFloatingLiteral(clang::FloatingLiteral *clangFL);

chillAST_NodeList* ConvertImplicitCastExpr(clang::ImplicitCastExpr *clangICE);

chillAST_NodeList* ConvertCStyleCastExpr(clang::CStyleCastExpr *clangICE);

chillAST_NodeList* ConvertReturnStmt(clang::ReturnStmt *clangRS);

chillAST_NodeList* ConvertCallExpr(clang::CallExpr *clangCE);

chillAST_NodeList* ConvertIfStmt(clang::IfStmt *clangIS);

chillAST_NodeList* ConvertMemberExpr(clang::MemberExpr *clangME);


chillAST_NodeList* ConvertTranslationUnit(clang::TranslationUnitDecl *TUD, char *filename);

/*!
 * \brief Convert fon Clang AST to CHiLL AST also append to parent node p if necessary
 *
 * @param s Clang statement
 * @return A set of new statements
 */
chillAST_NodeList* ConvertGenericClangAST(clang::Stmt *s);


std::vector<chillAST_VarDecl *> VariableDeclarations;
std::vector<chillAST_FunctionDecl *> FunctionDeclarations;

using namespace clang;
using namespace clang::driver;
using namespace omega;
using namespace std;


static string binops[] = {
    " ", " ",             // BO_PtrMemD, BO_PtrMemI,       // [C++ 5.5] Pointer-to-member operators.
    "*", "/", "%",        // BO_Mul, BO_Div, BO_Rem,       // [C99 6.5.5] Multiplicative operators.
    "+", "-",             // BO_Add, BO_Sub,               // [C99 6.5.6] Additive operators.
    "<<", ">>",           // BO_Shl, BO_Shr,               // [C99 6.5.7] Bitwise shift operators.
    "<", ">", "<=", ">=", // BO_LT, BO_GT, BO_LE, BO_GE,   // [C99 6.5.8] Relational operators.
    "==", "!=",           // BO_EQ, BO_NE,                 // [C99 6.5.9] Equality operators.
    "&",                  // BO_And,                       // [C99 6.5.10] Bitwise AND operator.
    "??",                 // BO_Xor,                       // [C99 6.5.11] Bitwise XOR operator.
    "|",                  // BO_Or,                        // [C99 6.5.12] Bitwise OR operator.
    "&&",                 // BO_LAnd,                      // [C99 6.5.13] Logical AND operator.
    "||",                 // BO_LOr,                       // [C99 6.5.14] Logical OR operator.
    "=", "*=",            // BO_Assign, BO_MulAssign,      // [C99 6.5.16] Assignment operators.
    "/=", "%=",           // BO_DivAssign, BO_RemAssign,
    "+=", "-=",           // BO_AddAssign, BO_SubAssign,
    "???", "???",         // BO_ShlAssign, BO_ShrAssign,
    "&&=", "???",         // BO_AndAssign, BO_XorAssign,
    "||=",                // BO_OrAssign,
    ","};                 // BO_Comma                      // [C99 6.5.17] Comma operator.


static string unops[] = {
    "++", "--",           // [C99 6.5.2.4] Postfix increment and decrement
    "++", "--",           // [C99 6.5.3.1] Prefix increment and decrement
    "@", "*",            // [C99 6.5.3.2] Address and indirection
    "+", "-",             // [C99 6.5.3.3] Unary arithmetic
    "~", "!",             // [C99 6.5.3.3] Unary arithmetic
    "__real", "__imag",   // "__real expr"/"__imag expr" Extension.
    "__extension"          // __extension__ marker.
};

// forward defs
SourceManager *globalSRCMAN;  // ugly. shame.

char *splitTypeInfo(char *underlyingtype);

chillAST_Node* unwrap(chillAST_NodeList* nl){
  chillAST_Node* n;
  if (!nl || !nl->size()) n = NULL;
  else n = (*nl)[0];
  delete nl;
  return n;
}

chillAST_NodeList* ConvertVarDecl(VarDecl *D) {
  bool isParm = false;

  QualType T0 = D->getType();
  QualType T = T0;
  if (ParmVarDecl *Parm = dyn_cast<ParmVarDecl>(D)) { // My GOD clang stinks
    T = Parm->getOriginalType();
    isParm = true;
  }

  char *vartype = strdup(T.getAsString().c_str());
  char *arraypart = splitTypeInfo(vartype);

  char *varname = strdup(D->getName().str().c_str());

  chillAST_VarDecl *chillVD = new chillAST_VarDecl(vartype, varname, arraypart, (void *) D);

  chillVD->isAParameter = isParm;

  int numdim = 0;
  chillVD->knownArraySizes = true;
  if (index(vartype, '*')) chillVD->knownArraySizes = false;  // float *a;   for example
  if (index(arraypart, '*')) chillVD->knownArraySizes = false;

  // note: vartype here, arraypart in next code..    is that right?
  if (index(vartype, '*')) {
    for (int i = 0; i < strlen(vartype); i++) if (vartype[i] == '*') numdim++;
    chillVD->numdimensions = numdim;
  }

  if (index(arraypart, '[')) {  // JUST [12][34][56]  no asterisks
    char *dupe = strdup(arraypart);

    int len = strlen(arraypart);
    for (int i = 0; i < len; i++) if (dupe[i] == '[') numdim++;

    chillVD->numdimensions = numdim;
    int *as = (int *) malloc(sizeof(int *) * numdim);
    if (!as) {
      fprintf(stderr, "can't malloc array sizes in ConvertVarDecl()\n");
      exit(-1);
    }
    chillVD->arraysizes = as; // 'as' changed later!


    char *ptr = dupe;
    while (ptr = index(ptr, '[')) {
      ptr++;
      int dim;
      sscanf(ptr, "%d", &dim);
      *as++ = dim;

      ptr = index(ptr, ']');
    }
    free(dupe);
  }

  Expr *Init = D->getInit();
  if (Init) {
    fprintf(stderr, " = VARDECL HAS INIT.  (TODO) (RIGHT NOW)");
    exit(-1);
  }

  free(vartype);
  free(varname);

  VariableDeclarations.push_back(chillVD);

  NL_RET(chillVD);
}


chillAST_NodeList* ConvertRecordDecl(clang::RecordDecl *RD) { // for structs and unions

  //fprintf(stderr, "ConvertRecordDecl(  )\n\nclang sees\n");
  //RD->dump();
  //fflush(stdout); 
  //fprintf(stderr, "\n"); 

  //fprintf(stderr, "%s with name %s\n", ((clang::Decl *)RD)->getDeclKindName(), RD->getNameAsString().c_str());  
  //const clang::ASTRecordLayout RL = RD->getASTContext().getASTRecordLayout( RD );
  //RD->getASTContext().DumpRecordLayout( RD , cout ); 

  int count = 0;
  for (clang::RecordDecl::field_iterator fi = RD->field_begin(); fi != RD->field_end(); fi++) count++;
  //fprintf(stderr, "%d fields in this struct/union\n", count); 

  char blurb[128];
  sprintf(blurb, "struct %s", RD->getNameAsString().c_str());
  fprintf(stderr, "blurb is '%s'\n", blurb);

  chillAST_TypedefDecl *astruct = new chillAST_TypedefDecl(blurb, "", NULL);
  astruct->setStruct(true);
  astruct->setStructName(RD->getNameAsString().c_str());

  for (clang::RecordDecl::field_iterator fi = RD->field_begin(); fi != RD->field_end(); fi++) {
    clang::FieldDecl *FD = (*fi);
    FD->dump();
    printf(";\n");
    fflush(stdout);
    string TypeStr = FD->getType().getAsString();

    const char *typ = TypeStr.c_str();
    const char *name = FD->getNameAsString().c_str();
    fprintf(stderr, "(typ) %s (name) %s\n", typ, name);

    chillAST_VarDecl *VD = NULL;
    // very clunky and incomplete
    VD = new chillAST_VarDecl(typ, name, "", astruct); // can't handle arrays yet

    astruct->subparts.push_back(VD);
  }


  fprintf(stderr, "I just defined a struct\n");
  astruct->print(0, stderr);

  NL_RET(astruct);
}


chillAST_NodeList* ConvertTypeDefDecl(TypedefDecl *TDD) {
  //fprintf(stderr, "ConvertTypedefDecl(  ) \n");
  //fprintf(stderr, "TDD has type %s\n", TDD->getDeclKindName()); 
  //TDD->dump(); fprintf(stderr, "\n"); 

  char *under = strdup(TDD->getUnderlyingType().getAsString().c_str());
  //fprintf(stderr, "under = '%s'\n", under); 
  char *arraypart = splitTypeInfo(under);
  //fprintf(stderr, "typedef %s %s%s;\n", under, TDD->getName().str().c_str(), arraypart);
  //  fprintf(stderr, "len arraypart = %d\n", strlen(arraypart)); 
  char *alias = strdup(TDD->getName().str().c_str());

  //fprintf(stderr, "underlying type %s  arraypart '%s'  name %s\n", under, arraypart, TDD->getName().str().c_str() ); 
  chillAST_TypedefDecl *CTDD = new chillAST_TypedefDecl(under, alias, arraypart);

  free(under);
  free(arraypart);

  NL_RET(CTDD);
}


chillAST_NodeList* ConvertDeclStmt(DeclStmt *clangDS) {
  //fprintf(stderr, "ConvertDeclStmt()\n"); 

  chillAST_VarDecl *chillvardecl; // the thing we'll return if this is a single declaration

  bool multiples = !clangDS->isSingleDecl();

  DeclGroupRef dgr = clangDS->getDeclGroup();
  clang::DeclGroupRef::iterator DI = dgr.begin();
  clang::DeclGroupRef::iterator DE = dgr.end();
  chillAST_NodeList* decls = new chillAST_NodeList();

  for (; DI != DE; ++DI) {
    Decl *D = *DI;
    const char *declty = D->getDeclKindName();
    //fprintf(stderr, "a decl of type %s\n", D->getDeclKindName()); 

    if (!strcmp("Var", declty)) {
      VarDecl *V = dyn_cast<VarDecl>(D);
      // ValueDecl *VD = dyn_cast<ValueDecl>(D); // not needed? 
      std::string Name = V->getNameAsString();
      char *varname = strdup(Name.c_str());

      //fprintf(stderr, "variable named %s\n", Name.c_str()); 
      QualType T = V->getType();
      string TypeStr = T.getAsString();
      char *vartype = strdup(TypeStr.c_str());

      CHILL_DEBUG_PRINT("DeclStmt (clang 0x%x) for %s %s\n", D, vartype,  varname);
      //fprintf(stderr, "%s %s\n", td, varname);
      char *arraypart = splitTypeInfo(vartype);

      chillvardecl = new chillAST_VarDecl(vartype, varname, arraypart, (void *) D);

      // store this away for declrefexpr that references it! 
      VariableDeclarations.push_back(chillvardecl);
      decls->push_back(chillvardecl);

      // TODO
      if (V->hasInit()) {
        CHILL_ERROR(" ConvertDeclStmt()  UNHANDLED initialization\n");
        exit(-1);
      }
    }
  }  // for each of possibly multiple decls 

  return decls;
}


chillAST_NodeList* ConvertCompoundStmt(CompoundStmt *clangCS) {
  int numchildren = clangCS->size();

  // make an empty CHILL compound statement 
  chillAST_CompoundStmt *chillCS = new chillAST_CompoundStmt;

  // for each clang child
  for (auto I = clangCS->child_begin(); I != clangCS->child_end(); ++I) { // ?? loop looks WRONG
    // create the chill ast for each child
    Stmt *child = *I;
    chillAST_NodeList* nl = ConvertGenericClangAST(child);
    // usually n will be a statement. We just add it as a child.
    // SOME DeclStmts have multiple declarations. They will add themselves and return NULL
    chillCS->addChildren(*nl);
    delete nl;
  }

  NL_RET(chillCS);
}


chillAST_NodeList* ConvertFunctionDecl(FunctionDecl *D) {
  QualType QT = D->getReturnType();
  string ReturnTypeStr = QT.getAsString();

  // Function name
  DeclarationName DeclName = D->getNameInfo().getName();
  string FuncName = DeclName.getAsString();

  chillAST_FunctionDecl *chillFD = new chillAST_FunctionDecl(ReturnTypeStr.c_str(), FuncName.c_str(), D);


  int numparams = D->getNumParams();

  CHILL_DEBUG_PRINT(" %d parameters\n", numparams);
  for (int i = 0; i < numparams; i++) {
    VarDecl *clangvardecl = D->getParamDecl(i);  // the ith parameter  (CLANG)
    ParmVarDecl *pvd = D->getParamDecl(i);
    QualType T = pvd->getOriginalType();
    CHILL_DEBUG_PRINT("OTYPE %s\n", T.getAsString().c_str());

    chillAST_NodeList* nl = ConvertVarDecl(clangvardecl);
    chillAST_VarDecl* decl = (chillAST_VarDecl*)unwrap(nl);
    //chillPVD->print();  fflush(stdout); 

    //chillPVD->isAParameter = 1;
    VariableDeclarations.push_back(decl);

    chillFD->addParameter(decl);
    CHILL_DEBUG_PRINT("chillAST ParmVarDecl for %s from chill location 0x%x\n", decl->varname, clangvardecl);
  } // for each parameter 



  //fprintf(stderr, ")\n{\n"); // beginning of function body 
  //if (D->isExternC())    { chillFD->setExtern();  fprintf(stderr, "%s is extern\n", FuncName.c_str()); }; 
  if (D->getBuiltinID()) {
    chillFD->setExtern();
    CHILL_DEBUG_PRINT("%s is builtin (extern)\n", FuncName.c_str());
  };

  Stmt *clangbody = D->getBody();
  if (clangbody) { // may just be fwd decl or external, without an actual body 
    //fprintf(stderr, "body of type %s\n", clangbody->getStmtClassName()); 
    //chillAST_Node *CB = ConvertCompoundStmt(  dyn_cast<CompoundStmt>(clangbody) ); // always a compound statement?
    chillAST_NodeList* nl = ConvertGenericClangAST(clangbody);
    //fprintf(stderr, "FunctionDecl body = 0x%x of type %s\n", CB, CB->getTypeString());
    chillFD->setBody(unwrap(nl));
  }

  //fprintf(stderr, "adding function %s  0x%x to FunctionDeclarations\n", chillFD->functionName, chillFD); 
  FunctionDeclarations.push_back(chillFD);
  NL_RET(chillFD);
}


chillAST_NodeList* ConvertForStmt(ForStmt *clangFS) {

  chillAST_Node *init = unwrap(ConvertGenericClangAST(clangFS->getInit()));
  chillAST_Node *cond = unwrap(ConvertGenericClangAST(clangFS->getCond()));
  chillAST_Node *incr = unwrap(ConvertGenericClangAST(clangFS->getInc()));
  chillAST_Node *body = unwrap(ConvertGenericClangAST(clangFS->getBody()));
  if (body->getType() != CHILLAST_NODE_COMPOUNDSTMT) {
    // make single statement loop bodies loop like other loops
    CHILL_DEBUG_PRINT("ForStmt body of type %s\n", body->getTypeString());
    chillAST_CompoundStmt *cs = new chillAST_CompoundStmt();
    cs->addChild(body);
    body = cs;
  }
  chillAST_ForStmt *chill_loop = new chillAST_ForStmt(init, cond, incr, body);
  NL_RET(chill_loop);
}


chillAST_NodeList* ConvertIfStmt(IfStmt *clangIS) {
  Expr *cond = clangIS->getCond();
  Stmt *thenpart = clangIS->getThen();
  Stmt *elsepart = clangIS->getElse();

  chillAST_Node *con = unwrap(ConvertGenericClangAST(cond));
  chillAST_Node *thn = NULL;
  if (thenpart) {
    thn = unwrap(ConvertGenericClangAST(thenpart));
    if (!thn->isCompoundStmt()) {
      chillAST_Node* tmp=new chillAST_CompoundStmt();
      tmp->addChild(thn);
      thn = tmp;
    }
  }
  chillAST_Node *els = NULL;
  if (elsepart) {
    els = unwrap(ConvertGenericClangAST(elsepart));
    if (!els->isCompoundStmt()) {
      chillAST_Node* tmp=new chillAST_CompoundStmt();
      tmp->addChild(els);
      els = tmp;
    }
  }

  chillAST_IfStmt *ifstmt = new chillAST_IfStmt(con, thn, els);
  NL_RET(ifstmt);
}


chillAST_NodeList* ConvertUnaryOperator(UnaryOperator *clangUO) {
  const char *op = unops[clangUO->getOpcode()].c_str();
  bool pre = clangUO->isPrefix();
  chillAST_Node *sub = unwrap(ConvertGenericClangAST(clangUO->getSubExpr()));

  chillAST_UnaryOperator *chillUO = new chillAST_UnaryOperator(op, pre, sub);
  NL_RET(chillUO);
}


chillAST_NodeList* ConvertBinaryOperator(BinaryOperator *clangBO) {

  // get the clang parts
  Expr *lhs = clangBO->getLHS();
  Expr *rhs = clangBO->getRHS();
  BinaryOperator::Opcode op = clangBO->getOpcode(); // this is CLANG op, not CHILL op


  // convert to chill equivalents
  chillAST_Node *l = unwrap(ConvertGenericClangAST(lhs));
  const char *opstring = binops[op].c_str();
  chillAST_Node *r = unwrap(ConvertGenericClangAST(rhs));
  // TODO chill equivalent for numeric op. 

  // build up the chill Binary Op AST node
  chillAST_BinaryOperator *binop = new chillAST_BinaryOperator(l, opstring, r);

  NL_RET(binop);
}


chillAST_NodeList* ConvertArraySubscriptExpr(ArraySubscriptExpr *clangASE) {

  Expr *clangbase = clangASE->getBase();
  Expr *clangindex = clangASE->getIdx();
  //fprintf(stderr, "clang base: "); clangbase->dump(); fprintf(stderr, "\n"); 

  chillAST_Node *bas = unwrap(ConvertGenericClangAST(clangbase));
  chillAST_Node *indx = unwrap(ConvertGenericClangAST(clangindex));

  // TODO clangAST contamination
  chillAST_ArraySubscriptExpr *chillASE = new chillAST_ArraySubscriptExpr(bas, indx, NULL, clangASE);
  NL_RET(chillASE);
}


chillAST_NodeList* ConvertDeclRefExpr(DeclRefExpr *clangDRE) {
  DeclarationNameInfo DNI = clangDRE->getNameInfo();

  ValueDecl *vd = static_cast<ValueDecl *>(clangDRE->getDecl()); // ValueDecl ?? VarDecl ??

  QualType QT = vd->getType();
  string TypeStr = QT.getAsString();
  //fprintf(stderr, "\n\n*** type %s ***\n\n", TypeStr.c_str()); 
  //fprintf(stderr, "kind %s\n", vd->getDeclKindName()); 

  DeclarationName DN = DNI.getName();
  const char *varname = DN.getAsString().c_str();
  chillAST_DeclRefExpr *chillDRE = new chillAST_DeclRefExpr(TypeStr.c_str(), varname, NULL);

  //fprintf(stderr, "clang DeclRefExpr refers to declaration of %s @ 0x%x\n", varname, vd);
  //fprintf(stderr, "clang DeclRefExpr refers to declaration of %s of kind %s\n", varname, vd->getDeclKindName()); 

  // find the definition (we hope)
  if ((!strcmp("Var", vd->getDeclKindName())) || (!strcmp("ParmVar", vd->getDeclKindName()))) {
    // it's a variable reference 
    int numvars = VariableDeclarations.size();
    chillAST_VarDecl *chillvd = NULL;
    for (int i = 0; i < numvars; i++) {
      if (VariableDeclarations[i]->uniquePtr == vd) {
        chillvd = VariableDeclarations[i];
        //fprintf(stderr, "found it at variabledeclaration %d of %d\n", i, numvars);
      }
    }
    if (!chillvd) {
      fprintf(stderr, "\nWARNING, ir_clang.cc clang DeclRefExpr %s refers to a declaration I can't find! at ox%x\n",
              varname, vd);
      fprintf(stderr, "variables I know of are:\n");
      for (int i = 0; i < numvars; i++) {
        chillAST_VarDecl *adecl = VariableDeclarations[i];
        if (adecl->isParmVarDecl()) fprintf(stderr, "(parameter) ");
        fprintf(stderr, "%s %s at location 0x%x\n", adecl->vartype, adecl->varname, adecl->uniquePtr);
      }
      fprintf(stderr, "\n");
    }

    if (chillvd == NULL) {
      fprintf(stderr, "chillDRE->decl = 0x%x\n", chillvd);
      exit(-1);
    }

    chillDRE->decl = (chillAST_Node *) chillvd; // start of spaghetti pointers ...
  } else if (!strcmp("Function", vd->getDeclKindName())) {
    //fprintf(stderr, "declrefexpr of type Function\n");
    int numfuncs = FunctionDeclarations.size();
    chillAST_FunctionDecl *chillfd = NULL;
    for (int i = 0; i < numfuncs; i++) {
      if (FunctionDeclarations[i]->uniquePtr == vd) {
        chillfd = FunctionDeclarations[i];
        //fprintf(stderr, "found it at functiondeclaration %d of %d\n", i, numfuncs);
      }
    }
    if (chillfd == NULL) {
      fprintf(stderr, "chillDRE->decl = 0x%x\n", chillfd);
      exit(-1);
    }

    chillDRE->decl = (chillAST_Node *) chillfd; // start of spaghetti pointers ...

  } else {
    fprintf(stderr, "clang DeclRefExpr refers to declaration of %s of kind %s\n", varname, vd->getDeclKindName());
    fprintf(stderr, "chillDRE->decl = UNDEFINED\n");
    exit(-1);
  }

  //fprintf(stderr, "%s\n", DN.getAsString().c_str());
  NL_RET(chillDRE);
}


chillAST_NodeList* ConvertIntegerLiteral(IntegerLiteral *clangIL) {
  bool isSigned = clangIL->getType()->isSignedIntegerType();
  //int val = clangIL->getIntValue();
  const char *printable = clangIL->getValue().toString(10, isSigned).c_str();
  int val = atoi(printable);
  //fprintf(stderr, "int value %s  (%d)\n", printable, val); 
  chillAST_IntegerLiteral *chillIL = new chillAST_IntegerLiteral(val);
  NL_RET(chillIL);
}


chillAST_NodeList* ConvertFloatingLiteral(FloatingLiteral *clangFL) {
  //fprintf(stderr, "\nConvertFloatingLiteral()\n"); 
  float val = clangFL->getValueAsApproximateDouble(); // TODO approx is a bad idea!
  string WHAT;
  SmallString<16> Str;
  clangFL->getValue().toString(Str);
  const char *printable = Str.c_str();
  //fprintf(stderr, "literal %s\n", printable); 

  SourceLocation sloc = clangFL->getLocStart();
  SourceLocation eloc = clangFL->getLocEnd();

  std::string start = sloc.printToString(*globalSRCMAN);
  std::string end = eloc.printToString(*globalSRCMAN);
  //fprintf(stderr, "literal try2 start %s end %s\n", start.c_str(), end.c_str()); 
  //printlines( sloc, eloc, globalSRCMAN ); 
  unsigned int startlineno = globalSRCMAN->getPresumedLineNumber(sloc);
  unsigned int endlineno = globalSRCMAN->getPresumedLineNumber(eloc);;
  const char *filename = globalSRCMAN->getBufferName(sloc);

  std::string fname = globalSRCMAN->getFilename(sloc);
  //fprintf(stderr, "fname %s\n", fname.c_str()); 

  if (filename && strlen(filename) > 0) {} // fprintf(stderr, "literal file '%s'\n", filename);
  else {
    fprintf(stderr, "\nConvertFloatingLiteral() filename is NULL?\n");

    //sloc =  globalSRCMAN->getFileLoc( sloc );  // should get spelling loc? 
    sloc = globalSRCMAN->getSpellingLoc(sloc);  // should get spelling loc?
    //eloc =  globalSRCMAN->getFileLoc( eloc );

    start = sloc.printToString(*globalSRCMAN);
    //end   = eloc.printToString( *globalSRCMAN );  
    //fprintf(stderr, "literal try3 start %s end %s\n", start.c_str(), end.c_str()); 

    startlineno = globalSRCMAN->getPresumedLineNumber(sloc);
    //endlineno = globalSRCMAN->getPresumedLineNumber( eloc ); ;    
    //fprintf(stderr, "start, end line numbers %d %d\n", startlineno, endlineno); 

    filename = globalSRCMAN->getBufferName(sloc);

    //if (globalSRCMAN->isMacroBodyExpansion( sloc )) { 
    //  fprintf(stderr, "IS MACRO\n");
    //} 
  }

  unsigned int offset = globalSRCMAN->getFileOffset(sloc);
  //fprintf(stderr, "literal file offset %d\n", offset); 

  FILE *fp = fopen(filename, "r");
  fseek(fp, offset, SEEK_SET); // go to the part of the file where the float is defined

  char buf[10240];
  fgets(buf, sizeof(buf), fp); // read a line starting where the float starts
  fclose(fp);

  // buf has the line we want   grab the float constant out of it
  //fprintf(stderr, "\nbuf '%s'\n", buf);
  char *ptr = buf;
  if (*ptr == '-') ptr++; // ignore possible minus sign
  int len = strspn(ptr, ".-0123456789f");
  buf[len] = '\0';
  //fprintf(stderr, "'%s'\n", buf);

  chillAST_FloatingLiteral *chillFL = new chillAST_FloatingLiteral(val,1, buf);

  //chillFL->print(); printf("\n"); fflush(stdout);
  NL_RET(chillFL);
}


chillAST_NodeList* ConvertImplicitCastExpr(ImplicitCastExpr *clangICE) {
  //fprintf(stderr, "ConvertImplicitCastExpr()\n"); 
  CastExpr *CE = dyn_cast<ImplicitCastExpr>(clangICE);
  //fprintf(stderr, "implicit cast of type %s\n", CE->getCastKindName());
  chillAST_Node *sub = unwrap(ConvertGenericClangAST(clangICE->getSubExpr()));
  chillAST_ImplicitCastExpr *chillICE = new chillAST_ImplicitCastExpr(sub);

  //sub->setParent( chillICE ); // these 2 lines work
  //return chillICE; 

  //sub->setParent(p);         // ignore the ImplicitCastExpr !!  TODO (probably a bad idea)
  NL_RET(chillICE);
}


chillAST_NodeList* ConvertCStyleCastExpr(CStyleCastExpr *clangCSCE) {
  CastExpr *CE = dyn_cast<CastExpr>(clangCSCE);

  const char *towhat = strdup(clangCSCE->getTypeAsWritten().getAsString().c_str());

  chillAST_Node *sub = unwrap(ConvertGenericClangAST(clangCSCE->getSubExprAsWritten()));
  chillAST_CStyleCastExpr *chillCSCE = new chillAST_CStyleCastExpr(towhat, sub);
  NL_RET(chillCSCE);
}


chillAST_NodeList* ConvertReturnStmt(ReturnStmt *clangRS) {
  chillAST_Node *retval = unwrap(ConvertGenericClangAST(clangRS->getRetValue())); // NULL is handled
  //if (retval == NULL) fprintf(stderr, "return stmt returns nothing\n");

  chillAST_ReturnStmt *chillRS = new chillAST_ReturnStmt(retval);
  NL_RET(chillRS);
}


chillAST_NodeList* ConvertCallExpr(CallExpr *clangCE) {
  //fprintf(stderr, "ConvertCallExpr()\n"); 

  chillAST_Node *callee = unwrap(ConvertGenericClangAST(clangCE->getCallee()));
  //fprintf(stderr, "callee is of type %s\n", callee->getTypeString()); 

  //chillAST_Node *next = ((chillAST_ImplicitCastExpr *)callee)->subexpr;
  //fprintf(stderr, "callee is of type %s\n", next->getTypeString()); 

  chillAST_CallExpr *chillCE = new chillAST_CallExpr(callee);

  int numargs = clangCE->getNumArgs();
  //fprintf(stderr, "CallExpr has %d args\n", numargs);
  Expr **clangargs = clangCE->getArgs();
  for (int i = 0; i < numargs; i++) {
    chillCE->addArg(unwrap(ConvertGenericClangAST(clangargs[i])));
  }

  NL_RET(chillCE);
}


chillAST_NodeList* ConvertParenExpr(ParenExpr *clangPE) {
  chillAST_Node *sub = unwrap(ConvertGenericClangAST(clangPE->getSubExpr()));
  chillAST_ParenExpr *chillPE = new chillAST_ParenExpr(sub);

  NL_RET(chillPE);
}


chillAST_NodeList* ConvertTranslationUnit(TranslationUnitDecl *TUD, char *filename) {
  static DeclContext *DC = TUD->castToDeclContext(TUD);

  chillAST_SourceFile *topnode = new chillAST_SourceFile(filename);
  topnode->setFrontend("clang");
  topnode->chill_array_counter = 1;
  topnode->chill_scalar_counter = 0;

  // now recursively build clang AST from the children of TUD
  DeclContext::decl_iterator start = DC->decls_begin();
  DeclContext::decl_iterator end = DC->decls_end();
  for (DeclContext::decl_iterator DI = start; DI != end; ++DI) {
    Decl *D = *DI;
    chillAST_Node *child;
    // Skip internal declarations of clang

    if (isa<FunctionDecl>(D)) {
      child = unwrap(ConvertFunctionDecl(dyn_cast<FunctionDecl>(D)));
    } else if (isa<VarDecl>(D)) {
      child = unwrap(ConvertVarDecl(dyn_cast<VarDecl>(D)));
    } else if (isa<TypedefDecl>(D)) {
      child = unwrap(ConvertTypeDefDecl(dyn_cast<TypedefDecl>(D)));
    } else if (isa<RecordDecl>(D)) {
      CHILL_DEBUG_PRINT("\nTUD RecordDecl\n");
      child = unwrap(ConvertRecordDecl(dyn_cast<RecordDecl>(D)));
    } else if (isa<TypeAliasDecl>(D)) {
      CHILL_ERROR("TUD TypeAliasDecl  TODO \n");
      exit(-1);
    } else {
      CHILL_ERROR("\nTUD a declaration of type %s (%d) which I can't handle\n", D->getDeclKindName(), D->getKind());
      exit(-1);
    }
    topnode -> addChild(child);
    if (D->isImplicit()) child->isFromSourceFile = false;
  }

  NL_RET(topnode);
}


chillAST_NodeList* ConvertGenericClangAST(Stmt *s) {

  if (s == NULL) return NULL;
  Decl *D = (Decl *) s;

  chillAST_NodeList *ret = NULL;    // find the SINGLE constant or data reference at this node or below

  if (isa<CompoundStmt>(s)) {
    ret = ConvertCompoundStmt(dyn_cast<CompoundStmt>(s));
  } else if (isa<DeclStmt>(s)) {
    ret = ConvertDeclStmt(dyn_cast<DeclStmt>(s));
  } else if (isa<ForStmt>(s)) {
    ret = ConvertForStmt(dyn_cast<ForStmt>(s));
  } else if (isa<BinaryOperator>(s)) {
    ret = ConvertBinaryOperator(dyn_cast<BinaryOperator>(s));
  } else if (isa<ArraySubscriptExpr>(s)) {
    ret = ConvertArraySubscriptExpr(dyn_cast<ArraySubscriptExpr>(s));
  } else if (isa<DeclRefExpr>(s)) {
    ret = ConvertDeclRefExpr(dyn_cast<DeclRefExpr>(s));
  } else if (isa<FloatingLiteral>(s)) {
    ret = ConvertFloatingLiteral(dyn_cast<FloatingLiteral>(s));
  } else if (isa<IntegerLiteral>(s)) {
    ret = ConvertIntegerLiteral(dyn_cast<IntegerLiteral>(s));
  } else if (isa<UnaryOperator>(s)) {
    ret = ConvertUnaryOperator(dyn_cast<UnaryOperator>(s));
  } else if (isa<ImplicitCastExpr>(s)) {
    ret = ConvertImplicitCastExpr(dyn_cast<ImplicitCastExpr>(s));
  } else if (isa<CStyleCastExpr>(s)) {
    ret = ConvertCStyleCastExpr(dyn_cast<CStyleCastExpr>(s));
  } else if (isa<ReturnStmt>(s)) {
    ret = ConvertReturnStmt(dyn_cast<ReturnStmt>(s));
  } else if (isa<CallExpr>(s)) {
    ret = ConvertCallExpr(dyn_cast<CallExpr>(s));
  } else if (isa<ParenExpr>(s)) {
    ret = ConvertParenExpr(dyn_cast<ParenExpr>(s));
  } else if (isa<IfStmt>(s)) {
    ret = ConvertIfStmt(dyn_cast<IfStmt>(s));
  } else if (isa<MemberExpr>(s)) {
    ret = ConvertMemberExpr(dyn_cast<MemberExpr>(s));


    // these can only happen at the top level?
    //   } else if (isa<FunctionDecl>(D))       { ret = ConvertFunctionDecl( dyn_cast<FunctionDecl>(D));
    //} else if (isa<VarDecl>(D))            { ret =      ConvertVarDecl( dyn_cast<VarDecl>(D) );
    //} else if (isa<TypedefDecl>(D))        { ret =  ConvertTypeDefDecl( dyn_cast<TypedefDecl>(D));
    //  else if (isa<TranslationUnitDecl>(s))  // need filename




    //   } else if (isa<>(s))                  {         Convert ( dyn_cast<>(s));

    /*
    */

  } else {
    // more work to do
    fprintf(stderr, "ir_clang.cc ConvertGenericClangAST() UNHANDLED ");
    //if (isa<Decl>(D)) fprintf(stderr, "Decl of kind %s\n",  D->getDeclKindName() );
    if (isa<Stmt>(s))fprintf(stderr, "Stmt of type %s\n", s->getStmtClassName());
    exit(-1);
  }

  return ret;
}










// ----------------------------------------------------------------------------
// Class: IR_chillScalarSymbol
// ----------------------------------------------------------------------------

std::string IR_chillScalarSymbol::name() const {
  //return vd_->getNameAsString();   CLANG
  //fprintf(stderr, "IR_chillScalarSymbol::name() %s\n", chillvd->varname); 
  return std::string(chillvd->varname);  // CHILL 
}


// Return size in bytes
int IR_chillScalarSymbol::size() const {
  //return (vd_->getASTContext().getTypeSize(vd_->getType())) / 8;    // ??
  fprintf(stderr, "IR_chillScalarSymbol::size()  probably WRONG\n");
  return (8); // bytes?? 
}


bool IR_chillScalarSymbol::operator==(const IR_Symbol &that) const {
  //fprintf(stderr, "IR_xxxxScalarSymbol::operator==  probably WRONG\n"); 
  if (typeid(*this) != typeid(that))
    return false;

  const IR_chillScalarSymbol *l_that = static_cast<const IR_chillScalarSymbol *>(&that);
  return this->chillvd == l_that->chillvd;
}

IR_Symbol *IR_chillScalarSymbol::clone() const {
  return new IR_chillScalarSymbol(ir_, chillvd);  // clone
}

// ----------------------------------------------------------------------------
// Class: IR_chillArraySymbol
// ----------------------------------------------------------------------------

std::string IR_chillArraySymbol::name() const {
  return std::string(strdup(chillvd->varname));
}


int IR_chillArraySymbol::elem_size() const {
  fprintf(stderr, "IR_chillArraySymbol::elem_size()  TODO\n");
  exit(-1);
  return 8;  // TODO 
  //const ArrayType *at = dyn_cast<ArrayType>(vd_->getType());
  //if(at) {
  //  return (vd_->getASTContext().getTypeSize(at->getElementType())) / 8;
  //} else
  //  throw chill::error::ir("Symbol is not an array!");
  //return 0;
}


int IR_chillArraySymbol::n_dim() const {
  //fprintf(stderr, "IR_chillArraySymbol::n_dim()\n");
  //fprintf(stderr, "variable %s %s %s\n", chillvd->vartype, chillvd->varname, chillvd->arraypart);
  //fprintf(stderr, "IR_chillArraySymbol::n_dim() %d\n", chillvd->numdimensions); 
  //fprintf(stderr, "IR_chillArraySymbol::n_dim()  TODO \n"); exit(-1); 
  return chillvd->numdimensions;
}


// TODO
omega::CG_outputRepr *IR_chillArraySymbol::size(int dim) const {
  fprintf(stderr, "IR_chillArraySymbol::n_size()  TODO \n");
  exit(-1);
  return NULL;
}


bool IR_chillArraySymbol::operator!=(const IR_Symbol &that) const {
  //fprintf(stderr, "IR_xxxxArraySymbol::operator!=   NOT EQUAL\n"); 
  //chillAST_VarDecl *chillvd;
  return chillvd != ((IR_chillArraySymbol *) &that)->chillvd;
}

bool IR_chillArraySymbol::operator==(const IR_Symbol &that) const {
  //fprintf(stderr, "IR_xxxxArraySymbol::operator==   EQUAL\n"); 
  //chillAST_VarDecl *chillvd;
  return chillvd == ((IR_chillArraySymbol *) &that)->chillvd;
  /*
  if (typeid(*this) != typeid(that))
    return false;
  
  const IR_chillArraySymbol *l_that = static_cast<const IR_chillArraySymbol *>(&that);
  return this->vd_ == l_that->vd_ && this->offset_ == l_that->offset_;
  */
}


IR_Symbol *IR_chillArraySymbol::clone() const {
  return new IR_chillArraySymbol(ir_, chillvd, offset_);
}

// ----------------------------------------------------------------------------
// Class: IR_chillConstantRef
// ----------------------------------------------------------------------------

bool IR_chillConstantRef::operator==(const IR_Ref &that) const {
  if (typeid(*this) != typeid(that))
    return false;

  const IR_chillConstantRef *l_that = static_cast<const IR_chillConstantRef *>(&that);

  if (this->type_ != l_that->type_)
    return false;

  if (this->type_ == IR_CONSTANT_INT)
    return this->i_ == l_that->i_;
  else
    return this->f_ == l_that->f_;
}


omega::CG_outputRepr *IR_chillConstantRef::convert() {
  //assert(astContext_ != NULL);
  if (type_ == IR_CONSTANT_INT) {

    fprintf(stderr, "IR_chillConstantRef::convert() unimplemented\n");
    exit(-1);

    // TODO 
    /*
    BuiltinType *bint = new BuiltinType(BuiltinType::Int);
    IntegerLiteral *ilit = new (astContext_)IntegerLiteral(*astContext_, llvm::APInt(32, i_), bint->desugar(), SourceLocation());
    omega::CG_chillRepr *result = new omega::CG_chillRepr(ilit);
    delete this;
    return result;
    */
  } else
    throw chill::error::ir("constant type not supported");
}


IR_Ref *IR_chillConstantRef::clone() const {
  if (type_ == IR_CONSTANT_INT)
    return new IR_chillConstantRef(ir_, i_);
  else if (type_ == IR_CONSTANT_FLOAT)
    return new IR_chillConstantRef(ir_, f_);
  else
    throw chill::error::ir("constant type not supported");
}

// ----------------------------------------------------------------------------
// Class: IR_chillScalarRef
// ----------------------------------------------------------------------------

bool IR_chillScalarRef::is_write() const {
  return op_pos_ == OP_DEST; // 2 other alternatives: OP_UNKNOWN, OP_SRC
}


IR_ScalarSymbol *IR_chillScalarRef::symbol() const {
  //VarDecl *vd = static_cast<VarDecl *>(vs_->getDecl());
  //fprintf(stderr, "ir_clang.cc  IR_chillScalarRef::symbol()\n"); //exit(-1);
  chillAST_VarDecl *vd = NULL;
  if (chillvd) vd = chillvd;
  return new IR_chillScalarSymbol(ir_, vd); // IR_chillScalarRef::symbol()
}


bool IR_chillScalarRef::operator==(const IR_Ref &that) const {
  if (typeid(*this) != typeid(that))
    return false;

  const IR_chillScalarRef *l_that = static_cast<const IR_chillScalarRef *>(&that);

  return this->chillvd == l_that->chillvd;
}


omega::CG_outputRepr *IR_chillScalarRef::convert() {
  //fprintf(stderr, "IR_chillScalarRef::convert() unimplemented\n"); exit(-1); 
  if (!dre) fprintf(stderr, "IR_chillScalarRef::convert()   CLANG SCALAR REF has no dre\n");
  omega::CG_chillRepr *result = new omega::CG_chillRepr(dre);
  delete this;
  return result;
}

IR_Ref *IR_chillScalarRef::clone() const {
  if (dre) return new IR_chillScalarRef(ir_, dre); // use declrefexpr if it exists
  return new IR_chillScalarRef(ir_, chillvd); // uses vardecl
}


// ----------------------------------------------------------------------------
// Class: IR_chillArrayRef
// ----------------------------------------------------------------------------

bool IR_chillArrayRef::is_write() const {

  return (iswrite); // TODO 
}


// TODO
omega::CG_outputRepr *IR_chillArrayRef::index(int dim) const {
  return new omega::CG_chillRepr(chillASE->getIndex(dim));
}


IR_ArraySymbol *IR_chillArrayRef::symbol() const {
  chillAST_Node *mb = chillASE->multibase();
  chillAST_VarDecl *vd = (chillAST_VarDecl *) mb;
  IR_ArraySymbol *AS = new IR_chillArraySymbol(ir_, vd);
  return AS;
/*
  chillAST_Node *b = chillASE->base;
  fprintf(stderr, "base of type %s\n", b->getTypeString());
  //b->print(); printf("\n"); fflush(stdout);
  if (b->getType() == CHILLAST_NODE_IMPLICITCASTEXPR) {
    b = ((chillAST_ImplicitCastExpr*)b)->subexpr;
    fprintf(stderr, "base of type %s\n", b->getTypeString());
  }

  if (b->getType() == CHILLAST_NODE_DECLREFEXPR)  {
    if (NULL == ((chillAST_DeclRefExpr*)b)->decl) {
      fprintf(stderr, "IR_chillArrayRef::symbol()  var decl = 0x%x\n", ((chillAST_DeclRefExpr*)b)->decl);
      exit(-1);
    }
    return new IR_chillArraySymbol(ir_, ((chillAST_DeclRefExpr*)b)->decl); // -> decl?
  }
  if (b->getType() ==  CHILLAST_NODE_ARRAYSUBSCRIPTEXPR)  { // multidimensional array
    return (
  }
  fprintf(stderr, "IR_chillArrayRef::symbol() can't handle\n");
  fprintf(stderr, "base of type %s\n", b->getTypeString());
  exit(-1);
  return NULL;
*/
}


bool IR_chillArrayRef::operator!=(const IR_Ref &that) const {
  bool op = (*this) == that; // opposite
  return !op;
}

bool IR_chillArrayRef::operator==(const IR_Ref &that) const {
  const IR_chillArrayRef *l_that = static_cast<const IR_chillArrayRef *>(&that);
  const chillAST_ArraySubscriptExpr *thatASE = l_that->chillASE;
  return (*chillASE) == (*thatASE);
  /*

  if (typeid(*this) != typeid(that))
    return false;
  
  const IR_chillArrayRef *l_that = static_cast<const IR_chillArrayRef *>(&that);
  
  return this->as_ == l_that->as_;
  */
}


omega::CG_outputRepr *IR_chillArrayRef::convert() {
  CG_chillRepr *result = new CG_chillRepr(chillASE->clone());
//  omega::CG_chillRepr *temp = new omega::CG_chillRepr(static_cast<Expr*>(this->as_));
//  omega::CG_outputRepr *result = temp->clone();
  delete this;
  return result;
}


IR_Ref *IR_chillArrayRef::clone() const {
  return new IR_chillArrayRef(ir_, chillASE, iswrite);
}


// ----------------------------------------------------------------------------
// Class: IR_chillLoop
// ----------------------------------------------------------------------------
IR_chillLoop::IR_chillLoop(const IR_Code *ir, clang::ForStmt *tf) {
  CHILL_ERROR("you lose\n");
  exit(-1);
};

IR_chillLoop::IR_chillLoop(const IR_Code *ir, chillAST_ForStmt *achillforstmt) {
  CHILL_DEBUG_BEGIN
    fprintf(stderr, "loop is:\n");
    achillforstmt->print();
  CHILL_DEBUG_END

  ir_ = ir;
  chillforstmt = achillforstmt;

  chillAST_BinaryOperator *init = (chillAST_BinaryOperator *) chillforstmt->getInit();
  chillAST_BinaryOperator *cond = (chillAST_BinaryOperator *) chillforstmt->getCond();
  // check to be sure  (assert) 
  if (!init->isAssignmentOp() || !cond->isComparisonOp()) {
    CHILL_ERROR("malformed loop init or cond:\n");
    achillforstmt->print();
    exit(-1);
  }

  chilllowerbound = init->getRHS();
  chillupperbound = cond->getRHS();
  conditionoperator = achillforstmt->conditionoperator;

  chillAST_Node *inc = chillforstmt->getInc();
  // check the increment
  if (inc->getType() == CHILLAST_NODE_UNARYOPERATOR) {
    if (!strcmp(((chillAST_UnaryOperator *) inc)->op, "++")) step_size_ = 1;
    else step_size_ = -1;
  } else if (inc->getType() == CHILLAST_NODE_BINARYOPERATOR) {
    int beets = false;  // slang
    chillAST_BinaryOperator *bop = (chillAST_BinaryOperator *) inc;
    if (bop->isAssignmentOp()) {        // I=I+1   or similar
      chillAST_Node *rhs = bop->getRHS();  // (I+1)
      // TODO looks like this will fail for I=1+I or I=J+1 etc. do more checking

      char *assop = bop->getOp();
      if (!strcmp(assop, "+=") || !strcmp(assop, "-=")) {
        chillAST_Node *stride = rhs;
        //fprintf(stderr, "stride is of type %s\n", stride->getTypeString());
        if (stride->isIntegerLiteral()) {
          int val = ((chillAST_IntegerLiteral *) stride)->value;
          if (!strcmp(assop, "+=")) step_size_ = val;
          else if (!strcmp(assop, "-=")) step_size_ = -val;
          else beets = true;
        } else beets = true;  // += or -= but not constant stride
      } else if (rhs->isBinaryOperator()) {
        chillAST_BinaryOperator *binoprhs = (chillAST_BinaryOperator *) rhs;
        chillAST_Node *intlit = binoprhs->getRHS();
        if (intlit->isIntegerLiteral()) {
          int val = ((chillAST_IntegerLiteral *) intlit)->value;
          if (!strcmp(binoprhs->getOp(), "+")) step_size_ = val;
          else if (!strcmp(binoprhs->getOp(), "-")) step_size_ = -val;
          else beets = true;
        } else beets = true;
      } else beets = true;
    } else beets = true;

    if (beets) {
      CHILL_ERROR("malformed loop increment (or more likely unhandled case)\n");
      inc->print();
      exit(-1);
    }
  } // binary operator 
  else {
    CHILL_ERROR("IR_chillLoop constructor, unhandled loop increment\n");
    inc->print();
    exit(-1);
  }
  //inc->print(0, stderr);fprintf(stderr, "\n"); 

  chillAST_DeclRefExpr *dre = (chillAST_DeclRefExpr *) init->getLHS();
  if (!dre->isDeclRefExpr()) {
    CHILL_DEBUG_PRINT("malformed loop init.\n");
    init->print();
  }

  chillindex = dre; // the loop index variable

  //fprintf(stderr, "\n\nindex is ");  dre->print(0, stderr);  fprintf(stderr, "\n"); 
  //fprintf(stderr, "init is   "); 
  //chilllowerbound->print(0, stderr);  fprintf(stderr, "\n");
  //fprintf(stderr, "condition is  %s ", "<"); 
  //chillupperbound->print(0, stderr);  fprintf(stderr, "\n");
  //fprintf(stderr, "step size is %d\n\n", step_size_) ; 

  chillbody = achillforstmt->getBody();

  CHILL_DEBUG_PRINT("IR_xxxxLoop::IR_xxxxLoop() DONE\n");
}


omega::CG_outputRepr *IR_chillLoop::lower_bound() const {
  CHILL_DEBUG_PRINT("IR_xxxxLoop::lower_bound()\n");
  return new omega::CG_chillRepr(chilllowerbound);
}

omega::CG_outputRepr *IR_chillLoop::upper_bound() const {
  CHILL_DEBUG_PRINT("IR_xxxxLoop::upper_bound()\n");
  return new omega::CG_chillRepr(chillupperbound);
}

IR_Block *IR_chillLoop::body() const {
  CHILL_DEBUG_PRINT("IR_xxxxLoop::body()\n");
  //assert(isa<CompoundStmt>(tf_->getBody()));
  //fprintf(stderr, "returning a clangBLOCK corresponding to the body of the loop\n"); 
  //fprintf(stderr, "body type %s\n", chillbody->getTypeString()); 
  return new IR_chillBlock(ir_, chillbody); // static_cast<CompoundStmt *>(tf_->getBody()));
}

IR_Control *IR_chillLoop::clone() const {
  CHILL_DEBUG_PRINT("IR_xxxxLoop::clone()\n");
  //chillforstmt->print(); fflush(stdout); 
  return new IR_chillLoop(ir_, chillforstmt);
}

IR_CONDITION_TYPE IR_chillLoop::stop_cond() const {
  chillAST_BinaryOperator *loopcondition = (chillAST_BinaryOperator *) chillupperbound;
  CHILL_DEBUG_PRINT("IR_xxxxLoop::stop_cond()\n");
  return conditionoperator;
}

IR_Block *IR_chillLoop::convert() { // convert the loop to a block 
  CHILL_DEBUG_PRINT("IR_xxxxLoop::convert()   maybe \n");
  return new IR_chillBlock(ir_, chillbody); // ??
  return NULL;
}

void IR_chillLoop::dump() const {
  CHILL_ERROR("TODO:  IR_chillLoop::dump()\n");
  exit(-1);
}


// ----------------------------------------------------------------------------
// Class: IR_chillBlock
// ----------------------------------------------------------------------------
omega::CG_outputRepr *IR_chillBlock::original() const {
  CHILL_ERROR("IR_xxxxBlock::original()  TODO \n");
  exit(-1);
  return NULL;
}


omega::CG_outputRepr *IR_chillBlock::extract() const {
  // if the block refers to a compound statement, return the next level
  // of statements ;  otherwise just return a repr of the statements
  omega::CG_chillRepr *OR;
  CHILL_DEBUG_PRINT("adding a statement from IR_chillBlock::extract()\n");
  OR = new omega::CG_chillRepr(); // empty of statements
  for (int i = 0; i < statements.size(); i++) OR->addStatement(statements[i]);
  CHILL_DEBUG_PRINT("IR_xxxxBlock::extract() LEAVING\n");
  return OR;
}

IR_Control *IR_chillBlock::clone() const {
  CHILL_DEBUG_PRINT("IR_xxxxBlock::clone()\n");
  return new IR_chillBlock(this);  // shallow copy ?
}

void IR_chillBlock::dump() const {
  CHILL_DEBUG_PRINT("IR_chillBlock::dump()  TODO\n");
  return;
}

//StmtList 
vector<chillAST_Node *> IR_chillBlock::getStmtList() const {
  CHILL_DEBUG_PRINT("IR_xxxxBlock::getStmtList()\n");
  return statements; // ?? 
}


void IR_chillBlock::addStatement(chillAST_Node *s) {
  statements.push_back(s);
}

class NULLASTConsumer : public ASTConsumer {
};


void findmanually(chillAST_Node *node, char *procname, std::vector<chillAST_Node *> &procs) {

  if (node->getType() == CHILLAST_NODE_FUNCTIONDECL) {
    char *name = ((chillAST_FunctionDecl *) node)->functionName;
    if (!strcmp(name, procname)) {
      procs.push_back(node);
      // quit recursing. probably not correct in some horrible case
      return;
    }
  }


  // this is where the children can be used effectively. 
  // we don't really care what kind of node we're at. We just check the node itself
  // and then its children is needed. 

  int numc = node->children.size();

  for (int i = 0; i < numc; i++) {
    findmanually(node->children[i], procname, procs);
  }
  return;
}

// ----------------------------------------------------------------------------
// Class: IR_clangCode_Global_Init
// ----------------------------------------------------------------------------

IR_clangCode_Global_Init *IR_clangCode_Global_Init::pinstance = 0;


IR_clangCode_Global_Init *IR_clangCode_Global_Init::Instance(char **argv) {
  if (pinstance == 0) {
    // this is the only way to create an IR_clangCode_Global_Init
    pinstance = new IR_clangCode_Global_Init;
    pinstance->ClangCompiler = new aClangCompiler(argv[1]);

  }
  return pinstance;
}


aClangCompiler::aClangCompiler(char *filename) {

  //fprintf(stderr, "making a clang compiler for file %s\n", filename);
  SourceFileName = strdup(filename);

  // Arguments to pass to the clang frontend
  std::vector<const char *> args;
  args.push_back(strdup(filename));

  // The compiler invocation needs a DiagnosticsEngine so it can report problems
  //IntrusiveRefCntPtr<DiagnosticOptions> DiagOpts = new DiagnosticOptions(); // temp
  diagnosticOptions = new DiagnosticOptions(); // private member of aClangCompiler

  pTextDiagnosticPrinter = new clang::TextDiagnosticPrinter(llvm::errs(),
                                                            diagnosticOptions); // private member of aClangCompiler

  //llvm::IntrusiveRefCntPtr<clang::DiagnosticIDs> DiagID(new clang::DiagnosticIDs());

  //clang::DiagnosticsEngine Diags(DiagID, &*DiagOpts, DiagClient);
  diagnosticsEngine = new clang::DiagnosticsEngine(diagID, diagnosticOptions, pTextDiagnosticPrinter);

  // Create the compiler invocation
  // This class is designed to represent an abstract "invocation" of the compiler, 
  // including data such as the include paths, the code generation options, 
  // the warning flags, and so on.   
  std::unique_ptr<clang::CompilerInvocation> CI(new clang::CompilerInvocation());
  //CI = new clang::CompilerInvocation;
  clang::CompilerInvocation::CreateFromArgs(*CI, &args[0], &args[0] + args.size(), *diagnosticsEngine);


  // Create the compiler instance
  Clang = new clang::CompilerInstance();  // TODO should have a better name ClangCompilerInstance


  // Get ready to report problems
  Clang->createDiagnostics(nullptr, true);
  //Clang.createDiagnostics(0, 0);


//#ifdef KIDDINGME
  //fprintf(stderr, "target\n");
  // Initialize target info with the default triple for our platform.
  //TargetOptions TO;
  //TO.Triple = llvm::sys::getDefaultTargetTriple();
  //TargetInfo *TI = TargetInfo::CreateTargetInfo( Clang->getDiagnostics(), TO);

  // the above causes core dumps, because clang is stupid and frees the target multiple times, corrupting memory
  targetOptions = std::make_shared<clang::TargetOptions>();
  targetOptions->Triple = llvm::sys::getDefaultTargetTriple();

  TargetInfo *pti = TargetInfo::CreateTargetInfo(Clang->getDiagnostics(), targetOptions);

  Clang->setTarget(pti);

//#endif


  // ?? 
  //fprintf(stderr, "filemgr\n");
  Clang->createFileManager();
  FileManager &FileMgr = Clang->getFileManager();
  fileManager = &FileMgr;

  //fprintf(stderr, "sourcemgr\n");
  Clang->createSourceManager(FileMgr);
  SourceManager &SourceMgr = Clang->getSourceManager();
  sourceManager = &SourceMgr; // ?? aclangcompiler copy
  globalSRCMAN = &SourceMgr; //  TODO   global bad

  Clang->setInvocation(CI.get()); // Replace the current invocation



  //fprintf(stderr, "PP\n");
  Clang->createPreprocessor(TU_Complete);



  //clang::Preprocessor Pre = Clang->getPreprocessor();
  //preprocessor = &Pre;

  //fprintf(stderr, "CONTEXT\n");
  Clang->createASTContext();                              // needs preprocessor 
  astContext_ = &Clang->getASTContext();


  //fprintf(stderr, "filein\n"); 
  const FileEntry *FileIn = FileMgr.getFile(filename); // needs preprocessor
  SourceMgr.setMainFileID(SourceMgr.createFileID(FileIn, clang::SourceLocation(), clang::SrcMgr::C_User));
  //DiagnosticConsumer DiagConsumer = Clang->getDiagnosticClient();
  Clang->getDiagnosticClient().BeginSourceFile(Clang->getLangOpts(), &Clang->getPreprocessor());


  NULLASTConsumer TheConsumer; // must pass a consumer in to ParseAST(). This one does nothing

  //fprintf(stderr, "ready? Parse.\n");
  CHILL_DEBUG_PRINT("actually parsing file %s using clang\n", filename);

  ParseAST(Clang->getPreprocessor(), &TheConsumer, Clang->getASTContext());

  // Translation Unit is contents of a file 
  TranslationUnitDecl *TUD = astContext_->getTranslationUnitDecl();
  // TUD->dump();  // print it out 

  // create another AST, very similar to the clang AST but not written by idiots
  CHILL_DEBUG_PRINT("converting entire clang AST into chill AST (ir_clang.cc)\n");
  chillAST_Node *wholefile = unwrap(ConvertTranslationUnit(TUD, filename));

  fflush(stdout);
  //fprintf(stderr, "printing whole file\n"); 
  //fprintf(stdout, "\n\n" );   fflush(stdout);
  //wholefile->print();
  //wholefile->dump(); 
  //fflush(stdout);

  entire_file_AST = (chillAST_SourceFile *) wholefile;


  astContext_ = &Clang->getASTContext();

  //#define DOUBLE
#ifdef DOUBLE
  fprintf(stderr, "DOUBLE\n");

  fprintf(stderr, "\n\nCLANG dump of the file I parsed:\n"); 
  llvm::OwningPtr<clang::FrontendAction> Act2(new clang::ASTDumpAction());
  // here it actually does the FrontEndAction ?? 
  if (!Clang->ExecuteAction(*Act2)) { // ast dump using builtin function
    exit(3);
  }
#endif
  fflush(stdout);
  fflush(stderr);
  fflush(stdout);
  fflush(stderr);
  fflush(stdout);
  fflush(stderr);
  fflush(stdout);
  fflush(stderr);


#ifdef DONTDOTHIS

  // calling this Action seems to overwrite the astcontext and the AST. (!)
  // don't ever do this, or you lose contact with the original AST (?)

  // Create an action and make the compiler instance carry it out
  //llvm::OwningPtr<clang::CodeGenAction> Act(new clang::EmitLLVMOnlyAction());
  llvm::OwningPtr<clang::FrontendAction> Act(new clang::ASTDumpAction());
  
  fprintf(stderr, "\n\ndump of the file I parsed:\n"); 
  // here it actually does the FrontEndAction ?? 
  if (!Clang->ExecuteAction(*Act)) { // ast dump using builtin function
    exit(3);
  }
  fflush(stdout);
#endif


  //fprintf(stderr, "leaving aClangCompiler::aClangCompiler( filename )\n"); 
}


chillAST_FunctionDecl *aClangCompiler::findprocedurebyname(char *procname) {

  //fprintf(stderr, "searching through files in the clang AST\n\n");
  //fprintf(stderr, "astContext_  0x%x\n", astContext_);

  std::vector<chillAST_Node *> procs;
  findmanually(entire_file_AST, procname, procs);

  //fprintf(stderr, "procs has %d members\n", procs.size());

  if (procs.size() == 0) {
    CHILL_ERROR("could not find function named '%s' in AST from file %s\n", procname, SourceFileName);
    exit(-1);
  }

  if (procs.size() > 1) {
    CHILL_ERROR("oddly, found %d functions named '%s' in AST from file %s\n", procs.size(), procname, SourceFileName);
    CHILL_ERROR("I am unsure what to do\n");
    exit(-1);
  }

  CHILL_DEBUG_PRINT("found the procedure named %s\n", procname);
  return (chillAST_FunctionDecl *) procs[0];

}


#ifdef NOPE
IR_clangCode_Global_Init::IR_clangCode_Global_Init(char *filename  , clang::FileSystemOptions fso ) :
  fileManager(fso) // ,  headerSearch( headerSearchOptions, fileManager, diagengine,  languageOptions, pTargetInfo )
{ 
  /* CLANG Initialization */
  diagnosticsEngine = new clang::DiagnosticsEngine( const IntrusiveRefCntPtr<DiagnosticIDs> &Diags,
                                                    diagnosticOptionsnsa utah
                                                    ) ; // DiagnosticConsumer *client = 0,  bool ShouldOwnClient = true)
  pTextDiagnosticPrinter = new clang::TextDiagnosticPrinter(llvm::outs(), diagnosticOptions);
  diagnostic = new clang::Diagnostic(pTextDiagnosticPrinter);
  sourceManager = new clang::SourceManager(*diagnostic);
  
  // FIXME
  
  // <Warning!!> -- Platform Specific Code lives here
  // This depends on A) that you're running linux and
  // B) that you have the same GCC LIBs installed that
  // I do. 
  // Search through Clang itself for something like this,
  // go on, you won't find it. The reason why is Clang
  // has its own versions of std* which are installed under 
  // /usr/local/lib/clang/<version>/include/
  // See somewhere around Driver.cpp:77 to see Clang adding
  // its version of the headers to its include path.
  /*headerSearchOptions.AddPath("/usr/include/linux", clang::frontend::Angled, false, false, false);
    headerSearchOptions.AddPath("/usr/include/c++/4.3/tr1", clang::frontend::Angled, false, false, false);
    headerSearchOptions.AddPath("/usr/include/c++/4.3", clang::frontend::Angled, false, false, false);*/
  // </Warning!!> -- End of Platform Specific Code
  
  targetOptions.Triple = llvm::sys::getHostTriple();
  pTargetInfo = clang::TargetInfo::CreateTargetInfo(*diagnostic, targetOptions);
  clang::ApplyHeaderSearchOptions( headerSearch, headerSearchOptions, languageOptions, pTargetInfo->getTriple());
  preprocessor = new clang::Preprocessor(*diagnostic, languageOptions, *pTargetInfo, *sourceManager, headerSearch);
  clang::InitializePreprocessor(*preprocessor, preprocessorOptions, headerSearchOptions, frontendOptions); 
  const clang::FileEntry *pFile = fileManager.getFile(filename);
  sourceManager->createMainFileID(pFile);
  //preprocessor.EnterMainSourceFile();
  
  clang::TargetInfo &targetInfo = *pTargetInfo;
  
  idTable = new clang::IdentifierTable(languageOptions);
  
  builtinContext = new clang::Builtin::Context(targetInfo);
  astContext_ = new clang::ASTContext(languageOptions, *sourceManager, targetInfo, *idTable, selTable, *builtinContext, 0); 
  astConsumer_ = new Chill_ASTConsumer();
  clang::Sema sema(*preprocessor, *astContext_, *astConsumer_);
  sema.Initialize();
  clang::ParseAST(*preprocessor, astConsumer_, *astContext_);
}
#endif


IR_clangCode_Global_Init::~IR_clangCode_Global_Init() {
  /*
  delete pTextDiagnosticPrinter;
  delete diagnostic;
  delete sourceManager;
  delete preprocessor;
  delete idTable;
  delete builtinContext;
  delete astContext_;
  delete astConsumer_;
  */
}



// ----------------------------------------------------------------------------
// Class: IR_clangCode
// ----------------------------------------------------------------------------

IR_clangCode::IR_clangCode(const char *filename, const char *proc_name, const char *dest_name) : IR_clangCode(filename,
                                                                                                   proc_name) {
  outfilename = strdup(dest_name);
}
IR_clangCode::IR_clangCode(const char *fname, const char *proc_name) : IR_Code() {
  CHILL_DEBUG_PRINT("IR_xxxxCode::IR_xxxxCode()\n");
  //fprintf(stderr, "IR_clangCode::IR_clangCode( filename %s, procedure %s )\n", filename, proc_name);

  filename = strdup(fname); // filename is internal to IR_clangCode
  procedurename = strdup(proc_name);
  outfilename = NULL;

  int argc = 2;
  char *argv[2];
  argv[0] = strdup("chill");
  argv[1] = strdup(filename);

  // use clang to parse the input file  ?   (or is that already done?) 
  //fprintf(stderr, "IR_clangCode::IR_clangCode(), parsing input file %s\n", argv[1]);

  // this causes opening and parsing of the file.  
  // this is the only call to Instance that has an argument list or file name 
  IR_clangCode_Global_Init *pInstance = IR_clangCode_Global_Init::Instance(argv);

  if (pInstance) {

    aClangCompiler *Clang = pInstance->ClangCompiler;
    //fprintf(stderr, "Clang is 0x%x\n", Clang);

    //fprintf(stderr, "want to get pointer to clang ast for procedure %s\n", proc_name); 
    pInstance->setCurrentFunction(NULL);  // we have no function AST yet

    entire_file_AST = Clang->entire_file_AST;  // ugly that same name, different classes
    chillAST_FunctionDecl *localFD = Clang->findprocedurebyname(strdup(proc_name));   // stored locally
    //fprintf(stderr, "back from findprocedurebyname( %s )\n", proc_name ); 
    //localFD->print();

    pInstance->setCurrentFunction(localFD);

    CHILL_DEBUG_BEGIN
      fprintf(stderr, "Dumping AST for localFD\n");
      localFD->dump();
    CHILL_DEBUG_END

    CHILL_DEBUG_PRINT("calling new CG_chillBuilder() umwut?\n");
    ocg_ = new omega::CG_chillBuilder(localFD->getSourceFile(),localFD);  // ocg == omega code gen
    chillfunc = localFD;

  }

  CHILL_DEBUG_PRINT("returning after reading source file and finding function\n\n");

  //chillfunc->dump( 0, stderr); 

}


IR_clangCode::~IR_clangCode() {
  CHILL_DEBUG_PRINT("output happening as part of the destructor !!\n");

  chillfunc->constantFold();

  chillfunc->cleanUpVarDecls();

  // TODO should output the entire file, not just the function we're working on
  chillAST_SourceFile *src = chillfunc->getSourceFile();
  if (src) {
    CHILL_DEBUG_BEGIN
      src->dump();
    CHILL_DEBUG_END
    if (src->isSourceFile()) src->printToFile(outfilename);
  }
}


//TODO
IR_ScalarSymbol *IR_clangCode::CreateScalarSymbol(const IR_Symbol *sym, int i) {
  if (typeid(*sym) == typeid(IR_chillScalarSymbol)) {  // should be the case ???
    fprintf(stderr, "IR_xxxxCode::CreateScalarSymbol() from a scalar symbol\n");
    const IR_chillScalarSymbol *CSS = (IR_chillScalarSymbol *) sym;
    chillAST_VarDecl *vd = CSS->chillvd;

    // do we have to check to see if it's already there? 
    VariableDeclarations.push_back(vd);
    chillAST_Node *bod = chillfunc->getBody(); // always a compoundStmt ??
    bod->insertChild(0, vd);
    fprintf(stderr, "returning ... really\n");
    return new IR_chillScalarSymbol(this, CSS->chillvd); // CSS->clone();
  }

  if (typeid(*sym) == typeid(IR_chillArraySymbol)) {
    fprintf(stderr, "IR_xxxxCode::CreateScalarSymbol() from an array symbol?\n");
    const IR_chillArraySymbol *CAS = (IR_chillArraySymbol *) sym;

    chillAST_VarDecl *vd = CAS->chillvd;
    fflush(stdout);

    // figure out the base type (probably float) of the array
    char *basetype = vd->underlyingtype;
    fprintf(stderr, "scalar will be of type SgType%s\n", basetype);

    char tmpname[128];
    sprintf(tmpname, "newVariable%i\0", vd->chill_scalar_counter++);
    chillAST_VarDecl *scalarvd = new chillAST_VarDecl(basetype, tmpname, "", NULL);  // TODO parent
    scalarvd->print();
    printf("\n");
    fflush(stdout);

    fprintf(stderr, "VarDecl has parent that is a NULL\n");

    return (IR_ScalarSymbol *) (new IR_chillScalarSymbol(this, scalarvd)); // CSS->clone();
  }

  fprintf(stderr,
          "IR_clangCode::CreateScalarSymbol(), passed a sym that is not a clang scalar symbol OR an array symbol???\n");
  int *n = NULL;
  n[0] = 1;
  exit(-1);
  return NULL;
}


IR_ArraySymbol *
IR_clangCode::CreateArraySymbol(const IR_Symbol *sym, std::vector<omega::CG_outputRepr *> &size, int i) {
  fprintf(stderr, "IR_xxxxCode::CreateArraySymbol()\n");

  // build a new array name 
  char namestring[128];

  sprintf(namestring, "_P%d\0", entire_file_AST->chill_array_counter++);
  fprintf(stderr, "creating Array %s\n", namestring);

  char arraypart[100];
  char *s = &arraypart[0];

  for (int i = 0; i < size.size(); i++) {
    omega::CG_outputRepr *OR = size[i];
    CG_chillRepr *CR = (CG_chillRepr *) OR;
    //fprintf(stderr, "%d chillnodes\n", CR->chillnodes.size()); 

    // this SHOULD be 1 chillnode of type IntegerLiteral (per dimension)
    int numnodes = CR->chillnodes.size();
    if (1 != numnodes) {
      fprintf(stderr,
              "IR_clangCode::CreateArraySymbol() array dimension %d has %d chillnodes\n",
              i, numnodes);
      exit(-1);
    }

    chillAST_Node *nodezero = CR->chillnodes[0];
    if (!nodezero->isIntegerLiteral()) {
      fprintf(stderr, "IR_clangCode::CreateArraySymbol() array dimension %d not an IntegerLiteral\n", i);
      exit(-1);
    }

    chillAST_IntegerLiteral *IL = (chillAST_IntegerLiteral *) nodezero;
    int val = IL->value;
    sprintf(s, "[%d]\0", val);
    s = &arraypart[strlen(arraypart)];
  }
  //fprintf(stderr, "arraypart '%s'\n", arraypart); 

  chillAST_VarDecl *vd = new chillAST_VarDecl("float", namestring, arraypart, NULL); // todo type from sym

  // put decl in some symbol table
  VariableDeclarations.push_back(vd);
  // insert decl in the IR_code body
  chillAST_Node *bod = chillfunc->getBody(); // always a compoundStmt ?? 
  bod->insertChild(0, vd);

  return new IR_chillArraySymbol(this, vd);
}

// TODO 
std::vector<IR_ScalarRef *> IR_clangCode::FindScalarRef(const omega::CG_outputRepr *repr) const {
  std::vector<IR_ScalarRef *> scalars;

  const omega::CG_chillRepr *crepr = static_cast<const omega::CG_chillRepr *>(repr);
  std::vector<chillAST_Node *> chillstmts = crepr->getChillCode();

  std::vector<chillAST_DeclRefExpr *> refs;
  for (int i = 0; i < chillstmts.size(); i++) {
    chillstmts[i]->gatherScalarRefs(refs,false);
  }
  for (int i = 0; i < refs.size(); i++) {
    scalars.push_back(new IR_chillScalarRef(this, refs[i]));
  }
  return scalars;
}


IR_ScalarRef *IR_clangCode::CreateScalarRef(const IR_ScalarSymbol *sym) {
  IR_chillScalarRef *sr = new IR_chillScalarRef(this, new chillAST_DeclRefExpr(
      ((IR_chillScalarSymbol *) sym)->chillvd)); // uses VarDecl to mak a declrefexpr
  return sr;
}

bool IR_clangCode::FromSameStmt(IR_ArrayRef *A, IR_ArrayRef *B) {
  return ((IR_chillArrayRef*)A)->chillASE->findContainingStmt() == ((IR_chillArrayRef*)A)->chillASE->findContainingStmt();
}

IR_ArrayRef *IR_clangCode::CreateArrayRef(const IR_ArraySymbol *sym, std::vector<omega::CG_outputRepr *> &index) {
  fprintf(stderr, "IR_clangCode::CreateArrayRef()   ir_clang.cc\n");
  fprintf(stderr, "sym->n_dim() %d   index.size() %d\n", sym->n_dim(), index.size());

  int t;
  if (sym->n_dim() != index.size()) {
    throw std::invalid_argument("incorrect array symbol dimensionality   dim != size    ir_clang.cc L2359");
  }

  const IR_chillArraySymbol *c_sym = static_cast<const IR_chillArraySymbol *>(sym);
  chillAST_VarDecl *vd = c_sym->chillvd;
  std::vector<chillAST_Node *> inds;

  for (int i = 0; i < index.size(); i++) {
    CG_chillRepr *CR = (CG_chillRepr *) index[i];

    int numnodes = CR->chillnodes.size();
    if (1 != numnodes) {
      CHILL_ERROR("IR_clangCode::CreateArrayRef() array dimension %d has %d chillnodes\n", i, numnodes);
      exit(-1);
    }

    inds.push_back(CR->chillnodes[0]);

    /* 
       chillAST_Node *nodezero = CR->chillnodes[0];
    if (!nodezero->isIntegerLiteral())  {
      fprintf(stderr,"IR_clangCode::CreateArrayRef() array dimension %d not an IntegerLiteral\n",i);
      fprintf(stderr, "it is a %s\n", nodezero->getTypeString()); 
      nodezero->print(); printf("\n"); fflush(stdout); 
      exit(-1);
    }

    chillAST_IntegerLiteral *IL = (chillAST_IntegerLiteral *)nodezero;
    int val = IL->value;
    inds.push_back( val );
    */
  }

  // now we've got the vardecl AND the indeces to make a chillAST that represents the array reference
  // TODO Passing NULL for chillAST node?
  CHILL_DEBUG_PRINT("Passed NULL as chillAST node");
  chillAST_ArraySubscriptExpr *ASE = new chillAST_ArraySubscriptExpr(vd, inds);

  auto ref = new IR_chillArrayRef(this, ASE, 0);

  return ref;
}

// find all array references ANYWHERE in this block of code  ?? 
std::vector<IR_ArrayRef *> IR_clangCode::FindArrayRef(const omega::CG_outputRepr *repr) const {
  //fprintf(stderr, "FindArrayRef()\n"); 
  std::vector<IR_ArrayRef *> arrays;
  const omega::CG_chillRepr *crepr = static_cast<const omega::CG_chillRepr *>(repr);
  std::vector<chillAST_Node *> chillstmts = crepr->getChillCode();

  //fprintf(stderr, "there are %d chill statements in this repr\n", chillstmts.size()); 

  std::vector<chillAST_ArraySubscriptExpr *> refs;
  for (int i = 0; i < chillstmts.size(); i++) {
    //fprintf(stderr, "\nchillstatement %d = ", i); chillstmts[i]->print(0, stderr); fprintf(stderr, "\n"); 
    chillstmts[i]->gatherArrayRefs(refs, false);
  }
  //fprintf(stderr, "%d total refs\n", refs.size());
  for (int i = 0; i < refs.size(); i++) {
    if (refs[i]->imreadfrom) {
      //fprintf(stderr, "ref[%d] going to be put in TWICE, as both read and write\n", i); 
      arrays.push_back(new IR_chillArrayRef(this, refs[i], 0));  // UGLY TODO dual usage of a ref in "+="
    }
    arrays.push_back(new IR_chillArrayRef(this, refs[i], refs[i]->imwrittento)); // this is wrong
    // we need to know whether this reference will be written, etc. 
  }

  /* 
  if(chillstmts.size() > 1) {
    for(int i=0; i<tnl->size(); ++i) {
      omega::CG_chillRepr *r = new omega::CG_chillRepr((*tnl)[i]);
      std::vector<IR_ArrayRef *> a = FindArrayRef(r);
      delete r;
      std::copy(a.begin(), a.end(), back_inserter(arrays));
    }
  } else if(chillstmts.size() == 1) {
    Stmt *s = (*tnl)[0];
    
    if(CompoundStmt *cs = dyn_cast<CompoundStmt>(s)) {
      for(CompoundStmt::body_iterator bi = cs->body_begin(); bi != cs->body_end(); ++bi) {
        omega::CG_chillRepr *r = new omega::CG_chillRepr(*bi);
        std::vector<IR_ArrayRef *> a = FindArrayRef(r);
        delete r;
        std::copy(a.begin(), a.end(), back_inserter(arrays));
      }
    } else if(ForStmt *fs = dyn_cast<ForStmt>(s)) {
      omega::CG_chillRepr *r = new omega::CG_chillRepr(fs->getBody());
      std::vector<IR_ArrayRef *> a = FindArrayRef(r);
      delete r;
      std::copy(a.begin(), a.end(), back_inserter(arrays));
    } else if(IfStmt *ifs = dyn_cast<IfStmt>(s)) {
      omega::CG_chillRepr *r = new omega::CG_chillRepr(ifs->getCond());
      std::vector<IR_ArrayRef *> a = FindArrayRef(r);
      delete r;
      std::copy(a.begin(), a.end(), back_inserter(arrays));
      r = new omega::CG_chillRepr(ifs->getThen());
      a = FindArrayRef(r);
      delete r;
      std::copy(a.begin(), a.end(), back_inserter(arrays));
      if(Stmt *s_else = ifs->getElse()) {
        r = new omega::CG_chillRepr(s_else);
        a = FindArrayRef(r);
        delete r;
        std::copy(a.begin(), a.end(), back_inserter(arrays));
      }
    } else if(Expr *e = dyn_cast<Expr>(s)) {
      omega::CG_chillRepr *r = new omega::CG_chillRepr(static_cast<Expr*>(s));
      std::vector<IR_ArrayRef *> a = FindArrayRef(r);
      delete r;
      std::copy(a.begin(), a.end(), back_inserter(arrays));
    } else throw chill::error::ir("control structure not supported");
  }
  */
/* 
  else { // We have an expression
    Expr *op = static_cast<const omega::CG_chillRepr *>(repr)->GetExpression();
    if(0) { // TODO: Handle pointer reference exp. here
    } else if(BinaryOperator *bop = dyn_cast<BinaryOperator>(op)) {
      omega::CG_chillRepr *r1 = new omega::CG_chillRepr(bop->getLHS());
      std::vector<IR_ArrayRef *> a1 = FindArrayRef(r1);
      delete r1;      
      std::copy(a1.begin(), a1.end(), back_inserter(arrays));
      omega::CG_chillRepr *r2 = new omega::CG_chillRepr(bop->getRHS());
      std::vector<IR_ArrayRef *> a2 = FindArrayRef(r2);
      delete r2;
      std::copy(a2.begin(), a2.end(), back_inserter(arrays));
    } else if(UnaryOperator *uop = dyn_cast<UnaryOperator>(op)) {
      omega::CG_chillRepr *r1 = new omega::CG_chillRepr(uop->getSubExpr());
      std::vector<IR_ArrayRef *> a1 = FindArrayRef(r1);
      delete r1;      
      std::copy(a1.begin(), a1.end(), back_inserter(arrays));
    } //else throw chill::error::ir("Invalid expr. type passed to FindArrayRef");
  }
  */
  return arrays;
}


std::vector<IR_Control *> IR_clangCode::FindOneLevelControlStructure(const IR_Block *block) const {
  const IR_chillBlock *CB = (const IR_chillBlock *) block;
  //fprintf(stderr, "block 0x%x\n", block); 

  std::vector<IR_Control *> controls;

  chillAST_Node *blockast = NULL;
  int numstmts = CB->statements.size();
  bool unwrap = false;
  CHILL_DEBUG_PRINT("%d statements\n", numstmts);

  if (numstmts == 0) return controls;

  else if (numstmts == 1) blockast = CB->statements[0]; // a single statement

  CHILL_DEBUG_BEGIN
    for (int i = 0; i < CB->statements.size(); ++i) {
      fprintf(stderr, "block's AST is of type %s\n", CB->statements[i]->getTypeString());
      CB->statements[i]->print();
    }
  CHILL_DEBUG_END

  // build up a vector of "controls".
  // a run of straight-line code (statements that can't cause branching) will be 
  // bundled up into an IR_Block
  // ifs and loops will get their own entry
  const std::vector<chillAST_Node *> *children = NULL;
  if (blockast) {
    if (blockast->isFunctionDecl()) {
      chillAST_FunctionDecl *FD = (chillAST_FunctionDecl *) blockast;
      chillAST_Node *bod = FD->getBody();
      children = bod->getChildren();
      unwrap = true;
    }
    if (blockast->isCompoundStmt()) {
      children = blockast->getChildren();
      unwrap = true;
    }
    if (blockast->isForStmt()) {
      controls.push_back(new IR_chillLoop(this, (chillAST_ForStmt *) blockast));
      return controls;
    }
  }
  if (!children)
    children = &(CB->statements);

  int numchildren = children->size();
  int ns;
  IR_chillBlock *basicblock = new IR_chillBlock(this); // no statements
  for (int i = 0; i < numchildren; i++) {
    CHILLAST_NODE_TYPE typ = (*children)[i]->getType();
    if (typ == CHILLAST_NODE_LOOP) {
      ns = basicblock->numstatements();
      if (ns) {
        CHILL_DEBUG_PRINT("pushing a run of statements as a block\n");
        controls.push_back(basicblock);
        basicblock = new IR_chillBlock(this); // start a new one
      }

      CHILL_DEBUG_PRINT("pushing the loop at %d\n", i);
      controls.push_back(new IR_chillLoop(this, (chillAST_ForStmt *) (*children)[i]));
    } else if (typ == CHILLAST_NODE_IFSTMT ) {
      ns = basicblock->numstatements();
      if (ns) {
        CHILL_DEBUG_PRINT("pushing a run of statements as a block\n");
        controls.push_back(basicblock);
        basicblock = new IR_chillBlock(this); // start a new one
      }
      CHILL_DEBUG_PRINT("pushing the if at %d\n", i);
      controls.push_back(new IR_chillIf(this, (chillAST_IfStmt *) (*children)[i]));
    } else
      basicblock->addStatement((*children)[i]);
  } // for each child
  ns = basicblock->numstatements();
  if (ns != 0 && (unwrap || ns != numchildren))
    controls.push_back(basicblock);

  CHILL_DEBUG_PRINT("returning vector of %d controls\n", controls.size());
  return controls;
}


IR_Block *IR_clangCode::MergeNeighboringControlStructures(const std::vector<IR_Control *> &controls) const {
  CHILL_DEBUG_PRINT("%d controls\n", controls.size());

  if (controls.size() == 0)
    return NULL;

  IR_chillBlock *CBlock = new IR_chillBlock(controls[0]->ir_); // the thing we're building

  std::vector<chillAST_Node *> statements;
  chillAST_Node *parent = NULL;
  for (int i = 0; i < controls.size(); i++) {
    switch (controls[i]->type()) {
      case IR_CONTROL_LOOP: {
        CHILL_DEBUG_PRINT("control %d is IR_CONTROL_LOOP\n", i);
        chillAST_ForStmt *loop = static_cast<IR_chillLoop *>(controls[i])->chillforstmt;
        if (parent == NULL) {
          parent = loop->parent;
        } else {
          if (parent != loop->parent) {
            throw chill::error::ir("controls to merge not at the same level");
          }
        }
        CBlock->addStatement(loop);
        break;
      }
      case IR_CONTROL_BLOCK: {
        CHILL_DEBUG_PRINT("control %d is IR_CONTROL_BLOCK\n", i);
        IR_chillBlock *CB = static_cast<IR_chillBlock *>(controls[i]);
        std::vector<chillAST_Node *> blockstmts = CB->statements;
        for (int j = 0; j < blockstmts.size(); j++) {
          if (parent == NULL) {
            parent = blockstmts[j]->parent;
          } else {
            if (parent != blockstmts[j]->parent) {
              throw chill::error::ir(
                  "ir_clang.cc  IR_clangCode::MergeNeighboringControlStructures  controls to merge not at the same level");
            }
          }
          CBlock->addStatement(blockstmts[j]);
        }
        break;
      }
      default:
        throw chill::error::ir("unrecognized control to merge");
    }
  } // for each control

  return CBlock;
}


IR_Block *IR_clangCode::GetCode() const {    // return IR_Block corresponding to current function?
  //fprintf(stderr, "IR_clangCode::GetCode()\n"); 
  //Stmt *s = func_->getBody();  // clang statement, and clang getBody
  //fprintf(stderr, "chillfunc 0x%x\n", chillfunc);

  //chillAST_Node *bod = chillfunc->getBody();  // chillAST 
  //fprintf(stderr, "printing the function getBody()\n"); 
  //fprintf(stderr, "sourceManager 0x%x\n", sourceManager); 
  //bod->print(); 

  return new IR_chillBlock(this, chillfunc);
}


void IR_clangCode::ReplaceCode(IR_Control *old, omega::CG_outputRepr *repr) {
  CG_chillRepr *chillrepr = (CG_chillRepr *) repr;
  std::vector<chillAST_Node *> newcode = chillrepr->getChillCode();
  int numnew = newcode.size();
  struct IR_chillLoop *cloop;
  std::vector<chillAST_VarDecl *> olddecls;
  chillfunc->gatherVarDecls(olddecls);
  std::vector<chillAST_VarDecl *> decls;
  for (int i = 0; i < numnew; i++)
    newcode[i]->gatherVarUsage(decls);

  for (int i = 0; i < decls.size(); i++) {
    int inthere = 0;
    for (int j = 0; j < VariableDeclarations.size(); j++)
      if (VariableDeclarations[j] == decls[i])
        inthere = 1;
    for (int j = 0; j < olddecls.size(); j++) {
      if (decls[i] == olddecls[j])
        inthere = 1;
      if (!strcmp(decls[i]->varname, olddecls[j]->varname))
        if (!strcmp(decls[i]->arraypart, olddecls[j]->arraypart))
          inthere = 1;
    }
    if (!inthere) {
      chillfunc->getBody()->insertChild(0, decls[i]);
      olddecls.push_back(decls[i]);
    }
  }

  chillAST_Node *par;
  switch (old->type()) {
    case IR_CONTROL_LOOP: {
      cloop = (struct IR_chillLoop *) old;
      chillAST_ForStmt *forstmt = cloop->chillforstmt;

      par = forstmt->parent;
      if (!par) {
        CHILL_ERROR("old parent was NULL\n");
        CHILL_ERROR("ir_clang.cc that will not work very well.\n");
        exit(-1);
      }

      CHILL_DEBUG_BEGIN
        fprintf(stderr, "\nold parent was\n\n{\n");
        par->print();
        fprintf(stderr, "\n}\n");
      CHILL_DEBUG_END

      std::vector<chillAST_Node *> *oldparentcode = par->getChildren(); // probably only works for compoundstmts

      // find loop in the parent
      int numstatements = oldparentcode->size();
      int index = par->findChild(forstmt);
      if (index < 0) {
        CHILL_ERROR("can't find the loop in its parent\n");
        exit(-1);
      }
      // insert the new code
      par->setChild(index, newcode[0]);    // overwrite old stmt
      // do we need to update the IR_cloop?
      cloop->chillforstmt = (chillAST_ForStmt *) newcode[0]; // ?? DFL

      if (numnew > 1) {
        // add the rest of the new statements
        CHILL_DEBUG_BEGIN
          for (int i = 1; i < numnew; i++) {
            fprintf(stderr, "inserting \n");
            newcode[i]->print(0, stderr);
          }
        CHILL_DEBUG_END
        for (int i = 1; i < numnew; i++)
          par->insertChild(index + i, newcode[i]);  // sets parent
      }

      // TODO add in (insert) variable declarations that go with the new loops

      fflush(stdout);
    }
      break;
    case IR_CONTROL_BLOCK: {
      CHILL_ERROR("old is IR_CONTROL_BLOCK\n");
      par = ((IR_chillBlock*)old)->statements[0]->parent;
      if (!par) {
        CHILL_ERROR("old parent was NULL\n");
        CHILL_ERROR("ir_clang.cc that will not work very well.\n");
        exit(-1);
      }
      IR_chillBlock *cblock = (struct IR_chillBlock *) old;
      std::vector<chillAST_Node *> *oldparentcode = par->getChildren(); // probably only works for compoundstmts
      int index = par->findChild(cblock->statements[0]);
      for (int i = 0;i<cblock->numstatements();++i) // delete all current statements
        par->removeChild(par->findChild(cblock->statements[i]));
      for (int i = 0; i < numnew; i++)
        par->insertChild(index + i, newcode[i]);  // insert New child
      // TODO add in (insert) variable declarations that go with the new loops
      break;
    }
    default:
      throw chill::error::ir("control structure to be replaced not supported");
      break;
  }

  fflush(stdout);
  CHILL_DEBUG_BEGIN
    fprintf(stderr, "new parent2 is\n\n{\n");
    std::vector<chillAST_Node *> *newparentcode = par->getChildren();
    for (int i = 0; i < newparentcode->size(); i++) {
      (*newparentcode)[i]->print();
    }
    fprintf(stderr, "}\n");
  CHILL_DEBUG_END


}


void IR_clangCode::ReplaceExpression(IR_Ref *old, omega::CG_outputRepr *repr) {
  fprintf(stderr, "IR_xxxxCode::ReplaceExpression()\n");

  if (typeid(*old) == typeid(IR_chillArrayRef)) {
    //fprintf(stderr, "expressions is IR_chillArrayRef\n"); 
    IR_chillArrayRef *CAR = (IR_chillArrayRef *) old;
    chillAST_ArraySubscriptExpr *CASE = CAR->chillASE;
    printf("\nreplacing old ");
    CASE->print();
    printf("\n");
    fflush(stdout);

    omega::CG_chillRepr *crepr = (omega::CG_chillRepr *) repr;
    if (crepr->chillnodes.size() != 1) {
      fprintf(stderr, "IR_clangCode::ReplaceExpression(), replacing with %d chillnodes???\n");
      //exit(-1);
    }

    chillAST_Node *newthing = crepr->chillnodes[0];
    fprintf(stderr, "with new ");
    newthing->print();
    printf("\n\n");
    fflush(stdout);

    if (!CASE->parent) {
      fprintf(stderr, "IR_clangCode::ReplaceExpression()  old has no parent ??\n");
      exit(-1);
    }

    fprintf(stderr, "OLD parent = "); // of type %s\n", CASE->parent->getTypeString()); 
    if (CASE->parent->isImplicitCastExpr()) CASE->parent->parent->print();
    else CASE->parent->print();
    printf("\n");
    fflush(stdout);

    //CASE->parent->print(); printf("\n"); fflush(stdout); 
    //CASE->parent->parent->print(); printf("\n"); fflush(stdout); 
    //CASE->parent->parent->print(); printf("\n"); fflush(stdout); 
    //CASE->parent->parent->parent->print(); printf("\n"); fflush(stdout); 

    CASE->parent->replaceChild(CASE, newthing);

    fprintf(stderr, "after replace parent is "); // of type %s\n", CASE->parent->getTypeString()); 
    if (CASE->parent->isImplicitCastExpr()) CASE->parent->parent->print();
    else CASE->parent->print();
    printf("\n\n");
    fflush(stdout);



    //CASE->parent->print(); printf("\n"); fflush(stdout); 
    //CASE->parent->parent->print(); printf("\n"); fflush(stdout); 
    //CASE->parent->parent->print(); printf("\n"); fflush(stdout); 
    //CASE->parent->parent->parent->print(); printf("\n"); fflush(stdout); 


  } else if (typeid(*old) == typeid(IR_chillScalarRef)) {
    fprintf(stderr, "IR_clangCode::ReplaceExpression()  IR_chillScalarRef unhandled\n");
  } else {
    fprintf(stderr, "UNKNOWN KIND OF REF\n");
    exit(-1);
  }

  delete old;
}


// TODO 
IR_CONDITION_TYPE IR_clangCode::QueryBooleanExpOperation(const omega::CG_outputRepr *repr) const {
  return IR_COND_UNKNOWN;
}


IR_OPERATION_TYPE IR_clangCode::QueryExpOperation(const omega::CG_outputRepr *repr) const {
  //fprintf(stderr, "IR_clangCode::QueryExpOperation()\n");

  CG_chillRepr *crepr = (CG_chillRepr *) repr;
  chillAST_Node *node = crepr->chillnodes[0];
  //fprintf(stderr, "chillAST node type %s\n", node->getTypeString());

  // really need to be more rigorous than this hack  // TODO 
  if (node->isImplicitCastExpr()) node = ((chillAST_ImplicitCastExpr *) node)->getSubExpr();
  if (node->isCStyleCastExpr()) node = ((chillAST_CStyleCastExpr *) node)->getSubExpr();
  if (node->isParenExpr()) node = ((chillAST_ParenExpr *) node)->getSubExpr();

  if (node->isIntegerLiteral() || node->isFloatingLiteral()) return IR_OP_CONSTANT;
  else if (node->isBinaryOperator() || node->isUnaryOperator()) {
    char *opstring;
    if (node->isBinaryOperator())
      opstring = ((chillAST_BinaryOperator *) node)->op; // TODO enum
    else
      opstring = ((chillAST_UnaryOperator *) node)->op; // TODO enum

    if (!strcmp(opstring, "+")) return IR_OP_PLUS;
    if (!strcmp(opstring, "-")) return IR_OP_MINUS;
    if (!strcmp(opstring, "*")) return IR_OP_MULTIPLY;
    if (!strcmp(opstring, "/")) return IR_OP_DIVIDE;
    if (!strcmp(opstring, "=")) return IR_OP_ASSIGNMENT;

    CHILL_ERROR("UNHANDLED Binary(or Unary)Operator op type (%s)\n", opstring);
    exit(-1);
  } else if (node->isDeclRefExpr()) return IR_OP_VARIABLE; // ??
    //else if (node->is ) return  something;
  else {
    CHILL_ERROR("IR_clangCode::QueryExpOperation()  UNHANDLED NODE TYPE %s\n", node->getTypeString());
    exit(-1);
  }

  /* CLANG 
  Expr *e = static_cast<const omega::CG_chillRepr *>(repr)->GetExpression();
  if(isa<IntegerLiteral>(e) || isa<FloatingLiteral>(e)) return IR_OP_CONSTANT;
  else if(isa<DeclRefExpr>(e)) return IR_OP_VARIABLE;
  else if(BinaryOperator *bop = dyn_cast<BinaryOperator>(e)) {
    switch(bop->getOpcode()) {
    case BO_Assign: return IR_OP_ASSIGNMENT;
    case BO_Add: return IR_OP_PLUS;
    case BO_Sub: return IR_OP_MINUS;
    case BO_Mul: return IR_OP_MULTIPLY;
    case BO_Div: return IR_OP_DIVIDE;
    default: return IR_OP_UNKNOWN;
    }
  } else if(UnaryOperator *uop = dyn_cast<UnaryOperator>(e)) {
    switch(uop->getOpcode()) {
    case UO_Minus: return IR_OP_NEGATIVE;
    case UO_Plus: return IR_OP_POSITIVE;
    default: return IR_OP_UNKNOWN;
    }
  } else if(ConditionalOperator *cop = dyn_cast<ConditionalOperator>(e)) {
    BinaryOperator *bop;
    if(bop = dyn_cast<BinaryOperator>(cop->getCond())) {
      if(bop->getOpcode() == BO_GT) return IR_OP_MAX;
      else if(bop->getOpcode() == BO_LT) return IR_OP_MIN;
    } else return IR_OP_UNKNOWN;
    
  } 
  
  else if(e == NULL) return IR_OP_NULL;
  else return IR_OP_UNKNOWN;
  }
   END CLANG */
}


std::vector<omega::CG_outputRepr *> IR_clangCode::QueryExpOperand(const omega::CG_outputRepr *repr) const {
  //fprintf(stderr, "IR_clangCode::QueryExpOperand()\n"); 
  std::vector<omega::CG_outputRepr *> v;

  CG_chillRepr *crepr = (CG_chillRepr *) repr;
  //Expr *e = static_cast<const omega::CG_chillRepr *>(repr)->GetExpression(); wrong.. CLANG
  chillAST_Node *e = crepr->chillnodes[0]; // ?? 
  //e->print(); printf("\n"); fflush(stdout); 

  // really need to be more rigorous than this hack  // TODO 
  if (e->isImplicitCastExpr()) e = ((chillAST_ImplicitCastExpr *) e)->getSubExpr();
  if (e->isCStyleCastExpr()) e = ((chillAST_CStyleCastExpr *) e)->getSubExpr();
  if (e->isParenExpr()) e = ((chillAST_ParenExpr *) e)->getSubExpr();


  //if(isa<IntegerLiteral>(e) || isa<FloatingLiteral>(e) || isa<DeclRefExpr>(e)) {
  if (e->isIntegerLiteral() || e->isFloatingLiteral() || e->isDeclRefExpr()) {
    //fprintf(stderr, "it's a constant\n"); 
    omega::CG_chillRepr *repr = new omega::CG_chillRepr(e);
    v.push_back(repr);
    //} else if(BinaryOperator *bop = dyn_cast<BinaryOperator>(e)) {
  } else if (e->isBinaryOperator()) {
    //fprintf(stderr, "ir_clang.cc BOP TODO\n"); exit(-1); // 
    chillAST_BinaryOperator *bop = (chillAST_BinaryOperator *) e;
    char *op = bop->op;  // TODO enum for operator types
    if (!strcmp(op, "=")) {
      v.push_back(new omega::CG_chillRepr(bop->getRHS()));  // for assign, return RHS
    } else if (!strcmp(op, "+") || !strcmp(op, "-") || !strcmp(op, "*") || !strcmp(op, "/")) {
      v.push_back(new omega::CG_chillRepr(bop->getLHS()));  // for +*-/ return both lhs and rhs
      v.push_back(new omega::CG_chillRepr(bop->getRHS()));
    } else {
      CHILL_ERROR("Binary Operator  UNHANDLED op (%s)\n", op);
      exit(-1);
    }
  } // BinaryOperator
  else if (e->isUnaryOperator()) {
    omega::CG_chillRepr *repr;
    chillAST_UnaryOperator *uop = (chillAST_UnaryOperator *) e;
    char *op = uop->op; // TODO enum
    if (!strcmp(op, "+") || !strcmp(op, "-")) {
      v.push_back(new omega::CG_chillRepr(uop->getSubExpr()));
    } else {
      CHILL_ERROR("ir_clang.cc  IR_clangCode::QueryExpOperand() Unary Operator  UNHANDLED op (%s)\n", op);
      exit(-1);
    }
  } // unaryoperator
  else {
    CHILL_ERROR("UNHANDLED node type %s\n", e->getTypeString());
    exit(-1);
  }


  /*
Expr *op1, *op2;
  switch(bop->getOpcode()) {
  case BO_Assign:
    op2 = bop->getRHS();
    repr = new omega::CG_chillRepr(op2);
    v.push_back(repr);
    break;
  case BO_Add:
  case BO_Sub:
  case BO_Mul:
  case BO_Div:
    op1 = bop->getLHS();
    repr = new omega::CG_chillRepr(op1);
    v.push_back(repr);
    op2 = bop->getRHS();
    repr = new omega::CG_chillRepr(op2);
    v.push_back(repr);
    break;
  default:
    throw chill::error::ir("operation not supported");
  }
  */
  //} else if(UnaryOperator *uop = dyn_cast<UnaryOperator>(e)) {
  //} else if(e->isUnaryOperator()) {
  /*
  omega::CG_chillRepr *repr;

  switch(uop->getOpcode()) {
  case UO_Minus:
  case UO_Plus:
    op1 = uop->getSubExpr();
    repr = new omega::CG_chillRepr(op1);
    v.push_back(repr);
    break;
  default:
    throw chill::error::ir("operation not supported");
  }
  */
  //} else if(ConditionalOperator *cop = dyn_cast<ConditionalOperator>(e)) {
  //omega::CG_chillRepr *repr;

  // TODO: Handle conditional operator here
  //} else  throw chill::error::ir("operand type UNsupported");

  return v;
}

IR_Ref *IR_clangCode::Repr2Ref(const omega::CG_outputRepr *repr) const {
  CG_chillRepr *crepr = (CG_chillRepr *) repr;
  chillAST_Node *node = crepr->chillnodes[0];

  if (node->isIntegerLiteral()) {
    // FIXME: Not sure if it'll work in all cases (long?)
    int val = ((chillAST_IntegerLiteral *) node)->value;
    return new IR_chillConstantRef(this, static_cast<omega::coef_t>(val));
  } else if (node->isFloatingLiteral()) {
    float val = ((chillAST_FloatingLiteral *) node)->value;
    return new IR_chillConstantRef(this, val);
  } else if (node->isDeclRefExpr()) {
    return new IR_chillScalarRef(this, (chillAST_DeclRefExpr *) node);  // uses DRE
  } else {
    fprintf(stderr, "ir_clang.cc IR_clangCode::Repr2Ref() UNHANDLED node type %s\n", node->getTypeString());
    exit(-1);
  }
}

chillAST_NodeList* ConvertMemberExpr(clang::MemberExpr *clangME) {
  chillAST_Node *base = unwrap(ConvertGenericClangAST(clangME->getBase()));

  DeclarationNameInfo memnameinfo = clangME->getMemberNameInfo();
  DeclarationName DN = memnameinfo.getName();
  const char *member = DN.getAsString().c_str();

  chillAST_MemberExpr *ME = new chillAST_MemberExpr(base, member, clangME);

  NL_RET(ME);

}

omega::CG_outputRepr *IR_chillIf::condition() const {
  assert( code->isIfStmt() && "If statement's code is not if statement");
  return new omega::CG_chillRepr(((chillAST_IfStmt*)code) -> getCond());
}

IR_Block *IR_chillIf::then_body() const {
  assert( code->isIfStmt() && "If statement's code is not if statement");
  chillAST_Node* thenPart = ((chillAST_IfStmt*)code) -> getThen();
  if (thenPart) return new IR_chillBlock(ir_,thenPart);
  return NULL;
}

IR_Block *IR_chillIf::else_body() const {
  assert( code->isIfStmt() && "If statement's code is not if statement");
  chillAST_Node* elsePart = ((chillAST_IfStmt*)code) -> getElse();
  if (elsePart) return new IR_chillBlock(ir_,elsePart);
  return NULL;
}

IR_Block *IR_chillIf::convert() {
  const IR_Code *ir = ir_;
  chillAST_Node *code = this->code;
  delete this;
  return new IR_chillBlock(ir,code);
}

IR_Control *IR_chillIf::clone() const {
  return new IR_chillIf(ir_,code);
}
