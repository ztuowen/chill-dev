//
// Created by ztuowen on 10/10/16.
//

#include <typeinfo>
#include <sstream>
#include "ir_chill.hh"
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

using namespace clang;
using namespace clang::driver;
using namespace omega;
using namespace std;

#define NL_RET(x) {chillAST_NodeList *ret = new chillAST_NodeList(); \
                    ret->push_back(x); \
                    return ret;}

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
  int count = 0;
  for (clang::RecordDecl::field_iterator fi = RD->field_begin(); fi != RD->field_end(); fi++) count++;

  char blurb[128];

  chillAST_TypedefDecl *astruct = new chillAST_TypedefDecl(blurb, "", NULL);
  astruct->setStruct(true);
  astruct->setStructName(RD->getNameAsString().c_str());

  for (clang::RecordDecl::field_iterator fi = RD->field_begin(); fi != RD->field_end(); fi++) {
    clang::FieldDecl *FD = (*fi);
    string TypeStr = FD->getType().getAsString();
    const char *typ = TypeStr.c_str();
    const char *name = FD->getNameAsString().c_str();
    chillAST_VarDecl *VD = NULL;
    // very clunky and incomplete
    VD = new chillAST_VarDecl(typ, name, "", astruct); // can't handle arrays yet
    astruct->subparts.push_back(VD);
  }
  NL_RET(astruct);
}


chillAST_NodeList* ConvertTypeDefDecl(TypedefDecl *TDD) {
  char *under = strdup(TDD->getUnderlyingType().getAsString().c_str());
  char *arraypart = splitTypeInfo(under);
  char *alias = strdup(TDD->getName().str().c_str());
  chillAST_TypedefDecl *CTDD = new chillAST_TypedefDecl(under, alias, arraypart);
  free(under);
  free(arraypart);
  NL_RET(CTDD);
}


chillAST_NodeList* ConvertDeclStmt(DeclStmt *clangDS) {
  chillAST_VarDecl *chillvardecl; // the thing we'll return if this is a single declaration
  bool multiples = !clangDS->isSingleDecl();
  DeclGroupRef dgr = clangDS->getDeclGroup();
  clang::DeclGroupRef::iterator DI = dgr.begin();
  clang::DeclGroupRef::iterator DE = dgr.end();
  chillAST_NodeList* decls = new chillAST_NodeList();
  for (; DI != DE; ++DI) {
    Decl *D = *DI;
    const char *declty = D->getDeclKindName();
    if (!strcmp("Var", declty)) {
      VarDecl *V = dyn_cast<VarDecl>(D);
      std::string Name = V->getNameAsString();
      char *varname = strdup(Name.c_str());
      QualType T = V->getType();
      string TypeStr = T.getAsString();
      char *vartype = strdup(TypeStr.c_str());
      CHILL_DEBUG_PRINT("DeclStmt (clang 0x%x) for %s %s\n", D, vartype,  varname);
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
  }
  return decls;
}


chillAST_NodeList* ConvertCompoundStmt(CompoundStmt *clangCS) {
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

    VariableDeclarations.push_back(decl);
    chillFD->addParameter(decl);
    CHILL_DEBUG_PRINT("chillAST ParmVarDecl for %s from chill location 0x%x\n", decl->varname, clangvardecl);
  } // for each parameter

  if (D->getBuiltinID()) {
    chillFD->setExtern();
    CHILL_DEBUG_PRINT("%s is builtin (extern)\n", FuncName.c_str());
  };

  Stmt *clangbody = D->getBody();
  if (clangbody) {
    // may just be fwd decl or external, without an actual body
    chillAST_NodeList* nl = ConvertGenericClangAST(clangbody);
    chillFD->setBody(unwrap(nl));
  }

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
  bool pre = !clangUO->isPostfix();
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

  DeclarationName DN = DNI.getName();
  const char *varname = DN.getAsString().c_str();
  chillAST_DeclRefExpr *chillDRE = new chillAST_DeclRefExpr(TypeStr.c_str(), varname, NULL);

  // find the definition (we hope) TODO Wrong treatment
  if ((!strcmp("Var", vd->getDeclKindName())) || (!strcmp("ParmVar", vd->getDeclKindName()))) {
    // it's a variable reference
    int numvars = VariableDeclarations.size();
    chillAST_VarDecl *chillvd = NULL;
    for (int i = 0; i < numvars; i++) {
      if (VariableDeclarations[i]->uniquePtr == vd)
        chillvd = VariableDeclarations[i];
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
      CHILL_ERROR("chillDRE->decl = 0x%x\n", chillvd);
      exit(-1);
    }

    chillDRE->decl = (chillAST_Node *) chillvd;
  } else if (!strcmp("Function", vd->getDeclKindName())) {
    int numfuncs = FunctionDeclarations.size();
    chillAST_FunctionDecl *chillfd = NULL;
    for (int i = 0; i < numfuncs; i++) {
      if (FunctionDeclarations[i]->uniquePtr == vd)
        chillfd = FunctionDeclarations[i];
    }
    if (chillfd == NULL) {
      CHILL_ERROR("chillDRE->decl = 0x%x\n", chillfd);
      exit(-1);
    }
    chillDRE->decl = (chillAST_Node *) chillfd;
  } else {
    CHILL_ERROR("clang DeclRefExpr refers to declaration of %s of kind %s\n", varname, vd->getDeclKindName());
    exit(-1);
  }
  NL_RET(chillDRE);
}


