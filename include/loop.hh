#ifndef LOOP_HH
#define LOOP_HH


#include <omega.h>
#include <code_gen/codegen.h>
#include <code_gen/CG.h>
#include <vector>
#include <map>
#include <set>
#include "dep.hh"
#include "ir_code.hh"
#include "irtools.hh"
#include <code_gen/CG_stringBuilder.h>

#include "stencil.hh"

/*!
 * \file
 * \brief Core loop transformation functionality.
 *
 * "level" (starting from 1) means loop level and it corresponds to "dim"
 * (starting from 0) in transformed iteration space [c_1,l_1,c_2,l_2,....,
 * c_n,l_n,c_(n+1)], e.g., l_2 is loop level 2 in generated code, dim 3
 * in transformed iteration space, and variable 4 in Omega relation.
 * All c's are constant numbers only and they will not show up as actual loops.
 *
 * Formula:
 *
 * ~~~
 *   dim = 2*level - 1
 *   var = dim + 1
 * ~~~
 */

class IR_Code;

enum TilingMethodType { StridedTile, CountedTile };
enum LoopLevelType { LoopLevelOriginal, LoopLevelTile, LoopLevelUnknown };

//! Describes properties of each loop level of a statement.
struct LoopLevel {
  LoopLevelType type;
  /*!
   * For LoopLevelOriginal means iteration space dimension
   * For LoopLevelTile means tiled loop level. Special value -1 for
   * LoopLevelTile means purely derived loop. For dependence dimension
   * payloads, the values must be in an increasing order.
   */
  int payload;
  /*!
   * Used by code generation to support
   * multi-level parallelization (default 0 means sequential loop under
   * the current parallelization level).
   */
  int parallel_level;
  bool segreducible;
  std::string segment_descriptor;
};


struct Statement {
  omega::CG_outputRepr *code;
  omega::Relation IS;
  omega::Relation xform;
  std::vector<LoopLevel> loop_level;
  ir_tree_node *ir_stmt_node;
  bool has_inspector;
  /*!
   * @brief Whether reduction is possible
   *
   * 0 == reduction not possible, 1 == reduction possible, 2 == reduction with some processing
   */
  int reduction;
  IR_OPERATION_TYPE reductionOp; // Manu

  class stencilInfo *statementStencil;
};


class Loop {
protected:
  int tmp_loop_var_name_counter;
  static const std::string tmp_loop_var_name_prefix;
  int overflow_var_name_counter;
  static const std::string overflow_var_name_prefix;
  std::vector<int> stmt_nesting_level_; // UNDERLINE 
  std::vector<std::string> index;
  std::map<int, omega::CG_outputRepr *> replace;
  std::map<int, std::pair<int, std::string> > reduced_statements;
  
public:
  void debugRelations() const;
  IR_Code *ir;
  std::vector<IR_PointerSymbol *> ptr_variables;
  std::vector<omega::Free_Var_Decl*> freevar;
  std::vector<Statement> stmt;
  std::vector<omega::CG_outputRepr*> actual_code;  // ????? 
  std::vector<ir_tree_node *> ir_stmt;
  std::vector<ir_tree_node *> ir_tree;
  std::set<std::string> reduced_write_refs;
  std::map<std::string, int> array_dims;
  DependenceGraph dep;
  int num_dep_dim;
  omega::Relation known;
  omega::CG_outputRepr *init_code;
  omega::CG_outputRepr *cleanup_code;
  std::map<int, std::vector<omega::Free_Var_Decl *> > overflow;
  std::vector<std::map<std::string, std::vector<omega::CG_outputRepr * > > > uninterpreted_symbols;
  std::vector<std::map<std::string, std::vector<omega::CG_outputRepr * > > >uninterpreted_symbols_stringrepr;
  
  
protected:
  mutable omega::CodeGen *last_compute_cg_;
  mutable omega::CG_result *last_compute_cgr_;
  mutable int last_compute_effort_;
  
protected:
  bool init_loop(std::vector<ir_tree_node *> &ir_tree, std::vector<ir_tree_node *> &ir_stmt);
  int get_dep_dim_of(int stmt, int level) const;
  int get_last_dep_dim_before(int stmt, int level) const;
  std::vector<omega::Relation> getNewIS() const;
  omega::Relation getNewIS(int stmt_num) const;
  std::vector<int> getLexicalOrder(int stmt_num) const;
  int getLexicalOrder(int stmt_num, int level) const;
  std::set<int> getStatements(const std::vector<int> &lex, int dim) const;
  void shiftLexicalOrder(const std::vector<int> &lex, int dim, int amount);
  void setLexicalOrder(int dim, const std::set<int> &active, int starting_order = 0, std::vector< std::vector<std::string> >idxNames= std::vector< std::vector<std::string> >());
  void apply_xform(int stmt_num);
  void apply_xform(std::set<int> &active);
  void apply_xform();
  std::set<int> getSubLoopNest(int stmt_num, int level) const;
  int  getMinLexValue(std::set<int> stmts, int level);
 
  
public:
  Loop() { ir = NULL; tmp_loop_var_name_counter = 1; init_code = NULL; }
  Loop(const IR_Control *control);
  ~Loop();
  
