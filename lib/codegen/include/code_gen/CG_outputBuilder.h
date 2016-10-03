/*****************************************************************************
 Copyright (C) 1994-2000 the Omega Project Team
 Copyright (C) 2005-2011 Chun Chen
 All Rights Reserved.

 Purpose:
   abstract base class of compiler IR code builder

 Notes:
   All "CG_outputRepr *" parameters are consumed inside the the function
 unless explicitly stated otherwise, i.e., not valid after the call.
   Parameter "indent" normally not used except it is used in unstructured
 string output for correct indentation.
 
 History:
   04/17/96 created - Lei Zhou
   05/02/08 clarify integer floor/mod/ceil definitions, -chen
   05/31/08 use virtual clone to implement CreateCopy, -chun
   08/05/10 clarify NULL parameter allowance, -chun
*****************************************************************************/

#ifndef _CG_OUTPUTBUILDER_H
#define _CG_OUTPUTBUILDER_H

#include <code_gen/CG_outputRepr.h>

#include <string>
#include <vector>

namespace omega {

  //! abstract base class of compiler IR code builder
  class CG_outputBuilder {
  private:

  public:

    CG_outputBuilder() {}

    virtual ~CG_outputBuilder() {}

    virtual CG_outputRepr *CreateStruct(const std::string class_name,
                                        std::vector<std::string> class_data_members,  // why not just vector< CG_outputRepr> subparts;
                                        std::vector<CG_outputRepr *> class_data_types)=0;

    virtual CG_outputRepr *CreateClassInstance(std::string name, CG_outputRepr *class_def)=0;

    virtual CG_outputRepr *lookup_member_data(CG_outputRepr *scope, std::string varName, CG_outputRepr *instance)=0;

    virtual CG_outputRepr *CreatePointer(std::string &name) const = 0;

    virtual CG_outputRepr *ObtainInspectorRange(const std::string &_s, const std::string &_name) const =0;

    /*!
     * \brief substitute variable in stmt
     *
     * @param indent
     * @param stmt
     * @param vars
     * @param subs
     * @param actuallyPrint
     * @return
     */
    virtual CG_outputRepr *CreateSubstitutedStmt(int indent,
                                                 CG_outputRepr *stmt,
                                                 const std::vector<std::string> &vars,
                                                 std::vector<CG_outputRepr *> &subs,
                                                 bool actuallyPrint = true) const = 0;

