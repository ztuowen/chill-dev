//
// Created by ztuowen on 9/24/16.
//

#include "printer/generic.h"

using namespace chill::printer;

void GenericPrinter::print(std::string ident, chillAST_Node *n, std::ostream &o) {
  switch (n->getType()) {
    case CHILLAST_NODE_ARRAYSUBSCRIPTEXPR:
      printS(ident, dynamic_cast<chillAST_ArraySubscriptExpr *>(n), o);
      break;
    case CHILLAST_NODE_BINARYOPERATOR:
      printS(ident, dynamic_cast<chillAST_BinaryOperator *>(n), o);
      break;
    case CHILLAST_NODE_CALLEXPR:
      printS(ident, dynamic_cast<chillAST_CallExpr *>(n), o);
      break;
    case CHILLAST_NODE_COMPOUNDSTMT:
      printS(ident, dynamic_cast<chillAST_CompoundStmt *>(n), o);
      break;
    case CHILLAST_NODE_CSTYLEADDRESSOF:
      printS(ident, dynamic_cast<chillAST_CStyleAddressOf *>(n), o);
      break;
    case CHILLAST_NODE_CSTYLECASTEXPR:
      printS(ident, dynamic_cast<chillAST_CStyleCastExpr *>(n), o);
      break;
    case CHILLAST_NODE_CUDAFREE:
      printS(ident, dynamic_cast<chillAST_CudaFree *>(n), o);
      break;
//    case CHILLAST_NODE_CUDAKERNELCALL:
//      printS(ident, dynamic_cast<chillAST_CudaKernelCall *>(n), o);
//      break;
    case CHILLAST_NODE_CUDAMALLOC:
      printS(ident, dynamic_cast<chillAST_CudaMalloc *>(n), o);
      break;
    case CHILLAST_NODE_CUDAMEMCPY:
      printS(ident, dynamic_cast<chillAST_CudaMemcpy *>(n), o);
      break;
    case CHILLAST_NODE_CUDASYNCTHREADS:
      printS(ident, dynamic_cast<chillAST_CudaSyncthreads *>(n), o);
      break;
    case CHILLAST_NODE_DECLREFEXPR:
      printS(ident, dynamic_cast<chillAST_DeclRefExpr *>(n), o);
      break;
    case CHILLAST_NODE_FLOATINGLITERAL:
      printS(ident, dynamic_cast<chillAST_FloatingLiteral *>(n), o);
      break;
    case CHILLAST_NODE_LOOP:
//    case CHILLAST_NODE_FORSTMT:
      printS(ident, dynamic_cast<chillAST_ForStmt *>(n), o);
      break;
    case CHILLAST_NODE_FREE:
      printS(ident, dynamic_cast<chillAST_Free *>(n), o);
      break;
    case CHILLAST_NODE_FUNCTIONDECL:
      printS(ident, dynamic_cast<chillAST_FunctionDecl *>(n), o);
      break;
    case CHILLAST_NODE_IFSTMT:
      printS(ident, dynamic_cast<chillAST_IfStmt *>(n), o);
      break;
    case CHILLAST_NODE_IMPLICITCASTEXPR:
      printS(ident, dynamic_cast<chillAST_ImplicitCastExpr *>(n), o);
      break;
    case CHILLAST_NODE_INTEGERLITERAL:
      printS(ident, dynamic_cast<chillAST_IntegerLiteral *>(n), o);
      break;
    case CHILLAST_NODE_MACRODEFINITION:
      printS(ident, dynamic_cast<chillAST_MacroDefinition *>(n), o);
      break;
    case CHILLAST_NODE_MALLOC:
      printS(ident, dynamic_cast<chillAST_Malloc *>(n), o);
      break;
    case CHILLAST_NODE_MEMBEREXPR:
      printS(ident, dynamic_cast<chillAST_MemberExpr *>(n), o);
      break;
    case CHILLAST_NODE_NOOP:
      printS(ident, dynamic_cast<chillAST_NoOp *>(n), o);
      break;
    case CHILLAST_NODE_NULL:
      printS(ident, dynamic_cast<chillAST_NULL *>(n), o);
      break;
    case CHILLAST_NODE_PARENEXPR:
      printS(ident, dynamic_cast<chillAST_ParenExpr *>(n), o);
      break;
    case CHILLAST_NODE_PREPROCESSING:
      printS(ident, dynamic_cast<chillAST_Preprocessing *>(n), o);
      break;
    case CHILLAST_NODE_RECORDDECL:
      printS(ident, dynamic_cast<chillAST_RecordDecl *>(n), o);
      break;
    case CHILLAST_NODE_RETURNSTMT:
      printS(ident, dynamic_cast<chillAST_ReturnStmt *>(n), o);
      break;
    case CHILLAST_NODE_SIZEOF:
      printS(ident, dynamic_cast<chillAST_Sizeof *>(n), o);
      break;
    case CHILLAST_NODE_TRANSLATIONUNIT:
//    case CHILLAST_NODE_SOURCEFILE:
      printS(ident, dynamic_cast<chillAST_SourceFile *>(n), o);
      break;
    case CHILLAST_NODE_TERNARYOPERATOR:
      printS(ident, dynamic_cast<chillAST_TernaryOperator *>(n), o);
      break;
    case CHILLAST_NODE_TYPEDEFDECL:
      printS(ident, dynamic_cast<chillAST_TypedefDecl *>(n), o);
      break;
    case CHILLAST_NODE_UNARYOPERATOR:
      printS(ident, dynamic_cast<chillAST_UnaryOperator *>(n), o);
      break;
    case CHILLAST_NODE_VARDECL:
      printS(ident, dynamic_cast<chillAST_VarDecl *>(n), o);
      break;
    case CHILLAST_NODE_UNKNOWN:
    default:
      CHILL_ERROR("Printing an unknown type of Node: %s\n", n->getTypeString());
  }
  o.flush();
}

