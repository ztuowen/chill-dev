//
// Created by ztuowen on 9/24/16.
//

#ifndef CHILL_CFAMILY_H
#define CHILL_CFAMILY_H

#include "printer.h"

/*!
 * \file
 * \brief Print the AST for C like syntax, This replace the old print function
 */

namespace chill {
  namespace printer{
    class CFamily : public GenericPrinter {
    public:
      CFamily() {}
      virtual int getPrec(chillAST_Node *n);
      virtual void print(std::string ident, chillAST_ArraySubscriptExpr *n, std::ostringstream &o);
      virtual void print(std::string ident, chillAST_BinaryOperator *n, std::ostringstream &o);
      virtual void print(std::string ident, chillAST_CallExpr *n, std::ostringstream &o);
      virtual void print(std::string ident, chillAST_CompoundStmt *n, std::ostringstream &o);
      virtual void print(std::string ident, chillAST_CStyleAddressOf *n, std::ostringstream &o);
      virtual void print(std::string ident, chillAST_CStyleCastExpr *n, std::ostringstream &o);
      virtual void print(std::string ident, chillAST_CudaFree *n, std::ostringstream &o);
      virtual void print(std::string ident, chillAST_CudaKernelCall *n, std::ostringstream &o);
      virtual void print(std::string ident, chillAST_CudaMalloc *n, std::ostringstream &o);
      virtual void print(std::string ident, chillAST_CudaMemcpy *n, std::ostringstream &o);
      virtual void print(std::string ident, chillAST_CudaSyncthreads *n, std::ostringstream &o);
      virtual void print(std::string ident, chillAST_DeclRefExpr *n, std::ostringstream &o);
      virtual void print(std::string ident, chillAST_FloatingLiteral *n, std::ostringstream &o);
      virtual void print(std::string ident, chillAST_ForStmt *n, std::ostringstream &o);
      virtual void print(std::string ident, chillAST_Free *n, std::ostringstream &o);
      virtual void print(std::string ident, chillAST_FunctionDecl *n, std::ostringstream &o);
      virtual void print(std::string ident, chillAST_ImplicitCastExpr *n, std::ostringstream &o);
      virtual void print(std::string ident, chillAST_MacroDefinition *n, std::ostringstream &o);
      virtual void print(std::string ident, chillAST_Malloc *n, std::ostringstream &o);
      virtual void print(std::string ident, chillAST_MemberExpr *n, std::ostringstream &o);
      virtual void print(std::string ident, chillAST_ParenExpr *n, std::ostringstream &o);
      virtual void print(std::string ident, chillAST_Preprocessing *n, std::ostringstream &o);
      virtual void print(std::string ident, chillAST_RecordDecl *n, std::ostringstream &o);
      virtual void print(std::string ident, chillAST_ReturnStmt *n, std::ostringstream &o);
      virtual void print(std::string ident, chillAST_Sizeof *n, std::ostringstream &o);
      virtual void print(std::string ident, chillAST_SourceFile *n, std::ostringstream &o);
      virtual void print(std::string ident, chillAST_TypedefDecl *n, std::ostringstream &o);
      virtual void print(std::string ident, chillAST_TernaryOperator *n, std::ostringstream &o);
      virtual void print(std::string ident, chillAST_UnaryOperator *n, std::ostringstream &o);
      virtual void print(std::string ident, chillAST_VarDecl *n, std::ostringstream &o);
      virtual void print(string ident, chillAST_Node *n, ostringStream &o);
    };
  }
}

#endif //CHILL_CFAMILY_H
