//
// Created by ztuowen on 9/24/16.
//

#ifndef CHILL_PRINTER_H_H
#define CHILL_PRINTER_H_H

#include "chillAST.h"
#include <string>
#include <sstream>

/*!
 * \file
 * \brief this is a generic AST printSer that prints the code out to a C-family like syntax
 */
namespace chill {
  namespace printer {
    class GenericPrinter {
    private:
      std::string identSpace;
    public:
      GenericPrinter() { identSpace = "  "; }
      //! Set the indentation for print
      /*!
       * Some subclass has indentation unused, like Dump. Also, only spaces is supported,
       * so it is a number of the spaces in the indentaion.
       * @param numspaces number of spaces for the indentation
       */
      void setIndentSpace(int numspaces) {
        identSpace = "";
        for (int i = 0; i < numspaces; ++i)
          identSpace += "  ";
      }
      virtual int getPrecS(chillAST_ArraySubscriptExpr *n) { return INT8_MAX; }
      virtual int getPrecS(chillAST_BinaryOperator *n) { return INT8_MAX; }
      virtual int getPrecS(chillAST_CallExpr *n) { return INT8_MAX; }
      virtual int getPrecS(chillAST_CompoundStmt *n) { return INT8_MAX; }
      virtual int getPrecS(chillAST_CStyleAddressOf *n) { return INT8_MAX; }
      virtual int getPrecS(chillAST_CStyleCastExpr *n) { return INT8_MAX; }
      virtual int getPrecS(chillAST_CudaFree *n) { return INT8_MAX; }
      virtual int getPrecS(chillAST_CudaKernelCall *n) { return INT8_MAX; }
      virtual int getPrecS(chillAST_CudaMalloc *n) { return INT8_MAX; }
      virtual int getPrecS(chillAST_CudaMemcpy *n) { return INT8_MAX; }
      virtual int getPrecS(chillAST_CudaSyncthreads *n) { return INT8_MAX; }
      virtual int getPrecS(chillAST_DeclRefExpr *n) { return INT8_MAX; }
      virtual int getPrecS(chillAST_FloatingLiteral *n) { return INT8_MAX; }
      virtual int getPrecS(chillAST_ForStmt *n) { return INT8_MAX; }
      virtual int getPrecS(chillAST_Free *n) { return INT8_MAX; }
      virtual int getPrecS(chillAST_FunctionDecl *n) { return INT8_MAX; }
      virtual int getPrecS(chillAST_IfStmt *n) { return INT8_MAX; }
      virtual int getPrecS(chillAST_IntegerLiteral *n) { return INT8_MAX; }
      virtual int getPrecS(chillAST_ImplicitCastExpr *n) { return INT8_MAX; }
      virtual int getPrecS(chillAST_MacroDefinition *n) { return INT8_MAX; }
      virtual int getPrecS(chillAST_Malloc *n) { return INT8_MAX; }
      virtual int getPrecS(chillAST_MemberExpr *n) { return INT8_MAX; }
      virtual int getPrecS(chillAST_NULL *n) { return INT8_MAX; }
      virtual int getPrecS(chillAST_NoOp *n) { return INT8_MAX; }
      virtual int getPrecS(chillAST_ParenExpr *n) { return INT8_MAX; }
      virtual int getPrecS(chillAST_Preprocessing *n) { return INT8_MAX; }
      virtual int getPrecS(chillAST_RecordDecl *n) { return INT8_MAX; }
      virtual int getPrecS(chillAST_ReturnStmt *n) { return INT8_MAX; }
      virtual int getPrecS(chillAST_Sizeof *n) { return INT8_MAX; }
      virtual int getPrecS(chillAST_SourceFile *n) { return INT8_MAX; }
      virtual int getPrecS(chillAST_TypedefDecl *n) { return INT8_MAX; }
      virtual int getPrecS(chillAST_TernaryOperator *n) { return INT8_MAX; }
      virtual int getPrecS(chillAST_UnaryOperator *n) { return INT8_MAX; }
      virtual int getPrecS(chillAST_VarDecl *n) { return INT8_MAX; }
      //! return the Precedence of the corresponding AST node
      /*!
       * @param n the chillAST_Node
       * @return a int representing the subnodes's precedence, 0 being the highest, INT8_MAX being the default
       */
      virtual int getPrec(chillAST_Node *n);
      virtual void printS(std::string ident, chillAST_ArraySubscriptExpr *n, std::ostream &o)=0;
      virtual void printS(std::string ident, chillAST_BinaryOperator *n, std::ostream &o)=0;
      virtual void printS(std::string ident, chillAST_CallExpr *n, std::ostream &o)=0;
      virtual void printS(std::string ident, chillAST_CompoundStmt *n, std::ostream &o)=0;
      virtual void printS(std::string ident, chillAST_CStyleAddressOf *n, std::ostream &o)=0;
      virtual void printS(std::string ident, chillAST_CStyleCastExpr *n, std::ostream &o)=0;
      virtual void printS(std::string ident, chillAST_CudaFree *n, std::ostream &o)=0;
      virtual void printS(std::string ident, chillAST_CudaKernelCall *n, std::ostream &o)=0;
      virtual void printS(std::string ident, chillAST_CudaMalloc *n, std::ostream &o)=0;
      virtual void printS(std::string ident, chillAST_CudaMemcpy *n, std::ostream &o)=0;
      virtual void printS(std::string ident, chillAST_CudaSyncthreads *n, std::ostream &o)=0;
      virtual void printS(std::string ident, chillAST_DeclRefExpr *n, std::ostream &o)=0;
      virtual void printS(std::string ident, chillAST_FloatingLiteral *n, std::ostream &o)=0;
      virtual void printS(std::string ident, chillAST_ForStmt *n, std::ostream &o)=0;
      virtual void printS(std::string ident, chillAST_Free *n, std::ostream &o)=0;
      virtual void printS(std::string ident, chillAST_FunctionDecl *n, std::ostream &o)=0;
      virtual void printS(std::string ident, chillAST_IfStmt *n, std::ostream &o)=0;
      virtual void printS(std::string ident, chillAST_IntegerLiteral *n, std::ostream &o)=0;
      virtual void printS(std::string ident, chillAST_ImplicitCastExpr *n, std::ostream &o)=0;
      virtual void printS(std::string ident, chillAST_MacroDefinition *n, std::ostream &o)=0;
      virtual void printS(std::string ident, chillAST_Malloc *n, std::ostream &o)=0;
      virtual void printS(std::string ident, chillAST_MemberExpr *n, std::ostream &o)=0;
      virtual void printS(std::string ident, chillAST_NULL *n, std::ostream &o)=0;
      virtual void printS(std::string ident, chillAST_NoOp *n, std::ostream &o)=0;
      virtual void printS(std::string ident, chillAST_ParenExpr *n, std::ostream &o)=0;
      virtual void printS(std::string ident, chillAST_Preprocessing *n, std::ostream &o)=0;
      virtual void printS(std::string ident, chillAST_RecordDecl *n, std::ostream &o)=0;
      virtual void printS(std::string ident, chillAST_ReturnStmt *n, std::ostream &o)=0;
      virtual void printS(std::string ident, chillAST_Sizeof *n, std::ostream &o)=0;
      virtual void printS(std::string ident, chillAST_SourceFile *n, std::ostream &o)=0;
      virtual void printS(std::string ident, chillAST_TypedefDecl *n, std::ostream &o)=0;
      virtual void printS(std::string ident, chillAST_TernaryOperator *n, std::ostream &o)=0;
      virtual void printS(std::string ident, chillAST_UnaryOperator *n, std::ostream &o)=0;
      virtual void printS(std::string ident, chillAST_VarDecl *n, std::ostream &o)=0;
      //! Print the AST to string stream, multiplexer
      /*!
       * @param ident indentation of the node
       * @param n the chillAST_Node
       * @param o the string stream
       */
      virtual void print(std::string ident, chillAST_Node *n, std::ostream &o);
      //! Print the AST to string, overload the printS function
      /*!
       * @param ident indentation of the node
       * @param n the chillAST_Node
       * @return a string of the corresponding code
       */
      virtual std::string print(std::string ident, chillAST_Node *n) {
        std::ostringstream os;
        print(ident, n, os);
        return os.str();
      }
      //! Print the AST to stdout
      /*!
       * @param ident indentation of the node, the one inherited from the parent
       * @param n the chillAST_Node
       */
      virtual void printOut(std::string ident, chillAST_Node *n) {
        print(ident,n,std::cout);
      }
      //! Print the AST to stdErr
      /*!
       * @param ident indentation of the node
       * @param n the chillAST_Node
       */
      virtual void printErr(std::string ident, chillAST_Node *n) {
        print(ident,n,std::cerr);
      }
      //! Print the subexpression with precedence
      virtual void printPrec(std::string ident,chillAST_Node *n,std::ostream &o, int prec) {
        if (getPrec(n) > prec) o<<"(";
        print(ident,n,prec);
        if (getPrec(n) > prec) o<<")";
      }
   };
  }
}

#endif //CHILL_PRINTER_H_H