  omega::CG_outputRepr *getCode(int effort = 1) const; // TODO was 1

  void stencilASEPadded(int stmt_num); 
  
  void printCode(int effort = 1) const;
  void addKnown(const omega::Relation &cond);
  void print_internal_loop_structure() const;
  bool isInitialized() const;
  int num_statement() const { return stmt.size(); }
  void printIterationSpace() const;
  void printDependenceGraph() const;
  void removeDependence(int stmt_num_from, int stmt_num_to);
  void dump() const;
  
  std::vector<std::set <int > > sort_by_same_loops(std::set<int > active, int level);
  //! legacy unimodular transformations for perfectly nested loops
  /*!
   * e.g. \f$M*(i,j)^T = (i',j')^T or M*(i,j,1)^T = (i',j')^T\f$
   */
  bool nonsingular(const std::vector<std::vector<int> > &M);
  
  void permute(const std::set<int> &active, const std::vector<int> &pi);
  void permute(int stmt_num, int level, const std::vector<int> &pi);
  void permute(const std::vector<int> &pi);
  void original();
  
  void tile(int stmt_num, int level, int tile_size, int outer_level = 1, TilingMethodType method = StridedTile, int alignment_offset = 0, int alignment_multiple = 1);
  std::set<int> split(int stmt_num, int level, const omega::Relation &cond);
  std::set<int> unroll(int stmt_num, int level, int unroll_amount, std::vector< std::vector<std::string> >idxNames= std::vector< std::vector<std::string> >(), int cleanup_split_level = 0);