int GenericPrinter::getPrec(chillAST_Node *n) {
  switch (n->getType()) {
    case CHILLAST_NODE_ARRAYSUBSCRIPTEXPR:
      return getPrecS(dynamic_cast<chillAST_ArraySubscriptExpr *>(n));
    case CHILLAST_NODE_BINARYOPERATOR:
      return getPrecS(dynamic_cast<chillAST_BinaryOperator *>(n));
    case CHILLAST_NODE_CALLEXPR:
      return getPrecS(dynamic_cast<chillAST_CallExpr *>(n));
    case CHILLAST_NODE_COMPOUNDSTMT:
      return getPrecS(dynamic_cast<chillAST_CompoundStmt *>(n));
    case CHILLAST_NODE_CSTYLEADDRESSOF:
      return getPrecS(dynamic_cast<chillAST_CStyleAddressOf *>(n));
    case CHILLAST_NODE_CSTYLECASTEXPR:
      return getPrecS(dynamic_cast<chillAST_CStyleCastExpr *>(n));
    case CHILLAST_NODE_CUDAFREE:
      return getPrecS(dynamic_cast<chillAST_CudaFree *>(n));
//    case CHILLAST_NODE_CUDAKERNELCALL:
//      return getPrecS(dynamic_cast<chillAST_CudaKernelCall *>(n));
    case CHILLAST_NODE_CUDAMALLOC:
      return getPrecS(dynamic_cast<chillAST_CudaMalloc *>(n));
    case CHILLAST_NODE_CUDAMEMCPY:
      return getPrecS(dynamic_cast<chillAST_CudaMemcpy *>(n));
    case CHILLAST_NODE_CUDASYNCTHREADS:
      return getPrecS(dynamic_cast<chillAST_CudaSyncthreads *>(n));
    case CHILLAST_NODE_DECLREFEXPR:
      return getPrecS(dynamic_cast<chillAST_DeclRefExpr *>(n));
    case CHILLAST_NODE_FLOATINGLITERAL:
      return getPrecS(dynamic_cast<chillAST_FloatingLiteral *>(n));
    case CHILLAST_NODE_LOOP:
//    case CHILLAST_NODE_FORSTMT:
      return getPrecS(dynamic_cast<chillAST_ForStmt *>(n));
    case CHILLAST_NODE_FREE:
      return getPrecS(dynamic_cast<chillAST_Free *>(n));
    case CHILLAST_NODE_FUNCTIONDECL:
      return getPrecS(dynamic_cast<chillAST_FunctionDecl *>(n));
    case CHILLAST_NODE_IFSTMT:
      return getPrecS(dynamic_cast<chillAST_IfStmt *>(n));
    case CHILLAST_NODE_IMPLICITCASTEXPR:
      return getPrecS(dynamic_cast<chillAST_ImplicitCastExpr *>(n));
    case CHILLAST_NODE_INTEGERLITERAL:
      return getPrecS(dynamic_cast<chillAST_IntegerLiteral *>(n));
    case CHILLAST_NODE_MACRODEFINITION:
      return getPrecS(dynamic_cast<chillAST_MacroDefinition *>(n));
    case CHILLAST_NODE_MALLOC:
      return getPrecS(dynamic_cast<chillAST_Malloc *>(n));
    case CHILLAST_NODE_MEMBEREXPR:
      return getPrecS(dynamic_cast<chillAST_MemberExpr *>(n));
    case CHILLAST_NODE_NOOP:
      return getPrecS(dynamic_cast<chillAST_NoOp *>(n));
    case CHILLAST_NODE_NULL:
      return getPrecS(dynamic_cast<chillAST_NULL *>(n));
    case CHILLAST_NODE_PARENEXPR:
      return getPrecS(dynamic_cast<chillAST_ParenExpr *>(n));
    case CHILLAST_NODE_PREPROCESSING:
      return getPrecS(dynamic_cast<chillAST_Preprocessing *>(n));
    case CHILLAST_NODE_RECORDDECL:
      return getPrecS(dynamic_cast<chillAST_RecordDecl *>(n));
    case CHILLAST_NODE_RETURNSTMT:
      return getPrecS(dynamic_cast<chillAST_ReturnStmt *>(n));
    case CHILLAST_NODE_SIZEOF:
      return getPrecS(dynamic_cast<chillAST_Sizeof *>(n));
    case CHILLAST_NODE_TRANSLATIONUNIT:
//    case CHILLAST_NODE_SOURCEFILE:
      return getPrecS(dynamic_cast<chillAST_SourceFile *>(n));
    case CHILLAST_NODE_TERNARYOPERATOR:
      return getPrecS(dynamic_cast<chillAST_TernaryOperator *>(n));
    case CHILLAST_NODE_TYPEDEFDECL:
      return getPrecS(dynamic_cast<chillAST_TypedefDecl *>(n));
    case CHILLAST_NODE_UNARYOPERATOR:
      return getPrecS(dynamic_cast<chillAST_UnaryOperator *>(n));
    case CHILLAST_NODE_VARDECL:
      return getPrecS(dynamic_cast<chillAST_VarDecl *>(n));
    case CHILLAST_NODE_UNKNOWN:
    default:
      CHILL_ERROR("Getting precedence an unknown type of Node: %s\n", n->getTypeString());
      return INT8_MAX;
  }
}