    /*!
     * @brief assignment stmt generation
     * @param indent
     * @param lhs
     * @param rhs
     * @return
     */
    virtual CG_outputRepr *CreateAssignment(int indent,
                                            CG_outputRepr *lhs,
                                            CG_outputRepr *rhs) const = 0;
    /*!
     * @brief Plus assignment stmt generation
     * @param indent
     * @param lhs
     * @param rhs
     * @return CG_outputRepr
     */
    virtual CG_outputRepr *CreatePlusAssignment(int indent,
                                                CG_outputRepr *lhs,
                                                CG_outputRepr *rhs) const = 0;
    /*!
     * @brief function invoation generation
     * @param funcName
     * @param argList
     * @param is_array
     * @return
     */
    virtual CG_outputRepr *CreateInvoke(const std::string &funcName,
                                        std::vector<CG_outputRepr *> &argList,
                                        bool is_array = false) const = 0;
    /*!
     * @brief comment generation
     * @param indent
     * @param commentText
     * @return
     */
    virtual CG_outputRepr *CreateComment(int indent,
                                         const std::string &commentText) const = 0;
    /*!
     * @brief Attribute Generation
     * @param control
     * @param commentText
     * @return
     */
    virtual CG_outputRepr *CreateAttribute(CG_outputRepr *control,
                                           const std::string &commentText) const = 0;
    /*!
     * @brief pragma attribute
     * @param scopeStmt
     * @param looplevel
     * @param pragmaText
     * @return
     */
    virtual CG_outputRepr *
    CreatePragmaAttribute(CG_outputRepr *scopeStmt, int looplevel, const std::string &pragmaText) const = 0;
    /*!
     * @brief Prefetch attribute
     * @param scopeStmt
     * @param looplevel
     * @param arrName
     * @param hint
     * @return
     */
    virtual CG_outputRepr *
    CreatePrefetchAttribute(CG_outputRepr *scopeStmt, int looplevel, const std::string &arrName, int hint) const = 0;
    /*!
     * @brief generate if stmt, true/false stmt can be NULL but not the condition
     * @param indent
     * @param guardCondition
     * @param true_stmtList
     * @param false_stmtList
     * @return
     */
    virtual CG_outputRepr *CreateIf(int indent, CG_outputRepr *guardCondition,
                                    CG_outputRepr *true_stmtList,
                                    CG_outputRepr *false_stmtList) const = 0;
    /*!
     * @brief generate loop inductive variable (loop control structure)
     * @param index
     * @param lower
     * @param upper
     * @param step
     * @return
     */
    virtual CG_outputRepr *CreateInductive(CG_outputRepr *index,
                                           CG_outputRepr *lower,
                                           CG_outputRepr *upper,
                                           CG_outputRepr *step) const = 0;
    /*!
     * @brief generate loop stmt from loop control and loop body, NULL parameter allowed
     * @param indent
     * @param control
     * @param stmtList
     * @return
     */
    virtual CG_outputRepr *CreateLoop(int indent, CG_outputRepr *control,
                                      CG_outputRepr *stmtList) const = 0;
    /*!
     * @brief copy operation, NULL parameter allowed
     *
     * this function makes pointer handling uniform regardless NULL status
     *
     * @param original
     * @return
     */
    virtual CG_outputRepr *CreateCopy(CG_outputRepr *original) const {
      if (original == NULL)
        return NULL;
      else
        return original->clone();
    }

    //! basic interger number creation
    virtual CG_outputRepr *CreateInt(int num) const = 0;

    //! basic single precision float number creation
    virtual CG_outputRepr *CreateFloat(float num) const = 0;

    //! basic double precision float number creation
    virtual CG_outputRepr *CreateDouble(double num) const = 0;

    /*!
     * @brief check if op is integer literal
     * @param op
     * @return
     */
    virtual bool isInteger(CG_outputRepr *op) const = 0;