  //! Datacopy function by reffering arrays by numbers
  /*!
   * for example
   * ~~~
   * A[i] = A[i-1] + B[i];
   * ~~~
   * parameter array_ref_num=[0,2] means to copy data touched by A[i-1] and A[i]
   *
   * @param array_ref_nums
   * @param level
   * @param allow_extra_read
   * @param fastest_changing_dimension
   * @param padding_stride
   * @param padding_alignment
   * @param memory_type
   * @return
   */
  bool datacopy(const std::vector<std::pair<int, std::vector<int> > > &array_ref_nums, int level, bool allow_extra_read = false, int fastest_changing_dimension = -1, int padding_stride = 1, int padding_alignment = 4, int memory_type = 0);
  //! Datacopy function by reffering arrays by name
  /*!
   * parameter array_name=A means to copy data touched by A[i-1] and A[i]
   * @param stmt_num
   * @param level
   * @param array_name
   * @param allow_extra_read
   * @param fastest_changing_dimension
   * @param padding_stride
   * @param padding_alignment
   * @param memory_type
   * @return
   */
  bool datacopy(int stmt_num, int level, const std::string &array_name, bool allow_extra_read = false, int fastest_changing_dimension = -1, int padding_stride = 1, int padding_alignment = 4, int memory_type = 0);
  bool datacopy_privatized(int stmt_num, int level, const std::string &array_name, const std::vector<int> &privatized_levels, bool allow_extra_read = false, int fastest_changing_dimension = -1, int padding_stride = 1, int padding_alignment = 1, int memory_type = 0);
  bool datacopy_privatized(const std::vector<std::pair<int, std::vector<int> > > &array_ref_nums, int level, const std::vector<int> &privatized_levels, bool allow_extra_read = false, int fastest_changing_dimension = -1, int padding_stride = 1, int padding_alignment = 1, int memory_type = 0);
  bool datacopy_privatized(const std::vector<std::pair<int, std::vector<IR_ArrayRef *> > > &stmt_refs, int level, const std::vector<int> &privatized_levels, bool allow_extra_read, int fastest_changing_dimension, int padding_stride, int padding_alignment, int memory_type = 0);
  bool find_stencil_shape( int stmt_num );
  
  
  Graph<std::set<int>, bool> construct_induced_graph_at_level(std::vector<std::set<int> > s, DependenceGraph dep, int dep_dim);
  std::vector<std::set<int> > typed_fusion(Graph<std::set<int>, bool> g, std::vector<bool> &types);

  void fuse(const std::set<int> &stmt_nums, int level);
  void distribute(const std::set<int> &stmt_nums, int level);
  void skew(const std::set<int> &stmt_nums, int level, const std::vector<int> &skew_amount);
  void shift(const std::set<int> &stmt_nums, int level, int shift_amount);
  void scale(const std::set<int> &stmt_nums, int level, int scale_amount);
  void reverse(const std::set<int> &stmt_nums, int level);
  void peel(int stmt_num, int level, int peel_amount = 1);
  void modular_shift(int stmt_num, int level, int shift_amount) {}
  void diagonal_map(int stmt_num, const std::pair<int, int> &levels, int offset) {}
  void modular_partition(int stmt_num, int level, int stride) {}
  void flatten(int stmt_num, std::string index_name, std::vector<int> &loop_levels, std::string inspector_name);
  void normalize(int stmt_num,  int loop_level);

  void generate_ghostcells_v2(std::vector<int> stmt, int loop_num, int ghost_value, int hold_inner_loop_constant=0 );
  


  void shift_to(int stmt_num, int level, int absolute_position);
  std::set<int> unroll_extra(int stmt_num, int level, int unroll_amount, int cleanup_split_level = 0);
  bool is_dependence_valid_based_on_lex_order(int i, int j,
                                              const DependenceVector &dv, bool before);
  void split_with_alignment(int stmt_num, int level, int alignment,
      int direction=0);

  void reduce(int stmt_num, std::vector<int> &level, int param, std::string func_name, std::vector<int> &seq_levels, std::vector<int> cudaized_levels = std::vector<int>(), int bound_level = -1);
  void scalar_expand(int stmt_num, const std::vector<int> &levels, std::string arrName, int memory_type =0, int padding_alignment=0, int assign_then_accumulate = 1, int padding_stride = 0);
  void ELLify(int stmt_num, std::vector<std::string> arrays_to_pad, int pad_to, bool dense_pad = false, std::string dense_pad_pos_array = "");
  void compact(int stmt_num, int level, std::string new_array, int zero,
      std::string data_array);
  void make_dense(int stmt_num, int loop_level, std::string new_loop_index);
  void set_array_size(std::string name, int size );
  omega::CG_outputRepr * iegen_parser(std::string &str, std::vector<std::string> &index_names);

  void pragma(int stmt_num, int level, const std::string &pragmaText);
  void prefetch(int stmt_num, int level, const std::string &arrName, int hint);
};
#endif