chillAST_NodeList* ConvertIntegerLiteral(IntegerLiteral *clangIL) {
  bool isSigned = clangIL->getType()->isSignedIntegerType();
  const char *printable = clangIL->getValue().toString(10, isSigned).c_str();
  int val = stoi(printable);
  chillAST_IntegerLiteral *chillIL = new chillAST_IntegerLiteral(val);
  NL_RET(chillIL);
}


chillAST_NodeList* ConvertFloatingLiteral(FloatingLiteral *clangFL) {
  // Get an appriximate value of the APFloat
  float val = clangFL->getValueAsApproximateDouble();
  SmallString<16> Str;
  clangFL->getValue().toString(Str);

  SourceLocation sloc = clangFL->getLocStart();
  SourceLocation eloc = clangFL->getLocEnd();

  std::string start = sloc.printToString(*globalSRCMAN);
  const char *filename = globalSRCMAN->getBufferName(sloc);

  if (filename && strlen(filename) > 0) {}
  else {
    fprintf(stderr, "\nConvertFloatingLiteral() filename is NULL?\n");
    sloc = globalSRCMAN->getSpellingLoc(sloc);  // should get spelling loc?
    start = sloc.printToString(*globalSRCMAN);
    filename = globalSRCMAN->getBufferName(sloc);
  }

  unsigned int offset = globalSRCMAN->getFileOffset(sloc);
  FILE *fp = fopen(filename, "r");
  fseek(fp, offset, SEEK_SET); // go to the part of the file where the float is defined
  char buf[10240];
  fgets(buf, sizeof(buf), fp); // read a line starting where the float starts
  fclose(fp);
  // buf has the line we want   grab the float constant out of it
  char *ptr = buf;
  if (*ptr == '-') ptr++; // ignore possible minus sign
  int len = strspn(ptr, ".-0123456789f");
  buf[len] = '\0';
  chillAST_FloatingLiteral *chillFL = new chillAST_FloatingLiteral(val,1, buf);
  NL_RET(chillFL);
}


chillAST_NodeList* ConvertImplicitCastExpr(ImplicitCastExpr *clangICE) {
  chillAST_Node *sub = unwrap(ConvertGenericClangAST(clangICE->getSubExpr()));
  chillAST_ImplicitCastExpr *chillICE = new chillAST_ImplicitCastExpr(sub);

  NL_RET(chillICE);
}


chillAST_NodeList* ConvertCStyleCastExpr(CStyleCastExpr *clangCSCE) {
  const char *towhat = strdup(clangCSCE->getTypeAsWritten().getAsString().c_str());

  chillAST_Node *sub = unwrap(ConvertGenericClangAST(clangCSCE->getSubExprAsWritten()));
  chillAST_CStyleCastExpr *chillCSCE = new chillAST_CStyleCastExpr(towhat, sub);
  NL_RET(chillCSCE);
}


chillAST_NodeList* ConvertReturnStmt(ReturnStmt *clangRS) {
  chillAST_Node *retval = unwrap(ConvertGenericClangAST(clangRS->getRetValue())); // NULL is handled

  chillAST_ReturnStmt *chillRS = new chillAST_ReturnStmt(retval);
  NL_RET(chillRS);
}


chillAST_NodeList* ConvertCallExpr(CallExpr *clangCE) {
  chillAST_Node *callee = unwrap(ConvertGenericClangAST(clangCE->getCallee()));
  chillAST_CallExpr *chillCE = new chillAST_CallExpr(callee);
  int numargs = clangCE->getNumArgs();
  Expr **clangargs = clangCE->getArgs();
  for (int i = 0; i < numargs; i++)
    chillCE->addArg(unwrap(ConvertGenericClangAST(clangargs[i])));

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

    if (D->isImplicit() || !globalSRCMAN->getFilename(D->getLocation()).equals(filename)) child->isFromSourceFile = false;
  }

  NL_RET(topnode);
}

