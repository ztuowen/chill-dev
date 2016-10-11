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


std::vector<chillAST_VarDecl *> VariableDeclarations;
std::vector<chillAST_FunctionDecl *> FunctionDeclarations;

using namespace clang;
using namespace clang::driver;
using namespace omega;
using namespace std;




// ----------------------------------------------------------------------------
// Class: IR_chillScalarSymbol
// ----------------------------------------------------------------------------

std::string IR_chillScalarSymbol::name() const {
  return std::string(chillvd->varname);  // CHILL
}


// Return size in bytes
int IR_chillScalarSymbol::size() const {
  CHILL_DEBUG_PRINT("IR_chillScalarSymbol::size()  probably WRONG\n");
  return (8); // bytes?? 
}


bool IR_chillScalarSymbol::operator==(const IR_Symbol &that) const {
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
  return chillvd->numdimensions;
}


// TODO
omega::CG_outputRepr *IR_chillArraySymbol::size(int dim) const {
  CHILL_ERROR("IR_chillArraySymbol::n_size()  TODO \n");
  exit(-1);
  return NULL;
}


bool IR_chillArraySymbol::operator!=(const IR_Symbol &that) const {
  return chillvd != ((IR_chillArraySymbol *) &that)->chillvd;
}

bool IR_chillArraySymbol::operator==(const IR_Symbol &that) const {
  return chillvd == ((IR_chillArraySymbol *) &that)->chillvd;
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
  chillAST_VarDecl *vd = NULL;
  if (chillvd) vd = chillvd;
  return new IR_chillScalarSymbol(ir_, vd);
}


bool IR_chillScalarRef::operator==(const IR_Ref &that) const {
  if (typeid(*this) != typeid(that))
    return false;

  const IR_chillScalarRef *l_that = static_cast<const IR_chillScalarRef *>(&that);

  return this->chillvd == l_that->chillvd;
}


omega::CG_outputRepr *IR_chillScalarRef::convert() {
  if (!dre) CHILL_ERROR("IR_chillScalarRef::convert()   CLANG SCALAR REF has no dre\n");
  omega::CG_chillRepr *result = new omega::CG_chillRepr(dre);
  delete this;
  return result;
}

IR_Ref *IR_chillScalarRef::clone() const {
  if (dre) return new IR_chillScalarRef(ir_, dre);
  return new IR_chillScalarRef(ir_, chillvd);
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
  well_formed = true;

  ir_ = ir;
  chillforstmt = achillforstmt;

  chillAST_BinaryOperator *init = (chillAST_BinaryOperator *) chillforstmt->getInit();
  chillAST_BinaryOperator *cond = (chillAST_BinaryOperator *) chillforstmt->getCond();
  // check to be sure  (assert)
  if (!init || !cond || !init->isAssignmentOp() || !cond->isComparisonOp()) {
    CHILL_ERROR("malformed loop init or cond:\n");
    well_formed = false;
    return;
  }

  chilllowerbound = new CG_chillRepr(init->getRHS());
  chillupperbound = new CG_chillRepr(cond->getRHS());
  conditionoperator = achillforstmt->conditionoperator;

  chillAST_Node *inc = chillforstmt->getInc();
  // check the increment
  if (inc->getType() == CHILLAST_NODE_UNARYOPERATOR) {
    if (!strcmp(((chillAST_UnaryOperator *) inc)->op, "++")) step_size_ = 1;
    else step_size_ = -1;
  } else if (inc->getType() == CHILLAST_NODE_BINARYOPERATOR) {
    int beets = false;
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
      well_formed = false;
      return;
    }
  } // binary operator 
  else {
    CHILL_ERROR("IR_chillLoop constructor, unhandled loop increment\n");
    well_formed = false;
    return;
  }
  chillAST_DeclRefExpr *dre = (chillAST_DeclRefExpr *) init->getLHS();
  if (!dre->isDeclRefExpr()) {
    CHILL_ERROR("Malformed loop init, unhandled loop increment\n");
    well_formed = false;
    return;
  }
  chillindex = dre; // the loop index variable
  chillbody = achillforstmt->getBody();
}


omega::CG_outputRepr *IR_chillLoop::lower_bound() const {
  CHILL_DEBUG_PRINT("IR_xxxxLoop::lower_bound()\n");
  return chilllowerbound;
}

omega::CG_outputRepr *IR_chillLoop::upper_bound() const {
  CHILL_DEBUG_PRINT("IR_xxxxLoop::upper_bound()\n");
  return chillupperbound;
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
          parent = loop->getParent();
        } else {
          if (parent != loop->getParent()) {
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
            parent = blockstmts[j]->getParent();
          } else {
            if (parent != blockstmts[j]->getParent()) {
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

      par = forstmt->getParent();
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
      par = ((IR_chillBlock*)old)->statements[0]->getParent();
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
  if (typeid(*old) == typeid(IR_chillArrayRef)) {
    IR_chillArrayRef *CAR = (IR_chillArrayRef *) old;
    chillAST_ArraySubscriptExpr *CASE = CAR->chillASE;

    omega::CG_chillRepr *crepr = (omega::CG_chillRepr *) repr;
    if (crepr->chillnodes.size() != 1) {
      CHILL_ERROR("IR_clangCode::ReplaceExpression(), replacing with %d chillnodes???\n");
      //exit(-1);
    }

    chillAST_Node *newthing = crepr->chillnodes[0];

    if (!CASE->getParent()) {
      CHILL_ERROR("IR_clangCode::ReplaceExpression()  old has no parent ??\n");
      exit(-1);
    }

    CASE->getParent()->replaceChild(CASE, newthing);

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
  CG_chillRepr *crepr = (CG_chillRepr *)repr;
  chillAST_Node *node = crepr->chillnodes[0];
  if (!node->isBinaryOperator())
    return IR_COND_UNKNOWN;
  chillAST_BinaryOperator *bin = (chillAST_BinaryOperator*)node;
  const char * opstring = bin->getOp();
  if (!strcmp(opstring, "==")) return IR_COND_EQ;
  if (!strcmp(opstring, "<=")) return IR_COND_LE;
  if (!strcmp(opstring, "<")) return IR_COND_LT;
  if (!strcmp(opstring, ">")) return IR_COND_GT;
  if (!strcmp(opstring, ">=")) return IR_COND_GE;
  if (!strcmp(opstring, "!=")) return IR_COND_NE;
  return IR_COND_UNKNOWN;
}


IR_OPERATION_TYPE IR_clangCode::QueryExpOperation(const omega::CG_outputRepr *repr) const {
  CG_chillRepr *crepr = (CG_chillRepr *) repr;
  chillAST_Node *node = crepr->chillnodes[0];
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
    } else {
      v.push_back(new omega::CG_chillRepr(bop->getLHS()));  // for +*-/ return both lhs and rhs
      v.push_back(new omega::CG_chillRepr(bop->getRHS()));
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
    CHILL_ERROR("ir_clang.cc IR_clangCode::Repr2Ref() UNHANDLED node type %s\n", node->getTypeString());
    exit(-1);
  }
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
