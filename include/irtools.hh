#ifndef IRTOOLS_HH
#define IRTOOLS_HH

/*!
 * \file
 * \brief Useful tools to analyze code in compiler IR format.
 */

#include <vector>
#include <omega.h>
#include <code_gen/CG_outputRepr.h>
#include "ir_code.hh"
#include "dep.hh"

//! It is used to initialize a loop.
struct ir_tree_node {
  IR_Control *content;
  ir_tree_node *parent;
  std::vector<ir_tree_node *> children;
/*! 
 * * For a loop node, payload is its mapped iteration space dimension. 
 * * For a simple block node, payload is its mapped statement number. 
 * * Normal if-else is splitted into two nodes
 *   * the one with odd payload represents then-part and
 *   * the one with even payload represents else-part.
 */
  int payload;
  
  ~ir_tree_node() {
    for (int i = 0; i < children.size(); i++)
      delete children[i];
    delete content;
  }
};

//! Build IR tree from the source code
/*!
 * Block type node can only be leaf, i.e., there is no further stuctures inside a block allowed
 * @param control
 * @param parent
 * @return
 */
std::vector<ir_tree_node *> build_ir_tree(IR_Control *control,
                                          ir_tree_node *parent = NULL);
//! Extract statements from IR tree
/*!
 * Statements returned are ordered in lexical order in the source code
 * @param ir_tree
 * @return
 */
std::vector<ir_tree_node *> extract_ir_stmts(
  const std::vector<ir_tree_node *> &ir_tree);
bool is_dependence_valid(ir_tree_node *src_node, ir_tree_node *dst_node,
                         const DependenceVector &dv, bool before);
//! test data dependeces between two statements
/*!
 * The first statement in parameter must be lexically before the second statement in parameter.
 * Returned dependences are all lexicographically positive
 * @param ir
 * @param repr1
 * @param IS1
 * @param repr2
 * @param IS2
 * @param freevar
 * @param index
 * @param i
 * @param j
 * @return Dependecies between the two statements. First vector is dependencies from first to second,
 * second vector is the reverse
 */
std::pair<std::vector<DependenceVector>, std::vector<DependenceVector> > test_data_dependences(
  IR_Code *ir, const omega::CG_outputRepr *repr1,
  const omega::Relation &IS1, const omega::CG_outputRepr *repr2,
  const omega::Relation &IS2, std::vector<omega::Free_Var_Decl*> &freevar,
  std::vector<std::string> index, int i, int j);

#endif
