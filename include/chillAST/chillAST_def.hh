

#ifndef _CHILLAST_DEF_H_
#define _CHILLAST_DEF_H_


#define CHILL_INDENT_AMOUNT 2

#include <iostream>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <vector>  // std::vector 

#include <ir_enums.hh> // for IR_CONDITION_* 

#define CHILLAST_NODETYPE_FORSTMT CHILLAST_NODETYPE_LOOP
#define CHILLAST_NODETYPE_TRANSLATIONUNIT CHILLAST_NODETYPE_SOURCEFILE

enum CHILL_ASTNODE_TYPE {
  CHILLAST_NODETYPE_UNKNOWN = 0,
  CHILLAST_NODETYPE_SOURCEFILE,
  CHILLAST_NODETYPE_TYPEDEFDECL,
  CHILLAST_NODETYPE_VARDECL,
  //  CHILLAST_NODETYPE_PARMVARDECL,   not used any more 
  CHILLAST_NODETYPE_FUNCTIONDECL,
  CHILLAST_NODETYPE_RECORDDECL,     // struct or union (or class) 
  CHILLAST_NODETYPE_MACRODEFINITION,
  CHILLAST_NODETYPE_COMPOUNDSTMT,
  CHILLAST_NODETYPE_LOOP,               // AKA ForStmt
  CHILLAST_NODETYPE_TERNARYOPERATOR,
  CHILLAST_NODETYPE_BINARYOPERATOR,
  CHILLAST_NODETYPE_UNARYOPERATOR,
  CHILLAST_NODETYPE_ARRAYSUBSCRIPTEXPR,
  CHILLAST_NODETYPE_MEMBEREXPR,          // structs/unions
  CHILLAST_NODETYPE_DECLREFEXPR,
  CHILLAST_NODETYPE_INTEGERLITERAL,
  CHILLAST_NODETYPE_FLOATINGLITERAL,
  CHILLAST_NODETYPE_IMPLICITCASTEXPR,
  CHILLAST_NODETYPE_RETURNSTMT,
  CHILLAST_NODETYPE_CALLEXPR,
  CHILLAST_NODETYPE_DECLSTMT,
  CHILLAST_NODETYPE_PARENEXPR,
  CHILLAST_NODETYPE_CSTYLECASTEXPR,
  CHILLAST_NODETYPE_CSTYLEADDRESSOF,
  CHILLAST_NODETYPE_IFSTMT,
  CHILLAST_NODETYPE_SIZEOF,
  CHILLAST_NODETYPE_MALLOC,
  CHILLAST_NODETYPE_FREE,
  CHILLAST_NODETYPE_PREPROCESSING, // comments, #define, #include, whatever else works
  CHILLAST_NODETYPE_NOOP,   // NO OP
  // CUDA specific
  CHILLAST_NODETYPE_CUDAMALLOC,
  CHILLAST_NODETYPE_CUDAFREE,
  CHILLAST_NODETYPE_CUDAMEMCPY,
  CHILLAST_NODETYPE_CUDAKERNELCALL,
  CHILLAST_NODETYPE_CUDASYNCTHREADS,
  CHILLAST_NODETYPE_NULL    // explicit non-statement 
  // TODO 

};

enum CHILL_FUNCTION_TYPE {
  CHILL_FUNCTION_CPU = 0,
  CHILL_FUNCTION_GPU
};

enum CHILL_MEMBER_EXP_TYPE {
  CHILL_MEMBER_EXP_DOT = 0,
  CHILL_MEMBER_EXP_ARROW
};

enum CHILL_PREPROCESSING_TYPE {
  CHILL_PREPROCESSING_TYPEUNKNOWN = 0,
  CHILL_PREPROCESSING_COMMENT,
  CHILL_PREPROCESSING_POUNDDEFINE,
  CHILL_PREPROCESSING_POUNDINCLUDE,
  CHILL_PREPROCESSING_PRAGMA  // unused so far
};