chillAST_NodeList* ConvertMemberExpr(clang::MemberExpr *clangME) {
  chillAST_Node *base = unwrap(ConvertGenericClangAST(clangME->getBase()));

  DeclarationNameInfo memnameinfo = clangME->getMemberNameInfo();
  DeclarationName DN = memnameinfo.getName();
  const char *member = DN.getAsString().c_str();

  chillAST_MemberExpr *ME = new chillAST_MemberExpr(base, member, clangME);

  NL_RET(ME);

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
  } else if (isa<FunctionDecl>(D)) {
    ret = ConvertFunctionDecl( dyn_cast<FunctionDecl>(D));
  } else if (isa<VarDecl>(D)) {
    ret = ConvertVarDecl( dyn_cast<VarDecl>(D) );
  } else if (isa<TypedefDecl>(D)) {
    ret =  ConvertTypeDefDecl( dyn_cast<TypedefDecl>(D));
  } else {
    // more work to do
    if (isa<Decl>(D)) CHILL_ERROR("Decl of kind %s unhandled\n",  D->getDeclKindName() );
    if (isa<Stmt>(s)) CHILL_ERROR("Stmt of type %s unhandled\n", s->getStmtClassName());
    exit(-1);
  }

  return ret;
}

class NULLASTConsumer : public ASTConsumer {
};

aClangCompiler::aClangCompiler(char *filename) {
  SourceFileName = strdup(filename);

  // Arguments to pass to the clang frontend
  std::vector<const char *> args;
  args.push_back(strdup(filename));

  // The compiler invocation needs a DiagnosticsEngine so it can report problems
  diagnosticOptions = new DiagnosticOptions(); // private member of aClangCompiler

  pTextDiagnosticPrinter = new clang::TextDiagnosticPrinter(llvm::errs(),
                                                            diagnosticOptions); // private member of aClangCompiler
  diagnosticsEngine = new clang::DiagnosticsEngine(diagID, diagnosticOptions, pTextDiagnosticPrinter);

  // Create the compiler invocation
  // This class is designed to represent an abstract "invocation" of the compiler,
  // including data such as the include paths, the code generation options,
  // the warning flags, and so on.
  std::unique_ptr<clang::CompilerInvocation> CI(new clang::CompilerInvocation());
  clang::CompilerInvocation::CreateFromArgs(*CI, &args[0], &args[0] + args.size(), *diagnosticsEngine);
  // Create the compiler instance
  Clang = new clang::CompilerInstance();  // TODO should have a better name ClangCompilerInstance


  // Get ready to report problems
  Clang->createDiagnostics(nullptr, true);
  //Clang.createDiagnostics(0, 0);


  targetOptions = std::make_shared<clang::TargetOptions>();
  targetOptions->Triple = llvm::sys::getDefaultTargetTriple();

  TargetInfo *pti = TargetInfo::CreateTargetInfo(Clang->getDiagnostics(), targetOptions);

  Clang->setTarget(pti);
  Clang->createFileManager();
  FileManager &FileMgr = Clang->getFileManager();
  fileManager = &FileMgr;
  Clang->createSourceManager(FileMgr);
  SourceManager &SourceMgr = Clang->getSourceManager();
  sourceManager = &SourceMgr; // ?? aclangcompiler copy
  globalSRCMAN = &SourceMgr; //  TODO   global bad

  Clang->setInvocation(CI.get()); // Replace the current invocation

  Clang->createPreprocessor(TU_Complete);

  Clang->createASTContext();                              // needs preprocessor
  astContext_ = &Clang->getASTContext();
  const FileEntry *FileIn = FileMgr.getFile(filename); // needs preprocessor
  SourceMgr.setMainFileID(SourceMgr.createFileID(FileIn, clang::SourceLocation(), clang::SrcMgr::C_User));
  Clang->getDiagnosticClient().BeginSourceFile(Clang->getLangOpts(), &Clang->getPreprocessor());

  NULLASTConsumer TheConsumer; // must pass a consumer in to ParseAST(). This one does nothing
  CHILL_DEBUG_PRINT("actually parsing file %s using clang\n", filename);
  ParseAST(Clang->getPreprocessor(), &TheConsumer, Clang->getASTContext());
  // Translation Unit is contents of a file
  TranslationUnitDecl *TUD = astContext_->getTranslationUnitDecl();
  // create another AST, very similar to the clang AST but not written by idiots
  CHILL_DEBUG_PRINT("converting entire clang AST into chill AST (ir_clang.cc)\n");
  chillAST_Node *wholefile = unwrap(ConvertTranslationUnit(TUD, filename));
  entire_file_AST = (chillAST_SourceFile *) wholefile;
  astContext_ = &Clang->getASTContext();
}

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

  int numc = node->getNumChildren();

  for (int i = 0; i < numc; i++)
    findmanually(node->getChild(i), procname, procs);
  return;
}

chillAST_FunctionDecl *aClangCompiler::findprocedurebyname(char *procname) {

  std::vector<chillAST_Node *> procs;
  findmanually(entire_file_AST, procname, procs);

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