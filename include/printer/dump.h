//
// Created by ztuowen on 9/24/16.
//

#ifndef CHILL_DUMP_H
#define CHILL_DUMP_H

#include "printer/generic.h"

/*!
 * \file
 * \brief this replace the old dump function in the chillAST
 */

namespace chill {
  namespace printer {
    class Dump : public GenericPrinter {
    public:
      Dump() {}
      virtual void printS(std::string ident, chillAST_ArraySubscriptExpr *n, std::ostream &o);
      virtual void printS(std::string ident, chillAST_BinaryOperator *n, std::ostream &o);
      virtual void printS(std::string ident, chillAST_CallExpr *n, std::ostream &o);
      virtual void printS(std::string ident, chillAST_CompoundStmt *n, std::ostream &o);
      virtual void printS(std::string ident, chillAST_CStyleAddressOf *n, std::ostream &o);
      virtual void printS(std::string ident, chillAST_CStyleCastExpr *n, std::ostream &o);
      virtual void printS(std::string ident, chillAST_CudaFree *n, std::ostream &o);
      virtual void printS(std::string ident, chillAST_CudaKernelCall *n, std::ostream &o);
      virtual void printS(std::string ident, chillAST_CudaMalloc *n, std::ostream &o);
      virtual void printS(std::string ident, chillAST_CudaMemcpy *n, std::ostream &o);
      virtual void printS(std::string ident, chillAST_CudaSyncthreads *n, std::ostream &o);
      virtual void printS(std::string ident, chillAST_DeclRefExpr *n, std::ostream &o);
      virtual void printS(std::string ident, chillAST_FloatingLiteral *n, std::ostream &o);
      virtual void printS(std::string ident, chillAST_ForStmt *n, std::ostream &o);
      virtual void printS(std::string ident, chillAST_Free *n, std::ostream &o);
      virtual void printS(std::string ident, chillAST_FunctionDecl *n, std::ostream &o);
      virtual void printS(std::string ident, chillAST_IfStmt *n, std::ostream &o);
      virtual void printS(std::string ident, chillAST_IntegerLiteral *n, std::ostream &o);
      virtual void printS(std::string ident, chillAST_ImplicitCastExpr *n, std::ostream &o);
      virtual void printS(std::string ident, chillAST_MacroDefinition *n, std::ostream &o);
      virtual void printS(std::string ident, chillAST_Malloc *n, std::ostream &o);
      virtual void printS(std::string ident, chillAST_MemberExpr *n, std::ostream &o);
      virtual void printS(std::string ident, chillAST_NULL *n, std::ostream &o);
      virtual void printS(std::string ident, chillAST_NoOp *n, std::ostream &o);
      virtual void printS(std::string ident, chillAST_ParenExpr *n, std::ostream &o);
      virtual void printS(std::string ident, chillAST_Preprocessing *n, std::ostream &o);
      virtual void printS(std::string ident, chillAST_RecordDecl *n, std::ostream &o);
      virtual void printS(std::string ident, chillAST_ReturnStmt *n, std::ostream &o);
      virtual void printS(std::string ident, chillAST_Sizeof *n, std::ostream &o);
      virtual void printS(std::string ident, chillAST_SourceFile *n, std::ostream &o);
      virtual void printS(std::string ident, chillAST_TypedefDecl *n, std::ostream &o);
      virtual void printS(std::string ident, chillAST_TernaryOperator *n, std::ostream &o);
      virtual void printS(std::string ident, chillAST_UnaryOperator *n, std::ostream &o);
      virtual void printS(std::string ident, chillAST_VarDecl *n, std::ostream &o);
      /*!
       * Just prints everything. Indent is igored due to need to limit the number of output
       * @param ident
       * @param n
       * @param o
       */
      virtual void print(std::string ident, chillAST_Node *n, std::ostream &o);
    };
  }
}

#endif //CHILL_DUMP_H