enum CHILL_PREPROCESSING_POSITION { // when tied to another statement
  CHILL_PREPROCESSING_POSITIONUNKNOWN = 0,
  CHILL_PREPROCESSING_LINEBEFORE,       // previous line 
  CHILL_PREPROCESSING_LINEAFTER,        // next line 
  CHILL_PREPROCESSING_TOTHERIGHT,       // for this kind of comment, on same line
  CHILL_PREPROCESSING_IMMEDIATELYBEFORE // on same line 
};

char *parseUnderlyingType(char *sometype);

char *parseArrayParts(char *sometype);

bool isRestrict(const char *sometype);

char *splitTypeInfo(char *underlyingtype);

char *ulhack(char *brackets); // change "1024UL" to "1024"
char *restricthack(char *typeinfo); // remove __restrict__ , MODIFIES the argument!

extern const char *Chill_AST_Node_Names[];  // WARNING MUST BE KEPT IN SYNC WITH BELOW LIST

// fwd declarations
class chillAST_node;         // the generic node. specific types derive from this
class chillAST_NULL;         // empty
class chillAST_SourceFile;  // ast for an entire source file (translationunit)

class chillAST_TypedefDecl;

class chillAST_VarDecl;

//class chillAST_ParmVarDecl;
class chillAST_FunctionDecl;

class chillAST_RecordDecl;       // structs and unions (and classes?)
class chillAST_MacroDefinition;

class chillAST_CompoundStmt;  // just a bunch of other statements
class chillAST_ForStmt;    // AKA a LOOP
class chillAST_TernaryOperator;

class chillAST_BinaryOperator;

class chillAST_ArraySubscriptExpr;

class chillAST_MemberExpr;

class chillAST_DeclRefExpr;

class chillAST_IntegerLiteral;

class chillAST_FloatingLiteral;

class chillAST_UnaryOperator;

class chillAST_ImplicitCastExpr;

class chillAST_CStyleCastExpr;

class chillAST_CStyleAddressOf;

class chillAST_ReturnStmt;

class chillAST_CallExpr;

class chillAST_ParenExpr;

class chillAST_Sizeof;

class chillAST_Malloc;

class chillAST_Free;

class chillAST_NoOp;

class chillAST_CudaMalloc;

class chillAST_CudaFree;

class chillAST_CudaMemcpy;

class chillAST_CudaKernelCall;

class chillAST_CudaSyncthreads;

class chillAST_Preprocessing;

typedef std::vector<chillAST_VarDecl *> chillAST_SymbolTable;  // typedef
typedef std::vector<chillAST_TypedefDecl *> chillAST_TypedefTable;  // typedef

bool symbolTableHasVariableNamed(chillAST_SymbolTable *table, const char *name); // fwd decl
chillAST_VarDecl *symbolTableFindVariableNamed(chillAST_SymbolTable *table,
                                               const char *name); // fwd decl TODO too many similar named functions

void printSymbolTable(chillAST_SymbolTable *st); // fwd decl
void printSymbolTableMoreInfo(chillAST_SymbolTable *st); // fwd decl


chillAST_node *lessthanmacro(chillAST_node *left, chillAST_node *right);  // fwd declaration
chillAST_SymbolTable *addSymbolToTable(chillAST_SymbolTable *st, chillAST_VarDecl *vd); // fwd decl
chillAST_TypedefTable *addTypedefToTable(chillAST_TypedefTable *tt, chillAST_TypedefDecl *td); // fwd decl


bool streq(const char *a, const char *b); // fwd decl
void chillindent(int i, FILE *fp);  // fwd declaration
void insertNewDeclAtLocationOfOldIfNeeded(chillAST_VarDecl *newdecl, chillAST_VarDecl *olddecl);

chillAST_DeclRefExpr *buildDeclRefExpr(chillAST_VarDecl *);

chillAST_FunctionDecl *findFunctionDecl(chillAST_node *node, const char *procname);

#endif

