//
// Created by ztuowen on 9/24/16.
//

#ifndef CHILL_PRINTER_H_H
#define CHILL_PRINTER_H_H

#include "chillAST.h"
#include "chillAST/chillAST_node.hh"
#include <string>
#include <sstream>

/*!
 * \file
 * \brief this is a generic AST printer that prints the code out to a C-family like syntax
 */
namespace chill {
  namespace printer {
    class GenericPrinter {
    private:
      std::string indentSpace;
    public:
      GenericPrinter() { indentSpace = "  "; }

      void setIndentSpace(int numspaces) {
        indentSpace = "";
        for (int i = 0; i < numspaces; ++i)
          indentSpace += "  ";
      }
      //! return the Precedence of the corresponding AST node
      /*!
       * @param n the chillAST_Node
       * @return a int representing the subnodes's precedence, 0 being the highest, INT8_MAX being the default
       */
      virtual int getPrec(chillAST_Node *n) { return INT8_MAX; }
      virtual void print(std::string ident, chillAST_ArraySubscriptExpr *n, std::ostringstream &o)=0;
      virtual void print(std::string ident, chillAST_BinaryOperator *n, std::ostringstream &o)=0;
      virtual void print(std::string ident, chillAST_CallExpr *n, std::ostringstream &o)=0;
      virtual void print(std::string ident, chillAST_CompoundStmt *n, std::ostringstream &o)=0;
      virtual void print(std::string ident, chillAST_CStyleAddressOf *n, std::ostringstream &o)=0;
      virtual void print(std::string ident, chillAST_CStyleCastExpr *n, std::ostringstream &o)=0;
      virtual void print(std::string ident, chillAST_CudaFree *n, std::ostringstream &o)=0;
      virtual void print(std::string ident, chillAST_CudaKernelCall *n, std::ostringstream &o)=0;
      virtual void print(std::string ident, chillAST_CudaMalloc *n, std::ostringstream &o)=0;
      virtual void print(std::string ident, chillAST_CudaMemcpy *n, std::ostringstream &o)=0;
      virtual void print(std::string ident, chillAST_CudaSyncthreads *n, std::ostringstream &o)=0;
      virtual void print(std::string ident, chillAST_DeclRefExpr *n, std::ostringstream &o)=0;
      virtual void print(std::string ident, chillAST_FloatingLiteral *n, std::ostringstream &o)=0;
      virtual void print(std::string ident, chillAST_ForStmt *n, std::ostringstream &o)=0;
      virtual void print(std::string ident, chillAST_Free *n, std::ostringstream &o)=0;
      virtual void print(std::string ident, chillAST_FunctionDecl *n, std::ostringstream &o)=0;
      virtual void print(std::string ident, chillAST_IfStmt *n, std::ostringstream &o)=0;
      virtual void print(std::string ident, chillAST_IntegerLiteral *n, std::ostringstream &o)=0;
      virtual void print(std::string ident, chillAST_ImplicitCastExpr *n, std::ostringstream &o)=0;
      virtual void print(std::string ident, chillAST_MacroDefinition *n, std::ostringstream &o)=0;
      virtual void print(std::string ident, chillAST_Malloc *n, std::ostringstream &o)=0;
      virtual void print(std::string ident, chillAST_MemberExpr *n, std::ostringstream &o)=0;
      virtual void print(std::string ident, chillAST_NULL *n, std::ostringstream &o)=0;
      virtual void print(std::string ident, chillAST_NoOp *n, std::ostringstream &o)=0;
      virtual void print(std::string ident, chillAST_ParenExpr *n, std::ostringstream &o)=0;
      virtual void print(std::string ident, chillAST_Preprocessing *n, std::ostringstream &o)=0;
      virtual void print(std::string ident, chillAST_RecordDecl *n, std::ostringstream &o)=0;
      virtual void print(std::string ident, chillAST_ReturnStmt *n, std::ostringstream &o)=0;
      virtual void print(std::string ident, chillAST_Sizeof *n, std::ostringstream &o)=0;
      virtual void print(std::string ident, chillAST_SourceFile *n, std::ostringstream &o)=0;
      virtual void print(std::string ident, chillAST_TypedefDecl *n, std::ostringstream &o)=0;
      virtual void print(std::string ident, chillAST_TernaryOperator *n, std::ostringstream &o)=0;
      virtual void print(std::string ident, chillAST_UnaryOperator *n, std::ostringstream &o)=0;
      virtual void print(std::string ident, chillAST_VarDecl *n, std::ostringstream &o)=0;
      //! Print the AST to string stream
      /*!
       * @param ident indentation of the node
       * @param n the chillAST_Node
       * @param o the string stream
       */
      virtual void print(std::string ident, chillAST_Node *n, std::ostringstream &o){
        switch (n->getType()) {
          case CHILLAST_NODE_ARRAYSUBSCRIPTEXPR:
            print(ident, dynamic_cast<chillAST_ArraySubscriptExpr*>(n),o);
            break;
          case CHILLAST_NODE_BINARYOPERATOR:
            print(ident, dynamic_cast<chillAST_BinaryOperator*>(n),o);
            break;
          case CHILLAST_NODE_CALLEXPR:
            print(ident, dynamic_cast<chillAST_CallExpr*>(n),o);
            break;
          case CHILLAST_NODE_COMPOUNDSTMT:
            print(ident, dynamic_cast<chillAST_CompoundStmt*>(n),o);
            break;
          case CHILLAST_NODE_CSTYLEADDRESSOF:
            print(ident, dynamic_cast<chillAST_CStyleAddressOf*>(n),o);
            break;
          case CHILLAST_NODE_CSTYLECASTEXPR:
            print(ident, dynamic_cast<chillAST_CStyleCastExpr*>(n),o);
            break;
          case CHILLAST_NODE_CUDAFREE:
            print(ident, dynamic_cast<chillAST_CudaFree*>(n),o);
            break;
          case CHILLAST_NODE_CUDAKERNELCALL:
            print(ident, dynamic_cast<chillAST_CudaKernelCall*>(n),o);
            break;
          case CHILLAST_NODE_CUDAMALLOC:
            print(ident, dynamic_cast<chillAST_CudaMalloc*>(n),o);
            break;
          case CHILLAST_NODE_CUDAMEMCPY:
            print(ident, dynamic_cast<chillAST_CudaMemcpy*>(n),o);
            break;
          case CHILLAST_NODE_CUDASYNCTHREADS:
            print(ident, dynamic_cast<chillAST_CudaSyncthreads*>(n),o);
            break;
          case CHILLAST_NODE_DECLREFEXPR:
            print(ident, dynamic_cast<chillAST_DeclRefExpr*>(n),o);
            break;
          case CHILLAST_NODE_FLOATINGLITERAL:
            print(ident, dynamic_cast<chillAST_FloatingLiteral*>(n),o);
            break;
          case CHILLAST_NODE_LOOP:
          case CHILLAST_NODE_FORSTMT:
            print(ident, dynamic_cast<chillAST_ForStmt*>(n),o);
            break;
          case CHILLAST_NODE_FREE:
            print(ident, dynamic_cast<chillAST_Free*>(n),o);
            break;
          case CHILLAST_NODE_FUNCTIONDECL:
            print(ident, dynamic_cast<chillAST_FunctionDecl*>(n),o);
            break;
          case CHILLAST_NODE_IFSTMT:
            print(ident, dynamic_cast<chillAST_IfStmt*>(n),o);
            break;
          case CHILLAST_NODE_IMPLICITCASTEXPR:
            print(ident, dynamic_cast<chillAST_ImplicitCastExpr*>(n),o);
            break;
          case CHILLAST_NODE_INTEGERLITERAL:
            print(ident, dynamic_cast<chillAST_IntegerLiteral*>(n),o);
            break;
          case CHILLAST_NODE_MACRODEFINITION:
            print(ident, dynamic_cast<chillAST_MacroDefinition*>(n),o);
            break;
          case CHILLAST_NODE_MALLOC:
            print(ident, dynamic_cast<chillAST_Malloc*>(n),o);
            break;
          case CHILLAST_NODE_MEMBEREXPR:
            print(ident, dynamic_cast<chillAST_MemberExpr*>(n),o);
            break;
          case CHILLAST_NODE_NOOP:
            print(ident, dynamic_cast<chillAST_NoOp*>(n),o);
            break;
          case CHILLAST_NODE_NULL:
            print(ident, dynamic_cast<chillAST_NULL*>(n),o);
            break;
          case CHILLAST_NODE_PARENEXPR:
            print(ident, dynamic_cast<chillAST_ParenExpr*>(n),o);
            break;
          case CHILLAST_NODE_PREPROCESSING:
            print(ident, dynamic_cast<chillAST_Preprocessing*>(n),o);
            break;
          case CHILLAST_NODE_RECORDDECL:
            print(ident, dynamic_cast<chillAST_RecordDecl*>(n),o);
            break;
          case CHILLAST_NODE_RETURNSTMT:
            print(ident, dynamic_cast<chillAST_ReturnStmt*>(n),o);
            break;
          case CHILLAST_NODE_SIZEOF:
            print(ident, dynamic_cast<chillAST_Sizeof*>(n),o);
            break;
          case CHILLAST_NODE_TRANSLATIONUNIT:
          case CHILLAST_NODE_SOURCEFILE:
            print(ident, dynamic_cast<chillAST_SourceFile*>(n),o);
            break;
          case CHILLAST_NODE_TERNARYOPERATOR:
            print(ident, dynamic_cast<chillAST_TernaryOperator*>(n),o);
            break;
          case CHILLAST_NODE_TYPEDEFDECL:
            print(ident, dynamic_cast<chillAST_TypedefDecl*>(n),o);
            break;
          case CHILLAST_NODE_UNARYOPERATOR:
            print(ident, dynamic_cast<chillAST_UnaryOperator*>(n),o);
            break;
          case CHILLAST_NODE_VARDECL:
            print(ident, dynamic_cast<chillAST_VarDecl*>(n),o);
            break;
          case CHILLAST_NODE_UNKNOWN:
          default:
            CHILL_ERROR("Printing an unknown type of Node: %s\n", n->getTypeString());
        }
      }
      //! Print the AST to string, overload the print function
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
   };
  }
}

#endif //CHILL_PRINTER_H_H
