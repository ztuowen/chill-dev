#ifndef OMEGATOOLS_HH
#define OMEGATOOLS_HH

/*!
 * \file
 * \brief Useful tools involving Omega manipulation.
 */

#include <string>
#include <omega.h>
#include "dep.hh"
#include "ir_code.hh"

std::string tmp_e();

//! Convert expression tree to omega relation.
/*!
 * \param destroy shallow deallocation of "repr", not freeing the actual code inside.
 */
void exp2formula(IR_Code *ir, omega::Relation &r, omega::F_And *f_root,
                 std::vector<omega::Free_Var_Decl *> &freevars,
                 omega::CG_outputRepr *repr, omega::Variable_ID lhs, char side,
                 IR_CONDITION_TYPE rel, bool destroy);

//! Build dependence relation for two array references.
omega::Relation arrays2relation(IR_Code *ir, std::vector<omega::Free_Var_Decl*> &freevars,
                                const IR_ArrayRef *ref_src, const omega::Relation &IS_w,
                                const IR_ArrayRef *ref_dst, const omega::Relation &IS_r);
//! Convert array dependence relation into set of dependence vectors
/*!
 * assuming ref_w is lexicographically before ref_r in the source code.
 */
std::pair<std::vector<DependenceVector>, std::vector<DependenceVector> > relation2dependences(
  const IR_ArrayRef *ref_src, const IR_ArrayRef *ref_dst, const omega::Relation &r);

//! Convert a boolean expression to omega relation.  
/*!
 * \param destroy shallow deallocation of "repr", not freeing the actual code inside.
 */
void exp2constraint(IR_Code *ir, omega::Relation &r, omega::F_And *f_root,
                    std::vector<omega::Free_Var_Decl *> &freevars,
                    omega::CG_outputRepr *repr, bool destroy);

bool is_single_iteration(const omega::Relation &r, int dim);
//!  Set/get the value of a variable which is know to be constant.
void assign_const(omega::Relation &r, int dim, int val);

int get_const(const omega::Relation &r, int dim, omega::Var_Kind type);

//! Find the position index variable in a Relation by name.
omega::Variable_ID find_index(omega::Relation &r, const std::string &s, char side);

//! Generate mapping relation for permuation.
omega::Relation permute_relation(const std::vector<int> &pi);

omega::Relation get_loop_bound(const omega::Relation &r, int dim);

//!  Determine whether the loop (starting from 0) in the iteration space has only one iteration.
bool is_single_loop_iteration(const omega::Relation &r, int level, const omega::Relation &known);
//! Get the bound for a specific loop.
omega::Relation get_loop_bound(const omega::Relation &r, int level, const omega::Relation &known);
omega::Relation get_max_loop_bound(const std::vector<omega::Relation> &r, int dim);
omega::Relation get_min_loop_bound(const std::vector<omega::Relation> &r, int dim);

//! Add strident to a loop.
/*!
 * Issues:
 *
 * * Don't work with relations with multiple disjuncts.
 * * Omega's dealing with max lower bound is awkward.
 */
void add_loop_stride(omega::Relation &r, const omega::Relation &bound, int dim, int stride);
bool is_inner_loop_depend_on_level(const omega::Relation &r, int level, const omega::Relation &known);
/*!
 * Suppose loop dim is i. Replace i with i+adjustment in loop bounds.
 *
 * ~~~
 * do i = 1, n
 *   do j = i, n
 * ~~~
 *  
 *  after call with dim = 0 and adjustment = 1:
 *
 * ~~~
 * do i = 1, n
 *   do j = i+1, n
 * ~~~
 */
omega::Relation adjust_loop_bound(const omega::Relation &r, int level, int adjustment);

enum LexicalOrderType {LEX_MATCH, LEX_BEFORE, LEX_AFTER, LEX_UNKNOWN};

#endif