    /*!
     * @brief DOC needed
     * @param varName
     * @return
     */
    virtual bool QueryInspectorType(const std::string &varName) const = 0;
    /*!
     * @brief basic identity/variable creation
     * @param varName
     * @return
     */
    virtual CG_outputRepr *CreateIdent(const std::string &varName) const = 0;
    /*!
     * @brief member access creation?
     * @param lop
     * @param rop
     * @return
     */
    virtual CG_outputRepr *CreateDotExpression(CG_outputRepr *lop,
                                               CG_outputRepr *rop) const =0;
    /*!
     * @brief Create array reference expression starting with a string
     * @param _s
     * @param rop
     * @return
     */
    virtual CG_outputRepr *CreateArrayRefExpression(const std::string &_s,
                                                    CG_outputRepr *rop) const =0;
    /*!
     * @brief Create array reference expression starting with a expresion
     * @param lop
     * @param rop
     * @return
     */
    virtual CG_outputRepr *CreateArrayRefExpression(CG_outputRepr *lop,
                                                    CG_outputRepr *rop) const =0;
    /*!
     * @brief TODO DOC NEEDED
     * @param _s
     * @param member_name
     * @return
     */
    virtual CG_outputRepr *ObtainInspectorData(const std::string &_s, const std::string &member_name) const =0;
    /*!
     * @brief TODO USAGE NEEDED
     * @return
     */
    virtual CG_outputRepr *CreateNullStatement() const =0;
    /*!
     * @brief Addition operations, NULL parameter means 0
     * @param lop
     * @param rop
     * @return
     */
    virtual CG_outputRepr *CreatePlus(CG_outputRepr *lop, CG_outputRepr *rop) const = 0;
    /*!
     * @brief Subtraction operations, NULL parameter means 0
     * @param lop
     * @param rop
     * @return
     */
    virtual CG_outputRepr *CreateMinus(CG_outputRepr *lop, CG_outputRepr *rop) const = 0;
    /*!
     * @brief Multiplication operations, NULL parameter means 0
     * @param lop
     * @param rop
     * @return
     */
    virtual CG_outputRepr *CreateTimes(CG_outputRepr *lop, CG_outputRepr *rop) const = 0;
    /*!
     * @brief Division operations, NULL parameter means 0
     *
     * integer division truncation method undefined, only use when lop is known
     * to be multiple of rop, otherwise use integer floor instead
     *
     * @param lop
     * @param rop
     * @return
     */
    virtual CG_outputRepr *CreateDivide(CG_outputRepr *lop, CG_outputRepr *rop) const {
      return CreateIntegerFloor(lop, rop);
    }
    /*!
     * @brief integer floor functions, NULL parameter means 0
     *
     * second parameter must be postive (i.e. b > 0 below), otherwise function undefined
     *
     * floor(a, b)
     * * = a/b if a >= 0
     * * = (a-b+1)/b if a < 0
     *
     * @param lop
     * @param rop
     * @return
     */
    virtual CG_outputRepr *CreateIntegerFloor(CG_outputRepr *lop, CG_outputRepr *rop) const = 0;
    /*!
     * @brief integer mod functions, NULL parameter means 0
     *
     * second parameter must be postive (i.e. b > 0 below), otherwise function undefined
     *
     * mod(a, b) = a-b*floor(a, b) where result must lie in range [0,b)
     *
     * @param lop
     * @param rop
     * @return
     */
    virtual CG_outputRepr *CreateIntegerMod(CG_outputRepr *lop, CG_outputRepr *rop) const {
      CG_outputRepr *lop2 = CreateCopy(lop);
      CG_outputRepr *rop2 = CreateCopy(rop);
      return CreateMinus(lop2, CreateTimes(rop2, CreateIntegerFloor(lop, rop)));
    }
    /*!
     * @brief integer ceil functions, NULL parameter means 0
     *
     * second parameter must be postive (i.e. b > 0 below), otherwise function undefined
     *
     * ceil(a, b) = -floor(-a, b) or floor(a+b-1, b) or floor(a-1, b)+1
     *
     * @param lop
     * @param rop
     * @return
     */
    virtual CG_outputRepr *CreateIntegerCeil(CG_outputRepr *lop, CG_outputRepr *rop) const {
      return CreateMinus(NULL, CreateIntegerFloor(CreateMinus(NULL, lop), rop));
    }

    //! binary logical operation, NULL parameter means TRUE
    virtual CG_outputRepr *CreateAnd(CG_outputRepr *lop, CG_outputRepr *rop) const = 0;

    //! binary conditional greater than or equal to
    virtual CG_outputRepr *CreateGE(CG_outputRepr *lop, CG_outputRepr *rop) const {
      return CreateLE(rop, lop);
    }

    //! binary conditional less than or equal to
    virtual CG_outputRepr *CreateLE(CG_outputRepr *lop, CG_outputRepr *rop) const = 0;

    //! binary conditional equal to
    virtual CG_outputRepr *CreateEQ(CG_outputRepr *lop, CG_outputRepr *rop) const = 0;

    //! binary conditional not equal to
    virtual CG_outputRepr *CreateNEQ(CG_outputRepr *lop, CG_outputRepr *rop) const = 0;
    //! address of operator
    virtual CG_outputRepr *CreateAddressOf(CG_outputRepr *op) const = 0;
    //! create a break statement
    virtual CG_outputRepr *CreateBreakStatement(void) const = 0;

    //! join stmts together, NULL parameter allowed
    virtual CG_outputRepr *StmtListAppend(CG_outputRepr *list1, CG_outputRepr *list2) const = 0;
    //! Wraps a expression into a statement. TODO DOC NEEDED
    virtual CG_outputRepr *CreateStatementFromExpression(CG_outputRepr *exp) const = 0;
    //! TODO USAGE AND DOC
    virtual const char *ClassName() { return "UNKNOWN"; }
  };

}

#endif
