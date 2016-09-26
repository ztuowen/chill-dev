

#ifndef _CHILLAST_DEF_H_
#define _CHILLAST_DEF_H_

#include "chilldebug.h"

#define CHILL_INDENT_AMOUNT 2

#include <iostream>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <vector>  // std::vector

#include <ir_enums.hh> // for IR_CONDITION_*

enum CHILLAST_NODE_TYPE {
  CHILLAST_NODE_UNKNOWN = 0,
  CHILLAST_NODE_TRANSLATIONUNIT = 1,
  CHILLAST_NODE_SOURCEFILE = 1,
  CHILLAST_NODE_TYPEDEFDECL,
  CHILLAST_NODE_VARDECL,
  //  CHILLAST_NODE_PARMVARDECL,   not used any more
      CHILLAST_NODE_FUNCTIONDECL,
  CHILLAST_NODE_RECORDDECL,     // struct or union (or class)
  CHILLAST_NODE_MACRODEFINITION,
  CHILLAST_NODE_COMPOUNDSTMT,
  CHILLAST_NODE_LOOP = 8,
  CHILLAST_NODE_FORSTMT = 8,
  CHILLAST_NODE_TERNARYOPERATOR,
  CHILLAST_NODE_BINARYOPERATOR,
  CHILLAST_NODE_UNARYOPERATOR,
  CHILLAST_NODE_ARRAYSUBSCRIPTEXPR,
  CHILLAST_NODE_MEMBEREXPR,          // structs/unions
  CHILLAST_NODE_DECLREFEXPR,
  CHILLAST_NODE_INTEGERLITERAL,
  CHILLAST_NODE_FLOATINGLITERAL,
  CHILLAST_NODE_IMPLICITCASTEXPR,
  CHILLAST_NODE_RETURNSTMT,
  CHILLAST_NODE_CALLEXPR,
  //CHILLAST_NODE_DECLSTMT, not used
      CHILLAST_NODE_PARENEXPR,
  CHILLAST_NODE_CSTYLECASTEXPR,
  CHILLAST_NODE_CSTYLEADDRESSOF,
  CHILLAST_NODE_IFSTMT,
  CHILLAST_NODE_SIZEOF,
  CHILLAST_NODE_MALLOC,
  CHILLAST_NODE_FREE,
  CHILLAST_NODE_PREPROCESSING, // comments, #define, #include, whatever else works
  CHILLAST_NODE_NOOP,   // NO OP
  // CUDA specific
      CHILLAST_NODE_CUDAMALLOC,
  CHILLAST_NODE_CUDAFREE,
  CHILLAST_NODE_CUDAMEMCPY,
  CHILLAST_NODE_CUDAKERNELCALL,
  CHILLAST_NODE_CUDASYNCTHREADS,
  CHILLAST_NODE_NULL    // explicit non-statement
  // TODO 

};

enum CHILLAST_FUNCTION_TYPE {
  CHILLAST_FUNCTION_CPU = 0,
  CHILLAST_FUNCTION_GPU
};

enum CHILLAST_MEMBER_EXP_TYPE {
  CHILLAST_MEMBER_EXP_DOT = 0,
  CHILLAST_MEMBER_EXP_ARROW
};

enum CHILLAST_PREPROCESSING_TYPE {
  CHILLAST_PREPROCESSING_TYPEUNKNOWN = 0,
  CHILLAST_PREPROCESSING_COMMENT,
  CHILLAST_PREPROCESSING_POUNDDEFINE,
  CHILLAST_PREPROCESSING_POUNDINCLUDE,
  CHILLAST_PREPROCESSING_PRAGMA  // unused so far
};

enum CHILLAST_PREPROCESSING_POSITION { // when tied to another statement
  CHILLAST_PREPROCESSING_POSITIONUNKNOWN = 0,
  CHILLAST_PREPROCESSING_LINEBEFORE,       // previous line
  CHILLAST_PREPROCESSING_LINEAFTER,        // next line
  CHILLAST_PREPROCESSING_TOTHERIGHT,       // for this kind of comment, on same line
  CHILLAST_PREPROCESSING_IMMEDIATELYBEFORE // on same line
};

char *parseUnderlyingType(const char *sometype);

char *parseArrayParts(char *sometype);

bool isRestrict(const char *sometype);

char *splitTypeInfo(char *underlyingtype);

//! change "1024UL" to "1024"
char *ulhack(char *brackets);

//! remove __restrict__ , MODIFIES the argument!
char *restricthack(char *typeinfo);

// TODO phase out this one unnecessary as we can write a virtual function that does this
extern const char *ChillAST_Node_Names[];  // WARNING MUST BE KEPT IN SYNC WITH BELOW LIST

// fwd declarations
//! the generic node, who specific types are derived from
class chillAST_Node;

//! empty node
class chillAST_NULL;

//! ast for an entire source file (translationunit)
class chillAST_SourceFile;

class chillAST_TypedefDecl;

class chillAST_VarDecl;

//class chillAST_ParmVarDecl;
class chillAST_FunctionDecl;

//! structs and unions (and classes?)
class chillAST_RecordDecl;

class chillAST_MacroDefinition;

//! A sequence of statements
class chillAST_CompoundStmt;

//! a For LOOP
class chillAST_ForStmt;

class chillAST_UnaryOperator;

class chillAST_BinaryOperator;

class chillAST_TernaryOperator;

class chillAST_ArraySubscriptExpr;

class chillAST_MemberExpr;

class chillAST_DeclRefExpr;

class chillAST_IntegerLiteral;

class chillAST_FloatingLiteral;

class chillAST_ImplicitCastExpr;

class chillAST_IfStmt;

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

// Not implemented
class chillAST_CudaKernelCall;

class chillAST_CudaSyncthreads;

class chillAST_Preprocessing;

typedef std::vector<chillAST_Node *> chillAST_NodeList;
typedef std::vector<chillAST_VarDecl *> chillAST_SymbolTable;
typedef std::vector<chillAST_TypedefDecl *> chillAST_TypedefTable;

chillAST_VarDecl *symbolTableFindName(chillAST_SymbolTable *table, const char *name);

chillAST_VarDecl *symbolTableFindVariableNamed(chillAST_SymbolTable *table,
                                               const char *name);

chillAST_TypedefDecl *typedefTableFindName(chillAST_TypedefTable *table, const char *name);

chillAST_Node *lessthanmacro(chillAST_Node *left, chillAST_Node *right);

chillAST_SymbolTable *addSymbolToTable(chillAST_SymbolTable *st, chillAST_VarDecl *vd);

chillAST_TypedefTable *addTypedefToTable(chillAST_TypedefTable *tt, chillAST_TypedefDecl *td);


void chillindent(int i, FILE *fp);

void insertNewDeclAtLocationOfOldIfNeeded(chillAST_VarDecl *newdecl, chillAST_VarDecl *olddecl);

chillAST_DeclRefExpr *buildDeclRefExpr(chillAST_VarDecl *);

chillAST_FunctionDecl *findFunctionDecl(chillAST_Node *node, const char *procname);

#endif

