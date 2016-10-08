/*****************************************************************************
 Copyright (C) 2008 University of Southern California
 Copyright (C) 2009-2010 University of Utah
 All Rights Reserved.

 Purpose:
 Core loop transformation functionality.

 Notes:
 "level" (starting from 1) means loop level and it corresponds to "dim"
 (starting from 0) in transformed iteration space [c_1,l_1,c_2,l_2,....,
 c_n,l_n,c_(n+1)], e.g., l_2 is loop level 2 in generated code, dim 3
 in transformed iteration space, and variable 4 in Omega relation.
 All c's are constant numbers only and they will not show up as actual loops.
 Formula:
 dim = 2*level - 1
 var = dim + 1

 History:
 10/2005 Created by Chun Chen.
 09/2009 Expand tile functionality, -chun
 10/2009 Initialize unfusible loop nest without bailing out, -chun
*****************************************************************************/

#include <limits.h>
#include <math.h>
#include <code_gen/codegen.h>
#include <code_gen/CG_utils.h>
#include <code_gen/CG_stringRepr.h>
#include <code_gen/CG_chillRepr.h>   // Mark.  Bad idea.  TODO 
#include <iostream>
#include <algorithm>
#include <map>
#include "loop.hh"
#include "omegatools.hh"
#include "irtools.hh"
#include "chill_error.hh"
#include <string.h>
#include <list>
#include <chilldebug.h>

// TODO 
#define _DEBUG_ true


using namespace omega;

const std::string Loop::tmp_loop_var_name_prefix = std::string(
    "chill_t"); // Manu:: In fortran, first character of a variable name must be a letter, so this change
const std::string Loop::overflow_var_name_prefix = std::string("over");

void echocontroltype(const IR_Control *control) {
  switch (control->type()) {
    case IR_CONTROL_BLOCK: {
      CHILL_DEBUG_PRINT("IR_CONTROL_BLOCK\n");
      break;
    }
    case IR_CONTROL_LOOP: {
      CHILL_DEBUG_PRINT("IR_CONTROL_LOOP\n");
      break;
    }
    case IR_CONTROL_IF: {
      CHILL_DEBUG_PRINT("IR_CONTROL_IF\n");
      break;
    }
    default:
      CHILL_DEBUG_PRINT("just a bunch of statements?\n");

  } // switch
}

omega::Relation Loop::getNewIS(int stmt_num) const {

  omega::Relation result;

  if (stmt[stmt_num].xform.is_null()) {
    omega::Relation known = omega::Extend_Set(omega::copy(this->known),
                                              stmt[stmt_num].IS.n_set() - this->known.n_set());
    result = omega::Intersection(omega::copy(stmt[stmt_num].IS), known);
  } else {
    omega::Relation known = omega::Extend_Set(omega::copy(this->known),
                                              stmt[stmt_num].xform.n_out() - this->known.n_set());
    result = omega::Intersection(
        omega::Range(
            omega::Restrict_Domain(
                omega::copy(stmt[stmt_num].xform),
                omega::copy(stmt[stmt_num].IS))), known);
  }

  result.simplify(2, 4);

  return result;
}


void Loop::reduce(int stmt_num,
                  std::vector<int> &level,
                  int param,
                  std::string func_name,
                  std::vector<int> &seq_levels,
                  std::vector<int> cudaized_levels,
                  int bound_level) {

  // illegal instruction?? fprintf(stderr, " Loop::reduce( stmt %d, param %d, func_name (encrypted)...)\n", stmt, param); // , func_name.c_str()); 

  //std::cout << "Reducing stmt# " << stmt_num << " at level " << level << "\n";
  //ir->printStmt(stmt[stmt_num].code);

  if (stmt[stmt_num].reduction != 1) {
    CHILL_DEBUG_PRINT("Cannot reduce this statement\n");
    return;
  }
  CHILL_DEBUG_PRINT("CAN reduce this statment?\n");

  /*for (int i = 0; i < level.size(); i++)
    if (stmt[stmt_num].loop_level[level[i] - 1].segreducible != true) {
    std::cout << "Cannot reduce this statement\n";
    return;
    }
    for (int i = 0; i < seq_levels.size(); i++)
    if (stmt[stmt_num].loop_level[seq_levels[i] - 1].segreducible != true) {
    std::cout << "Cannot reduce this statement\n";
    return;
    }
  */
  //  std::pair<int, std::string> to_insert(level, func_name);
  //  reduced_statements.insert(std::pair<int, std::pair<int, std::string> >(stmt_num, to_insert ));
  // invalidate saved codegen computation
  delete last_compute_cgr_;
  last_compute_cgr_ = NULL;
  delete last_compute_cg_;
  last_compute_cg_ = NULL;
  fprintf(stderr, "set last_compute_cg_ = NULL;\n");

  omega::CG_outputBuilder *ocg = ir->builder();

  omega::CG_outputRepr *funCallRepr;
  std::vector<omega::CG_outputRepr *> arg_repr_list;
  apply_xform(stmt_num);
  std::vector<IR_ArrayRef *> access = ir->FindArrayRef(stmt[stmt_num].code);
  std::set<std::string> names;
  for (int i = 0; i < access.size(); i++) {
    std::vector<IR_ArrayRef *> access2;
    for (int j = 0; j < access[i]->n_dim(); j++) {
      std::vector<IR_ArrayRef *> access3 = ir->FindArrayRef(
          access[i]->index(j));
      access2.insert(access2.end(), access3.begin(), access3.end());
    }
    if (access2.size() == 0) {
      if (names.find(access[i]->name()) == names.end()) {
        arg_repr_list.push_back(
            ocg->CreateAddressOf(access[i]->convert()));
        names.insert(access[i]->name());
        if (access[i]->is_write())
          reduced_write_refs.insert(access[i]->name());
      }
    } else {
      if (names.find(access[i]->name()) == names.end()) {
        arg_repr_list.push_back(ocg->CreateAddressOf(ocg->CreateArrayRefExpression(ocg->CreateIdent(access[i]->name()),
                                                                                   ocg->CreateInt(0))));
        names.insert(access[i]->name());
        if (access[i]->is_write())
          reduced_write_refs.insert(access[i]->name());
      }
    }
  }

  for (int i = 0; i < seq_levels.size(); i++)
    arg_repr_list.push_back(
        ocg->CreateIdent(
            stmt[stmt_num].IS.set_var(seq_levels[i])->name()));

  if (bound_level != -1) {

    omega::Relation new_IS = copy(stmt[stmt_num].IS);
    new_IS.copy_names(stmt[stmt_num].IS);
    new_IS.setup_names();
    new_IS.simplify();
    int dim = bound_level;
    //omega::Relation r = getNewIS(stmt_num);
    for (int j = dim + 1; j <= new_IS.n_set(); j++)
      new_IS = omega::Project(new_IS, new_IS.set_var(j));

    new_IS.simplify(2, 4);

    omega::Relation bound_ = get_loop_bound(copy(new_IS), dim - 1);
    omega::Variable_ID v = bound_.set_var(dim);
    std::vector<omega::CG_outputRepr *> ubList;
    for (omega::GEQ_Iterator e(
        const_cast<omega::Relation &>(bound_).single_conjunct()->GEQs());
         e; e++) {
      if ((*e).get_coef(v) < 0) {
        //  && (*e).is_const_except_for_global(v))
        omega::CG_outputRepr *UPPERBOUND =
            omega::output_upper_bound_repr(ir->builder(), *e, v,
                                           bound_,
                                           std::vector<
                                               std::pair<omega::CG_outputRepr *, int> >(
                                               bound_.n_set(),
                                               std::make_pair(
                                                   static_cast<omega::CG_outputRepr *>(NULL),
                                                   0)), uninterpreted_symbols[stmt_num]);
        if (UPPERBOUND != NULL)
          ubList.push_back(UPPERBOUND);

      }

    }

    omega::CG_outputRepr *ubRepr;
    if (ubList.size() > 1) {

      ubRepr = ir->builder()->CreateInvoke("min", ubList);
      arg_repr_list.push_back(ubRepr);
    } else if (ubList.size() == 1)
      arg_repr_list.push_back(ubList[0]);
  }

  funCallRepr = ocg->CreateInvoke(func_name, arg_repr_list);
  stmt[stmt_num].code = funCallRepr;
  for (int i = 0; i < level.size(); i++) {
    //stmt[*i].code = outputStatement(ocg, stmt[*i].code, 0, mapping, known, std::vector<CG_outputRepr *>(mapping.n_out(), NULL));
    std::vector<std::string> loop_vars;
    loop_vars.push_back(stmt[stmt_num].IS.set_var(level[i])->name());

    std::vector<omega::CG_outputRepr *> subs;
    subs.push_back(ocg->CreateInt(0));

    stmt[stmt_num].code = ocg->CreateSubstitutedStmt(0, stmt[stmt_num].code,
                                                     loop_vars, subs);

  }

  omega::Relation new_IS = copy(stmt[stmt_num].IS);
  new_IS.copy_names(stmt[stmt_num].IS);
  new_IS.setup_names();
  new_IS.simplify();
  int old_size = new_IS.n_set();

  omega::Relation R = omega::copy(stmt[stmt_num].IS);
  R.copy_names(stmt[stmt_num].IS);
  R.setup_names();

  for (int i = level.size() - 1; i >= 0; i--) {
    int j;

    for (j = 0; j < cudaized_levels.size(); j++) {
      if (cudaized_levels[j] == level[i])
        break;

    }

    if (j == cudaized_levels.size()) {
      R = omega::Project(R, level[i], omega::Input_Var);
      R.simplify();

    }
    //

  }

  omega::F_And *f_Root = R.and_with_and();
  for (int i = level.size() - 1; i >= 0; i--) {
    int j;

    for (j = 0; j < cudaized_levels.size(); j++) {
      if (cudaized_levels[j] == level[i])
        break;

    }

    if (j == cudaized_levels.size()) {

      omega::EQ_Handle h = f_Root->add_EQ();

      h.update_coef(R.set_var(level[i]), 1);
      h.update_const(-1);
    }
    //

  }

  R.simplify();
  stmt[stmt_num].IS = R;
}






//-----------------------------------------------------------------------------
// Class Loop
//-----------------------------------------------------------------------------
// --begin Anand: Added from CHiLL 0.2

bool Loop::isInitialized() const {
  return stmt.size() != 0 && !stmt[0].xform.is_null();
}

//--end Anand: added from CHiLL 0.2

bool Loop::init_loop(std::vector<ir_tree_node *> &ir_tree,
                     std::vector<ir_tree_node *> &ir_stmt) {

  CHILL_DEBUG_PRINT("extract_ir_stmts()\n");
  CHILL_DEBUG_PRINT("ir_tree has %d statements\n", ir_tree.size());

  ir_stmt = extract_ir_stmts(ir_tree);

  CHILL_DEBUG_PRINT("nesting level stmt size = %d\n", (int) ir_stmt.size());
  stmt_nesting_level_.resize(ir_stmt.size());

  std::vector<int> stmt_nesting_level(ir_stmt.size());

  CHILL_DEBUG_PRINT("%d statements?\n", (int) ir_stmt.size());

  // find out how deeply nested each statement is.  (how can these be different?) 
  for (int i = 0; i < ir_stmt.size(); i++) {
    ir_stmt[i]->payload = i;
    int t = 0;
    ir_tree_node *itn = ir_stmt[i];
    while (itn->parent != NULL) {
      itn = itn->parent;
      if (itn->content->type() == IR_CONTROL_LOOP)
        t++;
    }
    stmt_nesting_level_[i] = t;
    stmt_nesting_level[i] = t;
    CHILL_DEBUG_PRINT("stmt_nesting_level[%d] = %d\n", i, t);
  }

  if (actual_code.size() == 0)
    actual_code = std::vector<CG_outputRepr *>(ir_stmt.size());

  stmt = std::vector<Statement>(ir_stmt.size());
  CHILL_DEBUG_PRINT("in init_loop, made %d stmts\n", (int) ir_stmt.size());

  uninterpreted_symbols = std::vector<std::map<std::string, std::vector<omega::CG_outputRepr *> > >(ir_stmt.size());
  uninterpreted_symbols_stringrepr = std::vector<std::map<std::string, std::vector<omega::CG_outputRepr *> > >(
      ir_stmt.size());

  int n_dim = -1;
  int max_loc;
  for (int i = 0; i < ir_stmt.size(); i++) {
    int max_nesting_level = -1;
    int loc;

    // find the max nesting level and remember the statement that was at that level
    for (int j = 0; j < ir_stmt.size(); j++) {
      if (stmt_nesting_level[j] > max_nesting_level) {
        max_nesting_level = stmt_nesting_level[j];
        loc = j;
      }
    }

    CHILL_DEBUG_PRINT("max nesting level %d at location %d\n", max_nesting_level, loc);

    // most deeply nested statement acting as a reference point
    if (n_dim == -1) {
      CHILL_DEBUG_PRINT("n_dim now max_nesting_level %d\n", max_nesting_level);
      n_dim = max_nesting_level;
      max_loc = loc;

      index = std::vector<std::string>(n_dim);

      ir_tree_node *itn = ir_stmt[loc];
      CHILL_DEBUG_PRINT("itn = stmt[%d]\n", loc);
      int cur_dim = n_dim - 1;
      while (itn->parent != NULL) {
        CHILL_DEBUG_PRINT("parent\n");

        itn = itn->parent;
        if (itn->content->type() == IR_CONTROL_LOOP) {
          CHILL_DEBUG_PRINT("IR_CONTROL_LOOP  cur_dim %d\n", cur_dim);
          IR_Loop *IRL = static_cast<IR_Loop *>(itn->content);
          index[cur_dim] = IRL->index()->name();
          CHILL_DEBUG_PRINT("index[%d] = '%s'\n", cur_dim, index[cur_dim].c_str());
          itn->payload = cur_dim--;
        }
      }
    }

    CHILL_DEBUG_PRINT("align loops by names,\n");
    // align loops by names, temporary solution
    ir_tree_node *itn = ir_stmt[loc];  // defined outside loops?? 
    int depth = stmt_nesting_level_[loc] - 1;

    for (int t = depth; t >= 0; t--) {
      int y = t;
      itn = ir_stmt[loc];

      while ((itn->parent != NULL) && (y >= 0)) {
        itn = itn->parent;
        if (itn->content->type() == IR_CONTROL_LOOP)
          y--;
      }

      if (itn->content->type() == IR_CONTROL_LOOP && itn->payload == -1) {
        CG_outputBuilder *ocg = ir->builder();

        itn->payload = depth - t;

        CG_outputRepr *code =
            static_cast<IR_Block *>(ir_stmt[loc]->content)->extract();

        std::vector<CG_outputRepr *> index_expr;
        std::vector<std::string> old_index;
        CG_outputRepr *repl = ocg->CreateIdent(index[itn->payload]);
        index_expr.push_back(repl);
        old_index.push_back(
            static_cast<IR_Loop *>(itn->content)->index()->name());
        code = ocg->CreateSubstitutedStmt(0, code, old_index,
                                          index_expr);

        replace.insert(std::pair<int, CG_outputRepr *>(loc, code));
        //stmt[loc].code = code;

      }
    }

    CHILL_DEBUG_PRINT("set relation variable names                      ****\n");
    // set relation variable names

    // this finds the loop variables for loops enclosing this statement and puts
    // them in an Omega Relation (just their names, which could fail) 

    CHILL_DEBUG_PRINT("Relation r(%d)\n", n_dim);
    Relation r(n_dim);
    F_And *f_root = r.add_and();
    itn = ir_stmt[loc];
    int temp_depth = depth;
    while (itn->parent != NULL) {

      itn = itn->parent;
      if (itn->content->type() == IR_CONTROL_LOOP) {
        CHILL_DEBUG_PRINT("it's a loop.  temp_depth %d\n", temp_depth);
        CHILL_DEBUG_PRINT("r.name_set_var( %d, %s )\n", itn->payload + 1, index[temp_depth].c_str());
        r.name_set_var(itn->payload + 1, index[temp_depth]);
        temp_depth--;
      }
    }
    CHILL_DEBUG_PRINT("extract information from loop/if structures\n");
    // extract information from loop/if structures
    std::vector<bool> processed(n_dim, false);
    std::vector<std::string> vars_to_be_reversed;

    std::vector<std::string> insp_lb;
    std::vector<std::string> insp_ub;

    itn = ir_stmt[loc];
    while (itn->parent != NULL) { // keep heading upward 
      itn = itn->parent;

      switch (itn->content->type()) {
        case IR_CONTROL_LOOP: {
          CHILL_DEBUG_PRINT("IR_CONTROL_LOOP\n");
          IR_Loop *lp = static_cast<IR_Loop *>(itn->content);
          Variable_ID v = r.set_var(itn->payload + 1);
          int c;

          try {
            c = lp->step_size();
            if (c > 0) {
              CG_outputRepr *lb = lp->lower_bound();
              CHILL_DEBUG_BEGIN
                fprintf(stderr, "got the lower bound. it is:\n");
                lb->dump();
              CHILL_DEBUG_END
              exp2formula(ir, r, f_root, freevar, lb, v, 's',
                          IR_COND_GE, true, uninterpreted_symbols[i], uninterpreted_symbols_stringrepr[i]);

              CG_outputRepr *ub = lp->upper_bound();
              CHILL_DEBUG_BEGIN
                fprintf(stderr, "got the upper bound. it is:\n");
                ub->dump();
              CHILL_DEBUG_END

              IR_CONDITION_TYPE cond = lp->stop_cond();
              if (cond == IR_COND_LT || cond == IR_COND_LE)
                exp2formula(ir, r, f_root, freevar, ub, v, 's',
                            cond, true, uninterpreted_symbols[i], uninterpreted_symbols_stringrepr[i]);
              else
                throw chill::error::ir("loop condition not supported");


              if ((ir->QueryExpOperation(lp->lower_bound())
                   == IR_OP_ARRAY_VARIABLE)
                  && (ir->QueryExpOperation(lp->lower_bound())
                      == ir->QueryExpOperation(
                  lp->upper_bound()))) {
                std::vector<CG_outputRepr *> v =
                    ir->QueryExpOperand(lp->lower_bound());
                IR_ArrayRef *ref =
                    static_cast<IR_ArrayRef *>(ir->Repr2Ref(
                        v[0]));
                std::string s0 = ref->name();
                std::vector<CG_outputRepr *> v2 =
                    ir->QueryExpOperand(lp->upper_bound());
                IR_ArrayRef *ref2 =
                    static_cast<IR_ArrayRef *>(ir->Repr2Ref(
                        v2[0]));
                std::string s1 = ref2->name();

                if (s0 == s1) {
                  insp_lb.push_back(s0);
                  insp_ub.push_back(s1);

                }

              }


            } else if (c < 0) {
              CG_outputBuilder *ocg = ir->builder();
              CG_outputRepr *lb = lp->lower_bound();
              lb = ocg->CreateMinus(NULL, lb);
              exp2formula(ir, r, f_root, freevar, lb, v, 's',
                          IR_COND_GE, true, uninterpreted_symbols[i], uninterpreted_symbols_stringrepr[i]);
              CG_outputRepr *ub = lp->upper_bound();
              ub = ocg->CreateMinus(NULL, ub);
              IR_CONDITION_TYPE cond = lp->stop_cond();
              if (cond == IR_COND_GE)
                exp2formula(ir, r, f_root, freevar, ub, v, 's',
                            IR_COND_LE, true, uninterpreted_symbols[i], uninterpreted_symbols_stringrepr[i]);
              else if (cond == IR_COND_GT)
                exp2formula(ir, r, f_root, freevar, ub, v, 's',
                            IR_COND_LT, true, uninterpreted_symbols[i], uninterpreted_symbols_stringrepr[i]);
              else
                throw chill::error::ir("loop condition not supported");

              vars_to_be_reversed.push_back(lp->index()->name());
            } else
              throw chill::error::ir("loop step size zero");
          } catch (const chill::error::ir &e) {
            actual_code[loc] =
                static_cast<IR_Block *>(ir_stmt[loc]->content)->extract();
            for (int i = 0; i < itn->children.size(); i++)
              delete itn->children[i];
            itn->children = std::vector<ir_tree_node *>();
            itn->content = itn->content->convert();
            return false;
          }

          // check for loop increment or decrement that is not 1
          if (abs(c) != 1) {
            F_Exists *f_exists = f_root->add_exists();
            Variable_ID e = f_exists->declare();
            F_And *f_and = f_exists->add_and();
            Stride_Handle h = f_and->add_stride(abs(c));
            if (c > 0)
              h.update_coef(e, 1);
            else
              h.update_coef(e, -1);
            h.update_coef(v, -1);
            CG_outputRepr *lb = lp->lower_bound();
            exp2formula(ir, r, f_and, freevar, lb, e, 's', IR_COND_EQ,
                        true, uninterpreted_symbols[i], uninterpreted_symbols_stringrepr[i]);
          }

          processed[itn->payload] = true;
          break;
        }


        case IR_CONTROL_IF: {
          CHILL_DEBUG_PRINT("IR_CONTROL_IF\n");
          IR_If *theif = static_cast<IR_If *>(itn->content);

          CG_outputRepr *cond =
              static_cast<IR_If *>(itn->content)->condition();

          try {
            if (itn->payload % 2 == 1)
              exp2constraint(ir, r, f_root, freevar, cond, true, uninterpreted_symbols[i],
                             uninterpreted_symbols_stringrepr[i]);
            else {
              F_Not *f_not = f_root->add_not();
              F_And *f_and = f_not->add_and();
              exp2constraint(ir, r, f_and, freevar, cond, true, uninterpreted_symbols[i],
                             uninterpreted_symbols_stringrepr[i]);
            }
          } catch (const chill::error::ir&e) {
            std::vector<ir_tree_node *> *t;
            if (itn->parent == NULL)
              t = &ir_tree;
            else
              t = &(itn->parent->children);
            int id = itn->payload;
            int i = t->size() - 1;
            while (i >= 0) {
              if ((*t)[i] == itn) {
                for (int j = 0; j < itn->children.size(); j++)
                  delete itn->children[j];
                itn->children = std::vector<ir_tree_node *>();
                itn->content = itn->content->convert();
              } else if ((*t)[i]->payload >> 1 == id >> 1) {
                delete (*t)[i];
                t->erase(t->begin() + i);
              }
              i--;
            }
            return false;
          }

          break;
        }
        default:
          for (int i = 0; i < itn->children.size(); i++)
            delete itn->children[i];
          itn->children = std::vector<ir_tree_node *>();
          itn->content = itn->content->convert();
          return false;
      }
    }

    // add information for missing loops
    for (int j = 0; j < n_dim; j++)
      if (!processed[j]) {
        ir_tree_node *itn = ir_stmt[max_loc];
        while (itn->parent != NULL) {
          itn = itn->parent;
          if (itn->content->type() == IR_CONTROL_LOOP
              && itn->payload == j)
            break;
        }

        Variable_ID v = r.set_var(j + 1);
        if (loc < max_loc) {

          CG_outputBuilder *ocg = ir->builder();

          CG_outputRepr *lb =
              static_cast<IR_Loop *>(itn->content)->lower_bound();

          exp2formula(ir, r, f_root, freevar, lb, v, 's', IR_COND_EQ,
                      false, uninterpreted_symbols[i], uninterpreted_symbols_stringrepr[i]);
        } else { // loc > max_loc

          CG_outputBuilder *ocg = ir->builder();
          CG_outputRepr *ub =
              static_cast<IR_Loop *>(itn->content)->upper_bound();

          exp2formula(ir, r, f_root, freevar, ub, v, 's', IR_COND_EQ,
                      false, uninterpreted_symbols[i], uninterpreted_symbols_stringrepr[i]);
        }
      }

    r.setup_names();
    r.simplify();

    // THIS IS MISSING IN PROTONU's
    for (int j = 0; j < insp_lb.size(); j++) {

      std::string lb = insp_lb[j] + "_";
      std::string ub = lb + "_";

      Global_Var_ID u, l;
      bool found_ub = false;
      bool found_lb = false;
      for (DNF_Iterator di(copy(r).query_DNF()); di; di++)
        for (Constraint_Iterator ci = (*di)->constraints(); ci; ci++)

          for (Constr_Vars_Iter cvi(*ci); cvi; cvi++) {
            Variable_ID v = cvi.curr_var();
            if (v->kind() == Global_Var)
              if (v->get_global_var()->arity() > 0) {

                std::string name =
                    v->get_global_var()->base_name();
                if (name == lb) {
                  l = v->get_global_var();
                  found_lb = true;
                } else if (name == ub) {
                  u = v->get_global_var();
                  found_ub = true;
                }
              }

          }

      if (found_lb && found_ub) {
        Relation known_(copy(r).n_set());
        known_.copy_names(copy(r));
        known_.setup_names();
        Variable_ID index_lb = known_.get_local(l, Input_Tuple);
        Variable_ID index_ub = known_.get_local(u, Input_Tuple);
        F_And *fr = known_.add_and();
        GEQ_Handle g = fr->add_GEQ();
        g.update_coef(index_ub, 1);
        g.update_coef(index_lb, -1);
        g.update_const(-1);
        addKnown(known_);
      }
    }
    // insert the statement
    CG_outputBuilder *ocg = ir->builder();
    std::vector<CG_outputRepr *> reverse_expr;
    for (int j = 1; j <= vars_to_be_reversed.size(); j++) {
      CG_outputRepr *repl = ocg->CreateIdent(vars_to_be_reversed[j]);
      repl = ocg->CreateMinus(NULL, repl);
      reverse_expr.push_back(repl);
    }
    CG_outputRepr *code =
        static_cast<IR_Block *>(ir_stmt[loc]->content)->extract();
    code = ocg->CreateSubstitutedStmt(0, code, vars_to_be_reversed,
                                      reverse_expr);
    stmt[loc].code = code;
    stmt[loc].IS = r;

    //Anand: Add Information on uninterpreted function constraints to
    //Known relation

    CHILL_DEBUG_PRINT("stmt[%d].loop_level has size n_dim %d\n", loc, n_dim);

    stmt[loc].loop_level = std::vector<LoopLevel>(n_dim);
    stmt[loc].ir_stmt_node = ir_stmt[loc];
    stmt[loc].has_inspector = false;
    for (int ii = 0; ii < n_dim; ii++) {
      stmt[loc].loop_level[ii].type = LoopLevelOriginal;
      stmt[loc].loop_level[ii].payload = ii;
      stmt[loc].loop_level[ii].parallel_level = 0;
    }
    stmt_nesting_level[loc] = -1;
  }
  return true;
}


Loop::Loop(const IR_Control *control) {

  CHILL_DEBUG_PRINT("control type is %d   \n", control->type());
  echocontroltype(control);

  CHILL_DEBUG_PRINT("set last_compute_cg_ = NULL; \n");
  last_compute_cgr_ = NULL;
  last_compute_cg_ = NULL;

  ir = const_cast<IR_Code *>(control->ir_); // point to the CHILL IR that this loop came from
  if (ir == 0) {
    CHILL_DEBUG_PRINT("ir gotten from control = 0x%x\n", (long) ir);
    CHILL_DEBUG_PRINT("GONNA DIE SOON *******************************\n\n");
  }

  init_code = NULL;
  cleanup_code = NULL;
  tmp_loop_var_name_counter = 1;
  overflow_var_name_counter = 1;
  known = Relation::True(0);

  CHILL_DEBUG_PRINT("calling build_ir_tree()\n");
  CHILL_DEBUG_PRINT("about to clone control\n");
  ir_tree = build_ir_tree(control->clone(), NULL);

  int count = 0;
  while (!init_loop(ir_tree, ir_stmt));

  CHILL_DEBUG_PRINT("after init_loop, %d freevar\n", (int) freevar.size());

  CHILL_DEBUG_PRINT("after init_loop, %d statements\n", (int) stmt.size());
  for (int i = 0; i < stmt.size(); i++) {
    std::map<int, CG_outputRepr *>::iterator it = replace.find(i);
    if (it != replace.end())
      stmt[i].code = it->second;
    else
      stmt[i].code = stmt[i].code;
  }

  if (stmt.size() != 0)
    dep = DependenceGraph(stmt[0].IS.n_set());
  else
    dep = DependenceGraph(0);
  // init the dependence graph
  for (int i = 0; i < stmt.size(); i++)
    dep.insert();

  // TODO this really REALLY needs some comments
  for (int i = 0; i < stmt.size(); i++) {
    stmt[i].reduction = 0; // Manu -- initialization
    for (int j = i; j < stmt.size(); j++) {
      std::pair<std::vector<DependenceVector>,
          std::vector<DependenceVector> > dv = test_data_dependences(
          ir,
          stmt[i].code,
          stmt[i].IS,
          stmt[j].code,
          stmt[j].IS,
          freevar,
          index,
          stmt_nesting_level_[i],
          stmt_nesting_level_[j],
          uninterpreted_symbols[i],
          uninterpreted_symbols_stringrepr[i]);
      for (int k = 0; k < dv.first.size(); k++) {
        if (is_dependence_valid(ir_stmt[i], ir_stmt[j], dv.first[k],
                                true))
          dep.connect(i, j, dv.first[k]);
        else {
          dep.connect(j, i, dv.first[k].reverse());
        }

      }

      for (int k = 0; k < dv.second.size(); k++) {
        if (is_dependence_valid(ir_stmt[j], ir_stmt[i], dv.second[k],
                                false))
          dep.connect(j, i, dv.second[k]);
        else {
          dep.connect(i, j, dv.second[k].reverse());
        }
      }
    }
  }

  CHILL_DEBUG_PRINT("*** LOTS OF REDUCTIONS ***\n");

  // TODO: Reduction check
  // Manu:: Initial implementation / algorithm
  std::set<int> reducCand = std::set<int>();
  std::vector<int> canReduce = std::vector<int>();
  for (int i = 0; i < stmt.size(); i++) {
    if (!dep.hasEdge(i, i)) {
      continue;
    }
    // for each statement check if it has all the three dependences (RAW, WAR, WAW)
    // If there is such a statement, it is a reduction candidate. Mark all reduction candidates.
    std::vector<DependenceVector> tdv = dep.getEdge(i, i);
    for (int j = 0; j < tdv.size(); j++) {
      if (tdv[j].is_reduction_cand) {
        reducCand.insert(i);
      }
    }
  }
  bool reduc;
  std::set<int>::iterator it;
  int counter = 0;
  for (it = reducCand.begin(); it != reducCand.end(); it++) {
    reduc = true;
    for (int j = 0; j < stmt.size(); j++) {
      if ((*it != j)
          && (stmt_nesting_level_[*it] < stmt_nesting_level_[j])) {
        if (dep.hasEdge(*it, j) || dep.hasEdge(j, *it)) {
          reduc = false;
          break;
        }
      }
      counter += 1;
    }
    if (reduc) {
      canReduce.push_back(*it);
      stmt[*it].reduction = 2; // First, assume that reduction is possible with some processing
    }
  }
  // If reduction is possible without processing, update the value of the reduction variable to 1
  CHILL_DEBUG_PRINT("canReduce.size() %d\n", canReduce.size());
  for (int i = 0; i < canReduce.size(); i++) {
    // Here, assuming that stmtType returns 1 when there is a single statement within stmt[i]
    if (stmtType(ir, stmt[canReduce[i]].code) == 1) {
      stmt[canReduce[i]].reduction = 1;
      IR_OPERATION_TYPE opType;
      opType = getReductionOperator(ir, stmt[canReduce[i]].code);
      stmt[canReduce[i]].reductionOp = opType;
    }
  }

  // printing out stuff for debugging
  CHILL_DEBUG_BEGIN
    std::cout << "STATEMENTS THAT CAN BE REDUCED: \n";
    for (int i = 0; i < canReduce.size(); i++) {
      std::cout << "------- " << canReduce[i] << " ------- "
                << stmt[canReduce[i]].reduction << "\n";
      ir->printStmt(stmt[canReduce[i]].code); // Manu
      if (stmt[canReduce[i]].reductionOp == IR_OP_PLUS)
        std::cout << "Reduction type:: + \n";
      else if (stmt[canReduce[i]].reductionOp == IR_OP_MINUS)
        std::cout << "Reduction type:: - \n";
      else if (stmt[canReduce[i]].reductionOp == IR_OP_MULTIPLY)
        std::cout << "Reduction type:: * \n";
      else if (stmt[canReduce[i]].reductionOp == IR_OP_DIVIDE)
        std::cout << "Reduction type:: / \n";
      else
        std::cout << "Unknown reduction type\n";
    }
  CHILL_DEBUG_END
  // cleanup the IR tree

  CHILL_DEBUG_PRINT("init dumb transformation relations\n");

  // init dumb transformation relations e.g. [i, j] -> [ 0, i, 0, j, 0]
  for (int i = 0; i < stmt.size(); i++) {
    int n = stmt[i].IS.n_set();
    stmt[i].xform = Relation(n, 2 * n + 1);
    F_And *f_root = stmt[i].xform.add_and();

    for (int j = 1; j <= n; j++) {
      EQ_Handle h = f_root->add_EQ();
      h.update_coef(stmt[i].xform.output_var(2 * j), 1);
      h.update_coef(stmt[i].xform.input_var(j), -1);
    }

    for (int j = 1; j <= 2 * n + 1; j += 2) {
      EQ_Handle h = f_root->add_EQ();
      h.update_coef(stmt[i].xform.output_var(j), 1);
    }
    stmt[i].xform.simplify();
  }
  CHILL_DEBUG_PRINT("done with dumb\n");

  if (stmt.size() != 0)
    num_dep_dim = stmt[0].IS.n_set();
  else
    num_dep_dim = 0;
}

Loop::~Loop() {

  delete last_compute_cgr_;
  delete last_compute_cg_;

  for (int i = 0; i < stmt.size(); i++)
    if (stmt[i].code != NULL) {
      stmt[i].code->clear();
      delete stmt[i].code;
    }

  for (int i = 0; i < ir_tree.size(); i++)
    delete ir_tree[i];

  if (init_code != NULL) {
    init_code->clear();
    delete init_code;
  }
  if (cleanup_code != NULL) {
    cleanup_code->clear();
    delete cleanup_code;
  }
}


int Loop::get_dep_dim_of(int stmt_num, int level) const {
  if (stmt_num < 0 || stmt_num >= stmt.size())
    throw std::invalid_argument("invaid statement " + to_string(stmt_num));

  if (level < 1 || level > stmt[stmt_num].loop_level.size())
    return -1;

  int trip_count = 0;
  while (true) {
    switch (stmt[stmt_num].loop_level[level - 1].type) {
      case LoopLevelOriginal:
        return stmt[stmt_num].loop_level[level - 1].payload;
      case LoopLevelTile:
        level = stmt[stmt_num].loop_level[level - 1].payload;
        if (level < 1)
          return -1;
        if (level > stmt[stmt_num].loop_level.size())
          throw chill::error::loop("incorrect loop level information for statement "
                           + to_string(stmt_num));
        break;
      default:
        throw chill::error::loop(
            "unknown loop level information for statement "
            + to_string(stmt_num));
    }
    trip_count++;
    if (trip_count >= stmt[stmt_num].loop_level.size())
      throw chill::error::loop(
          "incorrect loop level information for statement "
          + to_string(stmt_num));
  }
}

int Loop::get_last_dep_dim_before(int stmt_num, int level) const {
  if (stmt_num < 0 || stmt_num >= stmt.size())
    throw std::invalid_argument("invaid statement " + to_string(stmt_num));

  if (level < 1)
    return -1;
  if (level > stmt[stmt_num].loop_level.size())
    level = stmt[stmt_num].loop_level.size() + 1;

  for (int i = level - 1; i >= 1; i--)
    if (stmt[stmt_num].loop_level[i - 1].type == LoopLevelOriginal)
      return stmt[stmt_num].loop_level[i - 1].payload;

  return -1;
}

void Loop::print_internal_loop_structure() const {
  for (int i = 0; i < stmt.size(); i++) {
    std::vector<int> lex = getLexicalOrder(i);
    std::cout << "s" << i + 1 << ": ";
    for (int j = 0; j < stmt[i].loop_level.size(); j++) {
      if (2 * j < lex.size())
        std::cout << lex[2 * j];
      switch (stmt[i].loop_level[j].type) {
        case LoopLevelOriginal:
          std::cout << "(dim:" << stmt[i].loop_level[j].payload << ")";
          break;
        case LoopLevelTile:
          std::cout << "(tile:" << stmt[i].loop_level[j].payload << ")";
          break;
        default:
          std::cout << "(unknown)";
      }
      std::cout << ' ';
    }
    for (int j = 2 * stmt[i].loop_level.size(); j < lex.size(); j += 2) {
      std::cout << lex[j];
      if (j != lex.size() - 1)
        std::cout << ' ';
    }
    std::cout << std::endl;
  }
}

void Loop::debugRelations() const {
  const int m = stmt.size();
  {
    std::vector<Relation> IS(m);
    std::vector<Relation> xforms(m);

    for (int i = 0; i < m; i++) {
      IS[i] = stmt[i].IS;
      xforms[i] = stmt[i].xform;  // const stucks
    }

    printf("\nxforms:\n");
    for (int i = 0; i < m; i++) {
      xforms[i].print();
      printf("\n");
    }
    printf("\nIS:\n");
    for (int i = 0; i < m; i++) {
      IS[i].print();
      printf("\n");
    }
    fflush(stdout);
  }
}


CG_outputRepr *Loop::getCode(int effort) const {
  CHILL_DEBUG_PRINT("effort %d\n", effort);

  const int m = stmt.size();
  if (m == 0)
    return NULL;
  const int n = stmt[0].xform.n_out();

  if (last_compute_cg_ == NULL) {
    CHILL_DEBUG_PRINT("last_compute_cg_ == NULL\n");

    std::vector<Relation> IS(m);
    std::vector<Relation> xforms(m);
    for (int i = 0; i < m; i++) {
      IS[i] = stmt[i].IS;
      xforms[i] = stmt[i].xform;
    }

    debugRelations();


    Relation known = Extend_Set(copy(this->known), n - this->known.n_set());

    last_compute_cg_ = new CodeGen(xforms, IS, known);
    delete last_compute_cgr_;
    last_compute_cgr_ = NULL;
  } else
    CHILL_DEBUG_PRINT("last_compute_cg_ NOT NULL\n");

  if (last_compute_cgr_ == NULL || last_compute_effort_ != effort) {
    delete last_compute_cgr_;
    last_compute_cgr_ = last_compute_cg_->buildAST(effort);
    last_compute_effort_ = effort;
  }

  std::vector<CG_outputRepr *> stmts(m);
  for (int i = 0; i < m; i++)
    stmts[i] = stmt[i].code;
  CG_outputBuilder *ocg = ir->builder();

  CHILL_DEBUG_PRINT("calling last_compute_cgr_->printRepr()\n");
  CG_outputRepr *repr = last_compute_cgr_->printRepr(ocg, stmts,
                                                     uninterpreted_symbols);

  if (init_code != NULL)
    repr = ocg->StmtListAppend(init_code->clone(), repr);
  if (cleanup_code != NULL)
    repr = ocg->StmtListAppend(repr, cleanup_code->clone());

  return repr;
}


void Loop::printCode(int effort) const {
  CHILL_DEBUG_PRINT("effort %d\n", effort);
  const int m = stmt.size();
  if (m == 0)
    return;
  const int n = stmt[0].xform.n_out();

  if (last_compute_cg_ == NULL) {
    CHILL_DEBUG_PRINT("last_compute_cg_ == NULL\n");
    std::vector<Relation> IS(m);
    std::vector<Relation> xforms(m);
    for (int i = 0; i < m; i++) {
      IS[i] = stmt[i].IS;
      xforms[i] = stmt[i].xform;
    }
    Relation known = Extend_Set(copy(this->known), n - this->known.n_set());

    last_compute_cg_ = new CodeGen(xforms, IS, known);
    delete last_compute_cgr_;
    last_compute_cgr_ = NULL;
  } else CHILL_DEBUG_PRINT("last_compute_cg_ NOT NULL\n");

  if (last_compute_cgr_ == NULL || last_compute_effort_ != effort) {
    delete last_compute_cgr_;
    last_compute_cgr_ = last_compute_cg_->buildAST(effort);
    last_compute_effort_ = effort;
  }

  std::string repr = last_compute_cgr_->printString(
      uninterpreted_symbols_stringrepr);
  std::cout << repr << std::endl;
}

void Loop::printIterationSpace() const {
  for (int i = 0; i < stmt.size(); i++) {
    std::cout << "s" << i << ": ";
    Relation r = getNewIS(i);
    for (int j = 1; j <= r.n_inp(); j++)
      r.name_input_var(j, CodeGen::loop_var_name_prefix + to_string(j));
    r.setup_names();
    r.print();
  }
}

void Loop::printDependenceGraph() const {
  if (dep.edgeCount() == 0)
    std::cout << "no dependence exists" << std::endl;
  else {
    std::cout << "dependence graph:" << std::endl;
    std::cout << dep;
  }
}

std::vector<Relation> Loop::getNewIS() const {
  const int m = stmt.size();

  std::vector<Relation> new_IS(m);
  for (int i = 0; i < m; i++)
    new_IS[i] = getNewIS(i);

  return new_IS;
}

// pragmas are tied to loops only ??? 
void Loop::pragma(int stmt_num, int level, const std::string &pragmaText) {
  // check sanity of parameters
  if (stmt_num < 0)
    throw std::invalid_argument("invalid statement " + to_string(stmt_num));

  CG_outputBuilder *ocg = ir->builder();
  CG_outputRepr *code = stmt[stmt_num].code;
  ocg->CreatePragmaAttribute(code, level, pragmaText);
}

void Loop::prefetch(int stmt_num, int level, const std::string &arrName, int hint) {
  // check sanity of parameters
  if (stmt_num < 0)
    throw std::invalid_argument("invalid statement " + to_string(stmt_num));

  CG_outputBuilder *ocg = ir->builder();
  CG_outputRepr *code = stmt[stmt_num].code;
  ocg->CreatePrefetchAttribute(code, level, arrName, hint);
}

std::vector<int> Loop::getLexicalOrder(int stmt_num) const {
  assert(stmt_num < stmt.size());

  const int n = stmt[stmt_num].xform.n_out();
  std::vector<int> lex(n, 0);

  for (int i = 0; i < n; i += 2)
    lex[i] = get_const(stmt[stmt_num].xform, i, Output_Var);

  return lex;
}

// find the sub loop nest specified by stmt_num and level,
// only iteration space satisfiable statements returned.
std::set<int> Loop::getSubLoopNest(int stmt_num, int level) const {
  assert(stmt_num >= 0 && stmt_num < stmt.size());
  assert(level > 0 && level <= stmt[stmt_num].loop_level.size());

  std::set<int> working;
  for (int i = 0; i < stmt.size(); i++)
    if (const_cast<Loop *>(this)->stmt[i].IS.is_upper_bound_satisfiable()
        && stmt[i].loop_level.size() >= level)
      working.insert(i);

  for (int i = 1; i <= level; i++) {
    int a = getLexicalOrder(stmt_num, i);
    for (std::set<int>::iterator j = working.begin(); j != working.end();) {
      int b = getLexicalOrder(*j, i);
      if (b != a)
        working.erase(j++);
      else
        ++j;
    }
  }

  return working;
}

int Loop::getLexicalOrder(int stmt_num, int level) const {
  assert(stmt_num >= 0 && stmt_num < stmt.size());
  assert(level > 0 && level <= stmt[stmt_num].loop_level.size() + 1);

  Relation &r = const_cast<Loop *>(this)->stmt[stmt_num].xform;
  for (EQ_Iterator e(r.single_conjunct()->EQs()); e; e++)
    if (abs((*e).get_coef(r.output_var(2 * level - 1))) == 1) {
      bool is_const = true;
      for (Constr_Vars_Iter cvi(*e); cvi; cvi++)
        if (cvi.curr_var() != r.output_var(2 * level - 1)) {
          is_const = false;
          break;
        }
      if (is_const) {
        int t = static_cast<int>((*e).get_const());
        return (*e).get_coef(r.output_var(2 * level - 1)) > 0 ? -t : t;
      }
    }

  throw chill::error::loop(
      "can't find lexical order for statement " + to_string(stmt_num)
      + "'s loop level " + to_string(level));
}

std::set<int> Loop::getStatements(const std::vector<int> &lex, int dim) const {
  const int m = stmt.size();

  std::set<int> same_loops;
  for (int i = 0; i < m; i++) {
    if (dim < 0)
      same_loops.insert(i);
    else {
      std::vector<int> a_lex = getLexicalOrder(i);
      int j;
      for (j = 0; j <= dim; j += 2)
        if (lex[j] != a_lex[j])
          break;
      if (j > dim)
        same_loops.insert(i);
    }

  }

  return same_loops;
}

void Loop::shiftLexicalOrder(const std::vector<int> &lex, int dim, int amount) {
  const int m = stmt.size();

  if (amount == 0)
    return;

  for (int i = 0; i < m; i++) {
    std::vector<int> lex2 = getLexicalOrder(i);

    bool need_shift = true;

    for (int j = 0; j < dim; j++)
      if (lex2[j] != lex[j]) {
        need_shift = false;
        break;
      }

    if (!need_shift)
      continue;

    if (amount > 0) {
      if (lex2[dim] < lex[dim])
        continue;
    } else if (amount < 0) {
      if (lex2[dim] > lex[dim])
        continue;
    }

    assign_const(stmt[i].xform, dim, lex2[dim] + amount);
  }
}

std::vector<std::set<int> > Loop::sort_by_same_loops(std::set<int> active,
                                                     int level) {

  std::set<int> not_nested_at_this_level;
  std::map<ir_tree_node *, std::set<int> > sorted_by_loop;
  std::map<int, std::set<int> > sorted_by_lex_order;
  std::vector<std::set<int> > to_return;
  bool lex_order_already_set = false;
  for (std::set<int>::iterator it = active.begin(); it != active.end();
       it++) {

    if (stmt[*it].ir_stmt_node == NULL)
      lex_order_already_set = true;
  }

  if (lex_order_already_set) {

    for (std::set<int>::iterator it = active.begin(); it != active.end();
         it++) {
      std::map<int, std::set<int> >::iterator it2 =
          sorted_by_lex_order.find(
              get_const(stmt[*it].xform, 2 * (level - 1),
                        Output_Var));

      if (it2 != sorted_by_lex_order.end())
        it2->second.insert(*it);
      else {

        std::set<int> to_insert;

        to_insert.insert(*it);

        sorted_by_lex_order.insert(
            std::pair<int, std::set<int> >(
                get_const(stmt[*it].xform, 2 * (level - 1),
                          Output_Var), to_insert));

      }

    }

    for (std::map<int, std::set<int> >::iterator it2 =
        sorted_by_lex_order.begin(); it2 != sorted_by_lex_order.end();
         it2++)
      to_return.push_back(it2->second);

  } else {

    for (std::set<int>::iterator it = active.begin(); it != active.end();
         it++) {

      ir_tree_node *itn = stmt[*it].ir_stmt_node;
      itn = itn->parent;
      //while (itn->content->type() != IR_CONTROL_LOOP && itn != NULL)
      //  itn = itn->parent;

      while ((itn != NULL) && (itn->payload != level - 1)) {
        itn = itn->parent;
        while (itn != NULL && itn->content->type() != IR_CONTROL_LOOP)
          itn = itn->parent;
      }

      if (itn == NULL)
        not_nested_at_this_level.insert(*it);
      else {
        std::map<ir_tree_node *, std::set<int> >::iterator it2 =
            sorted_by_loop.find(itn);

        if (it2 != sorted_by_loop.end())
          it2->second.insert(*it);
        else {
          std::set<int> to_insert;

          to_insert.insert(*it);

          sorted_by_loop.insert(
              std::pair<ir_tree_node *, std::set<int> >(itn,
                                                        to_insert));

        }

      }

    }
    if (not_nested_at_this_level.size() > 0) {
      for (std::set<int>::iterator it = not_nested_at_this_level.begin();
           it != not_nested_at_this_level.end(); it++) {
        std::set<int> temp;
        temp.insert(*it);
        to_return.push_back(temp);

      }
    }
    for (std::map<ir_tree_node *, std::set<int> >::iterator it2 =
        sorted_by_loop.begin(); it2 != sorted_by_loop.end(); it2++)
      to_return.push_back(it2->second);
  }
  return to_return;
}

void update_successors(int n,
                       int node_num[],
                       int cant_fuse_with[],
                       Graph<std::set<int>, bool> &g,
                       std::list<int> &work_list,
                       std::list<bool> &type_list,
                       std::vector<bool> types) {

  std::set<int> disconnect;
  for (Graph<std::set<int>, bool>::EdgeList::iterator i =
      g.vertex[n].second.begin(); i != g.vertex[n].second.end(); i++) {
    int m = i->first;

    if (node_num[m] != -1)
      throw chill::error::loop("Graph input for fusion has cycles not a DAG!!");

    std::vector<bool> check_ = g.getEdge(n, m);

    bool has_bad_edge_path = false;
    for (int i = 0; i < check_.size(); i++)
      if (!check_[i]) {
        has_bad_edge_path = true;
        break;
      }
    if (!types[m]) {
      cant_fuse_with[m] = std::max(cant_fuse_with[m], cant_fuse_with[n]);
    } else {
      if (has_bad_edge_path)
        cant_fuse_with[m] = std::max(cant_fuse_with[m], node_num[n]);
      else
        cant_fuse_with[m] = std::max(cant_fuse_with[m], cant_fuse_with[n]);
    }
    disconnect.insert(m);
  }


  for (std::set<int>::iterator i = disconnect.begin(); i != disconnect.end();
       i++) {
    g.disconnect(n, *i);

    bool no_incoming_edges = true;
    for (int j = 0; j < g.vertex.size(); j++)
      if (j != *i)
        if (g.hasEdge(j, *i)) {
          no_incoming_edges = false;
          break;
        }

    if (no_incoming_edges) {
      work_list.push_back(*i);
      type_list.push_back(types[*i]);
    }
  }
}


int Loop::getMinLexValue(std::set<int> stmts, int level) {

  int min;

  std::set<int>::iterator it = stmts.begin();
  min = getLexicalOrder(*it, level);

  for (; it != stmts.end(); it++) {
    int curr = getLexicalOrder(*it, level);
    if (curr < min)
      min = curr;
  }

  return min;
}


Graph<std::set<int>, bool> Loop::construct_induced_graph_at_level(
    std::vector<std::set<int> > s, DependenceGraph dep, int dep_dim) {
  Graph<std::set<int>, bool> g;

  for (int i = 0; i < s.size(); i++)
    g.insert(s[i]);

  for (int i = 0; i < s.size(); i++) {

    for (int j = i + 1; j < s.size(); j++) {
      bool has_true_edge_i_to_j = false;
      bool has_true_edge_j_to_i = false;
      bool is_connected_i_to_j = false;
      bool is_connected_j_to_i = false;
      for (std::set<int>::iterator ii = s[i].begin(); ii != s[i].end();
           ii++) {

        for (std::set<int>::iterator jj = s[j].begin();
             jj != s[j].end(); jj++) {

          std::vector<DependenceVector> dvs = dep.getEdge(*ii, *jj);
          for (int k = 0; k < dvs.size(); k++)
            if (dvs[k].is_control_dependence()
                || (dvs[k].is_data_dependence()
                    && dvs[k].has_been_carried_at(dep_dim))) {

              if (dvs[k].is_data_dependence()
                  && dvs[k].has_negative_been_carried_at(
                  dep_dim)) {
                //g.connect(i, j, false);
                is_connected_i_to_j = true;
                break;
              } else {
                //g.connect(i, j, true);

                has_true_edge_i_to_j = true;
                //break
              }
            }

          //if (is_connected)

          //    break;
          //        if (has_true_edge_i_to_j && !is_connected_i_to_j)
          //                g.connect(i, j, true);
          dvs = dep.getEdge(*jj, *ii);
          for (int k = 0; k < dvs.size(); k++)
            if (dvs[k].is_control_dependence()
                || (dvs[k].is_data_dependence()
                    && dvs[k].has_been_carried_at(dep_dim))) {

              if (is_connected_i_to_j || has_true_edge_i_to_j)
                throw chill::error::loop(
                    "Graph input for fusion has cycles not a DAG!!");

              if (dvs[k].is_data_dependence()
                  && dvs[k].has_negative_been_carried_at(
                  dep_dim)) {
                //g.connect(i, j, false);
                is_connected_j_to_i = true;
                break;
              } else {
                //g.connect(i, j, true);

                has_true_edge_j_to_i = true;
                //break;
              }
            }

          //    if (is_connected)
          //break;
          //    if (is_connected)
          //break;
        }

        //if (is_connected)
        //  break;
      }


      if (is_connected_i_to_j)
        g.connect(i, j, false);
      else if (has_true_edge_i_to_j)
        g.connect(i, j, true);

      if (is_connected_j_to_i)
        g.connect(j, i, false);
      else if (has_true_edge_j_to_i)
        g.connect(j, i, true);

    }
  }
  return g;
}


std::vector<std::set<int> > Loop::typed_fusion(Graph<std::set<int>, bool> g,
                                               std::vector<bool> &types) {

  bool roots[g.vertex.size()];

  for (int i = 0; i < g.vertex.size(); i++)
    roots[i] = true;

  for (int i = 0; i < g.vertex.size(); i++)
    for (int j = i + 1; j < g.vertex.size(); j++) {

      if (g.hasEdge(i, j))
        roots[j] = false;

      if (g.hasEdge(j, i))
        roots[i] = false;

    }

  std::list<int> work_list;
  std::list<bool> type_list;
  int cant_fuse_with[g.vertex.size()];
  int fused = 0;
  int lastfused = 0;
  int lastnum = 0;
  std::vector<std::set<int> > s;
  //Each Fused set's representative node

  int node_to_fused_nodes[g.vertex.size()];
  int node_num[g.vertex.size()];
  int next[g.vertex.size()];

  for (int i = 0; i < g.vertex.size(); i++) {
    if (roots[i] == true) {
      work_list.push_back(i);
      type_list.push_back(types[i]);
    }
    cant_fuse_with[i] = 0;
    node_to_fused_nodes[i] = 0;
    node_num[i] = -1;
    next[i] = 0;
  }


  // topological sort according to chun's permute algorithm
  //   std::vector<std::set<int> > s = g.topoSort();
  std::vector<std::set<int> > s2 = g.topoSort();
  if (work_list.empty() || (s2.size() != g.vertex.size())) {

    std::cout << s2.size() << "\t" << g.vertex.size() << std::endl;
    throw chill::error::loop("Input for fusion not a DAG!!");


  }
  int fused_nodes_counter = 0;
  while (!work_list.empty()) {
    int n = work_list.front();
    bool type = type_list.front();
    //int n_ = g.vertex[n].first;
    work_list.pop_front();
    type_list.pop_front();
    int node;
    /*if (cant_fuse_with[n] == 0)
      node = 0;
      else
      node = cant_fuse_with[n];
    */
    int p;
    if (type) {
      //if ((fused_nodes_counter != 0) && (node != fused_nodes_counter)) {
      if (cant_fuse_with[n] == 0)
        p = fused;
      else
        p = next[cant_fuse_with[n]];

      if (p != 0) {
        int rep_node = node_to_fused_nodes[p];
        node_num[n] = node_num[rep_node];

        try {
          update_successors(n, node_num, cant_fuse_with, g, work_list,
                            type_list, types);
        } catch (const chill::error::loop&e) {

          throw chill::error::loop(
              "statements cannot be fused together due to negative dependence");

        }
        for (std::set<int>::iterator it = g.vertex[n].first.begin();
             it != g.vertex[n].first.end(); it++)
          s[node_num[n] - 1].insert(*it);
      } else {
        //std::set<int> new_node;
        //new_node.insert(n_);
        s.push_back(g.vertex[n].first);
        lastnum = lastnum + 1;
        node_num[n] = lastnum;
        node_to_fused_nodes[node_num[n]] = n;

        if (lastfused == 0) {
          fused = lastnum;
          lastfused = fused;
        } else {
          next[lastfused] = lastnum;
          lastfused = lastnum;

        }

        try {
          update_successors(n, node_num, cant_fuse_with, g, work_list,
                            type_list, types);
        } catch (const chill::error::loop&e) {

          throw chill::error::loop(
              "statements cannot be fused together due to negative dependence");

        }
        fused_nodes_counter++;
      }

    } else {
      s.push_back(g.vertex[n].first);
      lastnum = lastnum + 1;
      node_num[n] = lastnum;
      node_to_fused_nodes[node_num[n]] = n;

      try {
        update_successors(n, node_num, cant_fuse_with, g, work_list,
                          type_list, types);
      } catch (const chill::error::loop&e) {

        throw chill::error::loop(
            "statements cannot be fused together due to negative dependence");

      }
      //fused_nodes_counter++;

    }

  }

  return s;
}


void Loop::setLexicalOrder(int dim, const std::set<int> &active,
                           int starting_order, std::vector<std::vector<std::string> > idxNames) {
  fprintf(stderr, "Loop::setLexicalOrder()  %d idxNames     active size %d  starting_order %d\n", idxNames.size(),
          active.size(), starting_order);
  if (active.size() == 0)
    return;

  for (int i = 0; i < idxNames.size(); i++) {
    std::vector<std::string> what = idxNames[i];
    for (int j = 0; j < what.size(); j++) {
      fprintf(stderr, "%2d %2d %s\n", i, j, what[j].c_str());
    }
  }

  // check for sanity of parameters
  if (dim < 0 || dim % 2 != 0)
    throw std::invalid_argument(
        "invalid constant loop level to set lexicographical order");
  std::vector<int> lex;
  int ref_stmt_num;
  for (std::set<int>::iterator i = active.begin(); i != active.end(); i++) {
    if ((*i) < 0 || (*i) >= stmt.size())
      throw std::invalid_argument(
          "invalid statement number " + to_string(*i));
    if (dim >= stmt[*i].xform.n_out())
      throw std::invalid_argument(
          "invalid constant loop level to set lexicographical order");
    if (i == active.begin()) {
      lex = getLexicalOrder(*i);
      ref_stmt_num = *i;
    } else {
      std::vector<int> lex2 = getLexicalOrder(*i);
      for (int j = 0; j < dim; j += 2)
        if (lex[j] != lex2[j])
          throw std::invalid_argument(
              "statements are not in the same sub loop nest");
    }
  }

  // separate statements by current loop level types
  int level = (dim + 2) / 2;
  std::map<std::pair<LoopLevelType, int>, std::set<int> > active_by_level_type;
  std::set<int> active_by_no_level;
  for (std::set<int>::iterator i = active.begin(); i != active.end(); i++) {
    if (level > stmt[*i].loop_level.size())
      active_by_no_level.insert(*i);
    else
      active_by_level_type[std::make_pair(
          stmt[*i].loop_level[level - 1].type,
          stmt[*i].loop_level[level - 1].payload)].insert(*i);
  }

  // further separate statements due to control dependences
  std::vector<std::set<int> > active_by_level_type_splitted;
  for (std::map<std::pair<LoopLevelType, int>, std::set<int> >::iterator i =
      active_by_level_type.begin(); i != active_by_level_type.end(); i++)
    active_by_level_type_splitted.push_back(i->second);
  for (std::set<int>::iterator i = active_by_no_level.begin();
       i != active_by_no_level.end(); i++)
    for (int j = active_by_level_type_splitted.size() - 1; j >= 0; j--) {
      std::set<int> controlled, not_controlled;
      for (std::set<int>::iterator k =
          active_by_level_type_splitted[j].begin();
           k != active_by_level_type_splitted[j].end(); k++) {
        std::vector<DependenceVector> dvs = dep.getEdge(*i, *k);
        bool is_controlled = false;
        for (int kk = 0; kk < dvs.size(); kk++)
          if (dvs[kk].type = DEP_CONTROL) {
            is_controlled = true;
            break;
          }
        if (is_controlled)
          controlled.insert(*k);
        else
          not_controlled.insert(*k);
      }
      if (controlled.size() != 0 && not_controlled.size() != 0) {
        active_by_level_type_splitted.erase(
            active_by_level_type_splitted.begin() + j);
        active_by_level_type_splitted.push_back(controlled);
        active_by_level_type_splitted.push_back(not_controlled);
      }
    }

  // set lexical order separating loops with different loop types first
  if (active_by_level_type_splitted.size() + active_by_no_level.size() > 1) {
    int dep_dim = get_last_dep_dim_before(ref_stmt_num, level) + 1;

    Graph<std::set<int>, Empty> g;
    for (std::vector<std::set<int> >::iterator i =
        active_by_level_type_splitted.begin();
         i != active_by_level_type_splitted.end(); i++)
      g.insert(*i);
    for (std::set<int>::iterator i = active_by_no_level.begin();
         i != active_by_no_level.end(); i++) {
      std::set<int> t;
      t.insert(*i);
      g.insert(t);
    }
    for (int i = 0; i < g.vertex.size(); i++)
      for (int j = i + 1; j < g.vertex.size(); j++) {
        bool connected = false;
        for (std::set<int>::iterator ii = g.vertex[i].first.begin();
             ii != g.vertex[i].first.end(); ii++) {
          for (std::set<int>::iterator jj = g.vertex[j].first.begin();
               jj != g.vertex[j].first.end(); jj++) {
            std::vector<DependenceVector> dvs = dep.getEdge(*ii,
                                                            *jj);
            for (int k = 0; k < dvs.size(); k++)
              if (dvs[k].is_control_dependence()
                  || (dvs[k].is_data_dependence()
                      && !dvs[k].has_been_carried_before(
                  dep_dim))) {
                g.connect(i, j);
                connected = true;
                break;
              }
            if (connected)
              break;
          }
          if (connected)
            break;
        }
        connected = false;
        for (std::set<int>::iterator ii = g.vertex[i].first.begin();
             ii != g.vertex[i].first.end(); ii++) {
          for (std::set<int>::iterator jj = g.vertex[j].first.begin();
               jj != g.vertex[j].first.end(); jj++) {
            std::vector<DependenceVector> dvs = dep.getEdge(*jj,
                                                            *ii);
            // find the sub loop nest specified by stmt_num and level,
            // only iteration space satisfiable statements returned.
            for (int k = 0; k < dvs.size(); k++)
              if (dvs[k].is_control_dependence()
                  || (dvs[k].is_data_dependence()
                      && !dvs[k].has_been_carried_before(
                  dep_dim))) {
                g.connect(j, i);
                connected = true;
                break;
              }
            if (connected)
              break;
          }
          if (connected)
            break;
        }
      }

    std::vector<std::set<int> > s = g.topoSort();
    if (s.size() != g.vertex.size())
      throw chill::error::loop(
          "cannot separate statements with different loop types at loop level "
          + to_string(level));

    // assign lexical order
    int order = starting_order;
    for (int i = 0; i < s.size(); i++) {
      std::set<int> &cur_scc = g.vertex[*(s[i].begin())].first;
      int sz = cur_scc.size();
      if (sz == 1) {
        int cur_stmt = *(cur_scc.begin());
        assign_const(stmt[cur_stmt].xform, dim, order);
        for (int j = dim + 2; j < stmt[cur_stmt].xform.n_out(); j += 2)
          assign_const(stmt[cur_stmt].xform, j, 0);
        order++;
      } else { // recurse ! 
        fprintf(stderr, "Loop:setLexicalOrder() recursing\n");
        setLexicalOrder(dim, cur_scc, order, idxNames);
        order += sz;
      }
    }
  } else { // set lexical order separating single iteration statements and loops

    std::set<int> true_singles;
    std::set<int> nonsingles;
    std::map<coef_t, std::set<int> > fake_singles;
    std::set<int> fake_singles_;

    // sort out statements that do not require loops
    for (std::set<int>::iterator i = active.begin(); i != active.end();
         i++) {
      Relation cur_IS = getNewIS(*i);
      if (is_single_iteration(cur_IS, dim + 1)) {
        bool is_all_single = true;
        for (int j = dim + 3; j < stmt[*i].xform.n_out(); j += 2)
          if (!is_single_iteration(cur_IS, j)) {
            is_all_single = false;
            break;
          }
        if (is_all_single)
          true_singles.insert(*i);
        else {
          fake_singles_.insert(*i);
          try {
            fake_singles[get_const(cur_IS, dim + 1, Set_Var)].insert(
                *i);
          } catch (const std::exception &e) {
            fake_singles[posInfinity].insert(*i);
          }
        }
      } else
        nonsingles.insert(*i);
    }


    // split nonsingles forcibly according to negative dependences present (loop unfusible)
    int dep_dim = get_dep_dim_of(ref_stmt_num, level);

    if (dim < stmt[ref_stmt_num].xform.n_out() - 1) {

      bool dummy_level_found = false;

      std::vector<std::set<int> > s;

      s = sort_by_same_loops(active, level);
      bool further_levels_exist = false;

      if (!idxNames.empty())
        if (level <= idxNames[ref_stmt_num].size())
          if (idxNames[ref_stmt_num][level - 1].length() == 0) {
            //  && s.size() == 1) {
            int order1 = 0;
            dummy_level_found = true;

            for (int i = level; i < idxNames[ref_stmt_num].size();
                 i++)
              if (idxNames[ref_stmt_num][i].length() > 0)
                further_levels_exist = true;

          }

      //if (!dummy_level_found) {

      if (s.size() > 1) {

        std::vector<bool> types;
        for (int i = 0; i < s.size(); i++)
          types.push_back(true);

        Graph<std::set<int>, bool> g = construct_induced_graph_at_level(
            s, dep, dep_dim);
        s = typed_fusion(g, types);
      }
      int order = starting_order;
      for (int i = 0; i < s.size(); i++) {

        for (std::set<int>::iterator it = s[i].begin();
             it != s[i].end(); it++) {
          assign_const(stmt[*it].xform, dim, order);
          stmt[*it].xform.simplify();
        }

        if ((dim + 2) <= (stmt[ref_stmt_num].xform.n_out() - 1)) {  // recurse ! 
          fprintf(stderr, "Loop:setLexicalOrder() recursing\n");
          setLexicalOrder(dim + 2, s[i], order, idxNames);
        }

        order++;
      }
      //}
      /*    else {
            
            int order1 = 0;
            int order = 0;
            for (std::set<int>::iterator i = active.begin();
            i != active.end(); i++) {
            if (!further_levels_exist)
            assign_const(stmt[*i].xform, dim, order1++);
            else
            assign_const(stmt[*i].xform, dim, order1);
            
            }
            
            if ((dim + 2) <= (stmt[ref_stmt_num].xform.n_out() - 1) && further_levels_exist)
            setLexicalOrder(dim + 2, active, order, idxNames);
            }
      */
    } else {
      int dummy_order = 0;
      for (std::set<int>::iterator i = active.begin(); i != active.end();
           i++) {
        assign_const(stmt[*i].xform, dim, dummy_order++);
        stmt[*i].xform.simplify();
      }
    }
    /*for (int i = 0; i < g2.vertex.size(); i++)
      for (int j = i+1; j < g2.vertex.size(); j++) {
      std::vector<DependenceVector> dvs = dep.getEdge(g2.vertex[i].first, g2.vertex[j].first);
      for (int k = 0; k < dvs.size(); k++)
      if (dvs[k].is_control_dependence() ||
      (dvs[k].is_data_dependence() && dvs[k].has_negative_been_carried_at(dep_dim))) {
      g2.connect(i, j);
      break;
      }
      dvs = dep.getEdge(g2.vertex[j].first, g2.vertex[i].first);
      for (int k = 0; k < dvs.size(); k++)
      if (dvs[k].is_control_dependence() ||
      (dvs[k].is_data_dependence() && dvs[k].has_negative_been_carried_at(dep_dim))) {
      g2.connect(j, i);
      break;
      }
      }
      
      std::vector<std::set<int> > s2 = g2.packed_topoSort();
      
      std::vector<std::set<int> > splitted_nonsingles;
      for (int i = 0; i < s2.size(); i++) {
      std::set<int> cur_scc;
      for (std::set<int>::iterator j = s2[i].begin(); j != s2[i].end(); j++)
      cur_scc.insert(g2.vertex[*j].first);
      splitted_nonsingles.push_back(cur_scc);
      }
    */
    //convert to dependence graph for grouped statements
    //dep_dim = get_last_dep_dim_before(ref_stmt_num, level) + 1;
    /*int order = 0;
      for (std::set<int>::iterator j = active.begin(); j != active.end();
      j++) {
      std::set<int> continuous;
      std::cout<< active.size()<<std::endl;
      while (nonsingles.find(*j) != nonsingles.end() && j != active.end()) {
      continuous.insert(*j);
      j++;
      }
      
      printf("continuous size is %d\n", continuous.size());
      
      
      
      if (continuous.size() > 0) {
      std::vector<std::set<int> > s = typed_fusion(continuous, dep,
      dep_dim);
      
      for (int i = 0; i < s.size(); i++) {
      for (std::set<int>::iterator l = s[i].begin();
      l != s[i].end(); l++) {
      assign_const(stmt[*l].xform, dim + 2, order);
      setLexicalOrder(dim + 2, s[i]);
      }
      order++;
      }
      }
      
      if (j != active.end()) {
      assign_const(stmt[*j].xform, dim + 2, order);
      
      for (int k = dim + 4; k < stmt[*j].xform.n_out(); k += 2)
      assign_const(stmt[*j].xform, k, 0);
      order++;
      }
      
      if( j == active.end())
      break;
      }
    */


    // assign lexical order
    /*int order = starting_order;
      for (int i = 0; i < s.size(); i++) {
      // translate each SCC into original statements
      std::set<int> cur_scc;
      for (std::set<int>::iterator j = s[i].begin(); j != s[i].end(); j++)
      copy(s[i].begin(), s[i].end(),
      inserter(cur_scc, cur_scc.begin()));
      
      // now assign the constant
      for (std::set<int>::iterator j = cur_scc.begin();
      j != cur_scc.end(); j++)
      assign_const(stmt[*j].xform, dim, order);
      
      if (cur_scc.size() > 1)
      setLexicalOrder(dim + 2, cur_scc);
      else if (cur_scc.size() == 1) {
      int cur_stmt = *(cur_scc.begin());
      for (int j = dim + 2; j < stmt[cur_stmt].xform.n_out(); j += 2)
      assign_const(stmt[cur_stmt].xform, j, 0);
      }
      
      if (cur_scc.size() > 0)
      order++;
      }
    */
  }

  fprintf(stderr, "LEAVING Loop::setLexicalOrder()  %d idxNames\n", idxNames.size());
  for (int i = 0; i < idxNames.size(); i++) {
    std::vector<std::string> what = idxNames[i];
    for (int j = 0; j < what.size(); j++) {
      fprintf(stderr, "%2d %2d %s\n", i, j, what[j].c_str());
    }
  }
}


void Loop::apply_xform() {
  std::set<int> active;
  for (int i = 0; i < stmt.size(); i++)
    active.insert(i);
  apply_xform(active);
}

void Loop::apply_xform(int stmt_num) {
  fprintf(stderr, "apply_xform( %d )\n", stmt_num);
  std::set<int> active;
  active.insert(stmt_num);
  apply_xform(active);
}

void Loop::apply_xform(std::set<int> &active) {
  int max_n = 0;

  omega::CG_outputBuilder *ocg = ir->builder();
  for (std::set<int>::iterator i = active.begin(); i != active.end(); i++) {
    int n = stmt[*i].loop_level.size();
    if (n > max_n)
      max_n = n;

    std::vector<int> lex = getLexicalOrder(*i);

    omega::Relation mapping(2 * n + 1, n);
    omega::F_And *f_root = mapping.add_and();
    for (int j = 1; j <= n; j++) {
      omega::EQ_Handle h = f_root->add_EQ();
      h.update_coef(mapping.output_var(j), 1);
      h.update_coef(mapping.input_var(2 * j), -1);
    }
    mapping = omega::Composition(mapping, stmt[*i].xform);
    mapping.simplify();

    // match omega input/output variables to variable names in the code
    for (int j = 1; j <= stmt[*i].IS.n_set(); j++)
      mapping.name_input_var(j, stmt[*i].IS.set_var(j)->name());
    for (int j = 1; j <= n; j++)
      mapping.name_output_var(j,
                              tmp_loop_var_name_prefix
                              + omega::to_string(
                                  tmp_loop_var_name_counter + j - 1));
    mapping.setup_names();
    omega::Relation known = Extend_Set(copy(this->known),
                                       mapping.n_out() - this->known.n_set());
    omega::CG_outputBuilder *ocgr = ir->builder();
    //this is probably CG_chillBuilder;
    omega::CG_stringBuilder *ocgs = new omega::CG_stringBuilder;
    if (uninterpreted_symbols[*i].size() == 0) {


      std::set<std::string> globals;

      for (omega::DNF_Iterator di(stmt[*i].IS.query_DNF()); di; di++) {

        for (omega::Constraint_Iterator e(*di); e; e++) {
          for (omega::Constr_Vars_Iter cvi(*e); cvi; cvi++) {
            omega::Variable_ID v = cvi.curr_var();
            if (v->kind() == omega::Global_Var
                && v->get_global_var()->arity() > 0
                && globals.find(v->name()) == globals.end()) {
              omega::Global_Var_ID g = v->get_global_var();
              globals.insert(v->name());
              std::vector<omega::CG_outputRepr *> reprs;
              std::vector<omega::CG_outputRepr *> reprs2;

              for (int l = 1; l <= g->arity(); l++) {
                omega::CG_outputRepr *temp = ocgr->CreateIdent(
                    stmt[*i].IS.set_var(l)->name());
                omega::CG_outputRepr *temp2 = ocgs->CreateIdent(
                    stmt[*i].IS.set_var(l)->name());

                reprs.push_back(temp);
                reprs2.push_back(temp2);
              }
              uninterpreted_symbols[*i].insert(
                  std::pair<std::string,
                      std::vector<omega::CG_outputRepr *> >(
                      (const char*)(v->get_global_var()->base_name()),
                      reprs));
              uninterpreted_symbols_stringrepr[*i].insert(
                  std::pair<std::string,
                      std::vector<omega::CG_outputRepr *> >(
                      (const char*)(v->get_global_var()->base_name()),
                      reprs2));
            }
          }
        }
      }
    }

    std::vector<std::string> loop_vars;
    for (int j = 1; j <= stmt[*i].IS.n_set(); j++) {
      loop_vars.push_back(stmt[*i].IS.set_var(j)->name());
    }
    std::vector<CG_outputRepr *> subs = output_substitutions(ocg,
                                                             Inverse(copy(mapping)),
                                                             std::vector<std::pair<CG_outputRepr *, int> >(
                                                                 mapping.n_out(),
                                                                 std::make_pair(
                                                                     static_cast<CG_outputRepr *>(NULL), 0)),
                                                             uninterpreted_symbols[*i]);

    std::vector<CG_outputRepr *> subs2;
    for (int l = 0; l < subs.size(); l++)
      subs2.push_back(subs[l]->clone());

    int count = 0;
    for (std::map<std::string, std::vector<CG_outputRepr *> >::iterator it =
        uninterpreted_symbols[*i].begin();
         it != uninterpreted_symbols[*i].end(); it++) {
      std::vector<CG_outputRepr *> reprs_ = it->second;
      std::vector<CG_outputRepr *> reprs_2;
      for (int k = 0; k < reprs_.size(); k++) {
        std::vector<CG_outputRepr *> subs;
        for (int l = 0; l < subs2.size(); l++) {
          subs.push_back(subs2[l]->clone());
        }
        CG_outputRepr *c = reprs_[k]->clone();
        CG_outputRepr *s = ocgr->CreateSubstitutedStmt(0, c,
                                                       loop_vars, subs, true);
        reprs_2.push_back(s);
      }
      it->second = reprs_2;
      count++;
    }

    std::vector<CG_outputRepr *> subs3 = output_substitutions(
        ocgs, Inverse(copy(mapping)),
        std::vector<std::pair<CG_outputRepr *, int> >(
            mapping.n_out(),
            std::make_pair(
                static_cast<CG_outputRepr *>(NULL), 0)),
        uninterpreted_symbols_stringrepr[*i]);

    for (std::map<std::string, std::vector<CG_outputRepr *> >::iterator it =
        uninterpreted_symbols_stringrepr[*i].begin();
         it != uninterpreted_symbols_stringrepr[*i].end(); it++) {

      std::vector<CG_outputRepr *> reprs_ = it->second;
      std::vector<CG_outputRepr *> reprs_2;
      for (int k = 0; k < reprs_.size(); k++) {
        std::vector<CG_outputRepr *> subs;
        /*  for (int l = 0; l < subs3.size(); l++)
            subs.push_back(subs3[l]->clone());
            reprs_2.push_back(
            ocgs->CreateSubstitutedStmt(0, reprs_[k]->clone(),
            loop_vars, subs));
        */
        reprs_2.push_back(subs3[k]->clone());
      }

      it->second = reprs_2;

    }
    stmt[*i].code = ocg->CreateSubstitutedStmt(0, stmt[*i].code, loop_vars,
                                               subs);
    stmt[*i].IS = omega::Range(Restrict_Domain(mapping, stmt[*i].IS));
    stmt[*i].IS.simplify();

    // replace original transformation relation with straight 1-1 mapping
    mapping = Relation(n, 2 * n + 1);
    f_root = mapping.add_and();
    for (int j = 1; j <= n; j++) {
      EQ_Handle h = f_root->add_EQ();
      h.update_coef(mapping.output_var(2 * j), 1);
      h.update_coef(mapping.input_var(j), -1);
    }
    for (int j = 1; j <= 2 * n + 1; j += 2) {
      EQ_Handle h = f_root->add_EQ();
      h.update_coef(mapping.output_var(j), 1);
      h.update_const(-lex[j - 1]);
    }
    stmt[*i].xform = mapping;
  }
  tmp_loop_var_name_counter += max_n;
}


void Loop::addKnown(const Relation &cond) {

  // invalidate saved codegen computation
  delete last_compute_cgr_;
  last_compute_cgr_ = NULL;
  delete last_compute_cg_;
  last_compute_cg_ = NULL;
  CHILL_DEBUG_PRINT("Loop::addKnown(), SETTING last_compute_cg_ = NULL\n");

  int n1 = this->known.n_set();

  Relation r = copy(cond);
  int n2 = r.n_set();

  if (n1 < n2)
    this->known = Extend_Set(this->known, n2 - n1);
  else if (n1 > n2)
    r = Extend_Set(r, n1 - n2);

  this->known = Intersection(this->known, r);
}

void Loop::removeDependence(int stmt_num_from, int stmt_num_to) {
  // check for sanity of parameters
  if (stmt_num_from >= stmt.size())
    throw std::invalid_argument(
        "invalid statement number " + to_string(stmt_num_from));
  if (stmt_num_to >= stmt.size())
    throw std::invalid_argument(
        "invalid statement number " + to_string(stmt_num_to));

  dep.disconnect(stmt_num_from, stmt_num_to);
}

void Loop::dump() const {
  for (int i = 0; i < stmt.size(); i++) {
    std::vector<int> lex = getLexicalOrder(i);
    std::cout << "s" << i + 1 << ": ";
    for (int j = 0; j < stmt[i].loop_level.size(); j++) {
      if (2 * j < lex.size())
        std::cout << lex[2 * j];
      switch (stmt[i].loop_level[j].type) {
        case LoopLevelOriginal:
          std::cout << "(dim:" << stmt[i].loop_level[j].payload << ")";
          break;
        case LoopLevelTile:
          std::cout << "(tile:" << stmt[i].loop_level[j].payload << ")";
          break;
        default:
          std::cout << "(unknown)";
      }
      std::cout << ' ';
    }
    for (int j = 2 * stmt[i].loop_level.size(); j < lex.size(); j += 2) {
      std::cout << lex[j];
      if (j != lex.size() - 1)
        std::cout << ' ';
    }
    std::cout << std::endl;
  }
}

bool Loop::nonsingular(const std::vector<std::vector<int> > &T) {
  if (stmt.size() == 0)
    return true;

  // check for sanity of parameters
  for (int i = 0; i < stmt.size(); i++) {
    if (stmt[i].loop_level.size() != num_dep_dim)
      throw std::invalid_argument(
          "nonsingular loop transformations must be applied to original perfect loop nest");
    for (int j = 0; j < stmt[i].loop_level.size(); j++)
      if (stmt[i].loop_level[j].type != LoopLevelOriginal)
        throw std::invalid_argument(
            "nonsingular loop transformations must be applied to original perfect loop nest");
  }
  if (T.size() != num_dep_dim)
    throw std::invalid_argument("invalid transformation matrix");
  for (int i = 0; i < stmt.size(); i++)
    if (T[i].size() != num_dep_dim + 1 && T[i].size() != num_dep_dim)
      throw std::invalid_argument("invalid transformation matrix");
  // invalidate saved codegen computation
  delete last_compute_cgr_;
  last_compute_cgr_ = NULL;
  delete last_compute_cg_;
  last_compute_cg_ = NULL;
  fprintf(stderr, "Loop::nonsingular(), SETTING last_compute_cg_ = NULL\n");

  // build relation from matrix
  Relation mapping(2 * num_dep_dim + 1, 2 * num_dep_dim + 1);
  F_And *f_root = mapping.add_and();
  for (int i = 0; i < num_dep_dim; i++) {
    EQ_Handle h = f_root->add_EQ();
    h.update_coef(mapping.output_var(2 * (i + 1)), -1);
    for (int j = 0; j < num_dep_dim; j++)
      if (T[i][j] != 0)
        h.update_coef(mapping.input_var(2 * (j + 1)), T[i][j]);
    if (T[i].size() == num_dep_dim + 1)
      h.update_const(T[i][num_dep_dim]);
  }
  for (int i = 1; i <= 2 * num_dep_dim + 1; i += 2) {
    EQ_Handle h = f_root->add_EQ();
    h.update_coef(mapping.output_var(i), -1);
    h.update_coef(mapping.input_var(i), 1);
  }

  // update transformation relations
  for (int i = 0; i < stmt.size(); i++)
    stmt[i].xform = Composition(copy(mapping), stmt[i].xform);

  // update dependence graph
  for (int i = 0; i < dep.vertex.size(); i++)
    for (DependenceGraph::EdgeList::iterator j =
        dep.vertex[i].second.begin(); j != dep.vertex[i].second.end();
         j++) {
      std::vector<DependenceVector> dvs = j->second;
      for (int k = 0; k < dvs.size(); k++) {
        DependenceVector &dv = dvs[k];
        switch (dv.type) {
          case DEP_W2R:
          case DEP_R2W:
          case DEP_W2W:
          case DEP_R2R: {
            std::vector<coef_t> lbounds(num_dep_dim), ubounds(
                num_dep_dim);
            for (int p = 0; p < num_dep_dim; p++) {
              coef_t lb = 0;
              coef_t ub = 0;
              for (int q = 0; q < num_dep_dim; q++) {
                if (T[p][q] > 0) {
                  if (lb == -posInfinity
                      || dv.lbounds[q] == -posInfinity)
                    lb = -posInfinity;
                  else
                    lb += T[p][q] * dv.lbounds[q];
                  if (ub == posInfinity
                      || dv.ubounds[q] == posInfinity)
                    ub = posInfinity;
                  else
                    ub += T[p][q] * dv.ubounds[q];
                } else if (T[p][q] < 0) {
                  if (lb == -posInfinity
                      || dv.ubounds[q] == posInfinity)
                    lb = -posInfinity;
                  else
                    lb += T[p][q] * dv.ubounds[q];
                  if (ub == posInfinity
                      || dv.lbounds[q] == -posInfinity)
                    ub = posInfinity;
                  else
                    ub += T[p][q] * dv.lbounds[q];
                }
              }
              if (T[p].size() == num_dep_dim + 1) {
                if (lb != -posInfinity)
                  lb += T[p][num_dep_dim];
                if (ub != posInfinity)
                  ub += T[p][num_dep_dim];
              }
              lbounds[p] = lb;
              ubounds[p] = ub;
            }
            dv.lbounds = lbounds;
            dv.ubounds = ubounds;

            break;
          }
          default:;
        }
      }
      j->second = dvs;
    }

  // set constant loop values
  std::set<int> active;
  for (int i = 0; i < stmt.size(); i++)
    active.insert(i);
  setLexicalOrder(0, active);

  return true;
}


bool Loop::is_dependence_valid_based_on_lex_order(int i, int j,
                                                  const DependenceVector &dv, bool before) {
  std::vector<int> lex_i = getLexicalOrder(i);
  std::vector<int> lex_j = getLexicalOrder(j);
  int last_dim;
  if (!dv.is_scalar_dependence) {
    for (last_dim = 0;
         last_dim < lex_i.size() && (lex_i[last_dim] == lex_j[last_dim]);
         last_dim++);
    last_dim = last_dim / 2;
    if (last_dim == 0)
      return true;

    for (int i = 0; i < last_dim; i++) {
      if (dv.lbounds[i] > 0)
        return true;
      else if (dv.lbounds[i] < 0)
        return false;
    }
  }
  if (before)
    return true;

  return false;

}

// Manu:: reduction operation

void Loop::scalar_expand(int stmt_num, const std::vector<int> &levels,
                         std::string arrName, int memory_type, int padding_alignment,
                         int assign_then_accumulate, int padding_stride) {

  //std::cout << "In scalar_expand function: " << stmt_num << ", " << arrName << "\n";
  //std::cout.flush(); 

  //fprintf(stderr, "\n%d statements\n", stmt.size());
  //for (int i=0; i<stmt.size(); i++) { 
  //  fprintf(stderr, "%2d   ", i); 
  //  ((CG_chillRepr *)stmt[i].code)->Dump();
  //} 
  //fprintf(stderr, "\n"); 

  // check for sanity of parameters
  bool found_non_constant_size_dimension = false;

  if (stmt_num < 0 || stmt_num >= stmt.size())
    throw std::invalid_argument(
        "invalid statement number " + to_string(stmt_num));
  //Anand: adding check for privatized levels
  //if (arrName != "RHS")
  //  throw std::invalid_argument(
  //      "invalid 3rd argument: only 'RHS' supported " + arrName);
  for (int i = 0; i < levels.size(); i++) {
    if (levels[i] <= 0 || levels[i] > stmt[stmt_num].loop_level.size())
      throw std::invalid_argument(
          "1invalid loop level " + to_string(levels[i]));

    if (i > 0) {
      if (levels[i] < levels[i - 1])
        throw std::invalid_argument(
            "loop levels must be in ascending order");
    }
  }
  //end --adding check for privatized levels

  delete last_compute_cgr_;
  last_compute_cgr_ = NULL;
  delete last_compute_cg_;
  last_compute_cg_ = NULL;
  fprintf(stderr, "Loop::scalar_expand(), SETTING last_compute_cg_ = NULL\n");

  fprintf(stderr, "\nloop.cc finding array accesses in stmt %d of the code\n", stmt_num);
  std::vector<IR_ArrayRef *> access = ir->FindArrayRef(stmt[stmt_num].code);
  fprintf(stderr, "loop.cc L2726  %d access\n", access.size());

  IR_ArraySymbol *sym = NULL;
  fprintf(stderr, "arrName %s\n", arrName.c_str());
  if (arrName == "RHS") {
    fprintf(stderr, "sym RHS\n");
    sym = access[0]->symbol();
  } else {
    fprintf(stderr, "looking for array %s in access\n", arrName.c_str());
    for (int k = 0; k < access.size(); k++) { // BUH

      //fprintf(stderr, "access[%d] = %s ", k, access[k]->getTypeString()); access[k]->print(0,stderr); fprintf(stderr, "\n"); 

      std::string name = access[k]->symbol()->name();
      //fprintf(stderr, "comparing %s to %s\n", name.c_str(), arrName.c_str()); 

      if (access[k]->symbol()->name() == arrName) {
        fprintf(stderr, "found it   sym access[ k=%d ]\n", k);
        sym = access[k]->symbol();
      }
    }
  }
  if (!sym) fprintf(stderr, "DIDN'T FIND IT\n");
  fprintf(stderr, "sym %p\n", sym);

  // collect array references by name
  std::vector<int> lex = getLexicalOrder(stmt_num);
  int dim = 2 * levels[levels.size() - 1] - 1;
  std::set<int> same_loop = getStatements(lex, dim - 1);

  //Anand: shifting this down
  //  assign_const(stmt[newStmt_num].xform, 2*level+1, 1);

  //  std::cout << " before temp array name \n ";
  // create a temporary variable
  IR_Symbol *tmp_sym;

  // get the loop upperbound, that would be the size of the temp array.
  omega::coef_t lb[levels.size()], ub[levels.size()], size[levels.size()];

  //Anand Adding apply xform so that tiled loop bounds are reflected
  fprintf(stderr, "Adding apply xform so that tiled loop bounds are reflected\n");
  apply_xform(same_loop);
  fprintf(stderr, "loop.cc, back from apply_xform()\n");

  //Anand commenting out the folowing 4 lines
  /*  copy(stmt[stmt_num].IS).query_variable_bounds(
      copy(stmt[stmt_num].IS).set_var(level), lb, ub);
      std::cout << "Upper Bound = " << ub << "\n";
      std::cout << "lower Bound = " << lb << "\n";
  */
  // testing testing -- Manu ////////////////////////////////////////////////
  /*
  // int n_dim = sym->n_dim();
  // std::cout << "------- n_dim ----------- " << n_dim << "\n";
  std::pair<EQ_Handle, Variable_ID> result = find_simplest_stride(stmt[stmt_num].IS, stmt[stmt_num].IS.set_var(level));
  omega::coef_t  index_stride;
  if (result.second != NULL) {
  index_stride = abs(result.first.get_coef(result.second))/gcd(abs(result.first.get_coef(result.second)), abs(result.first.get_coef(stmt[stmt_num].IS.set_var(level))));
  std::cout << "simplest_stride :: " << index_stride << ", " << result.first.get_coef(result.second) << ", " << result.first.get_coef(stmt[stmt_num].IS.set_var(level))<< "\n";
  }
  Relation bound;
  // bound = get_loop_bound(stmt[stmt_num].IS, level);
  bound = SimpleHull(stmt[stmt_num].IS,true, true);
  bound.print();
  
  bound = copy(stmt[stmt_num].IS);
  for (int i = 1; i < level; i++) {
  bound = Project(bound, i, Set_Var);
  std::cout << "-------------------------------\n";
  bound.print();
  }
  
  bound.simplify();
  bound.print();
  // bound = get_loop_bound(bound, level);
  
  copy(bound).query_variable_bounds(copy(bound).set_var(level), lb, ub);
  std::cout << "Upper Bound = " << ub << "\n";
  std::cout << "lower Bound = " << lb << "\n";
  
  result = find_simplest_stride(bound, bound.set_var(level));
  if (result.second != NULL)
  index_stride = abs(result.first.get_coef(result.second))/gcd(abs(result.first.get_coef(result.second)), abs(result.first.get_coef(bound.set_var(level))));
  else
  index_stride = 1;
  std::cout << "simplest_stride 11:: " << index_stride << "\n";
  */
  ////////////////////////////////////////////////////////////////////////////////
  ///////////////////////////// copied datacopy code here /////////////////////////////////////////////

  //std::cout << "In scalar_expand function 2: " << stmt_num << ", " << arrName << "\n";
  //std::cout.flush(); 

  //fprintf(stderr, "\n%d statements\n", stmt.size());
  //for (int i=0; i<stmt.size(); i++) { 
  //  fprintf(stderr, "%2d   ", i); 
  //  ((CG_chillRepr *)stmt[i].code)->Dump();
  //} 
  //fprintf(stderr, "\n"); 



  int n_dim = levels.size();
  Relation copy_is = copy(stmt[stmt_num].IS);
  // extract temporary array information
  CG_outputBuilder *ocg1 = ir->builder();
  std::vector<CG_outputRepr *> index_lb(n_dim); // initialized to NULL
  std::vector<coef_t> index_stride(n_dim);
  std::vector<bool> is_index_eq(n_dim, false);
  std::vector<std::pair<int, CG_outputRepr *> > index_sz(0);
  Relation reduced_copy_is = copy(copy_is);
  std::vector<CG_outputRepr *> size_repr;
  std::vector<int> size_int;
  Relation xform = copy(stmt[stmt_num].xform);
  for (int i = 0; i < n_dim; i++) {

    dim = 2 * levels[i] - 1;
    //Anand: Commenting out the lines below: not required
    //    if (i != 0)
    //    reduced_copy_is = Project(reduced_copy_is, level - 1 + i, Set_Var);
    Relation bound = get_loop_bound(copy(reduced_copy_is), levels[i] - 1);

    // extract stride
    std::pair<EQ_Handle, Variable_ID> result = find_simplest_stride(bound,
                                                                    bound.set_var(levels[i]));
    if (result.second != NULL)
      index_stride[i] = abs(result.first.get_coef(result.second))
                        / gcd(abs(result.first.get_coef(result.second)),
                              abs(
                                  result.first.get_coef(
                                      bound.set_var(levels[i]))));
    else
      index_stride[i] = 1;
    //  std::cout << "simplest_stride 11:: " << index_stride[i] << "\n";

    // check if this array index requires loop
    Conjunct *c = bound.query_DNF()->single_conjunct();
    for (EQ_Iterator ei(c->EQs()); ei; ei++) {
      if ((*ei).has_wildcards())
        continue;

      int coef = (*ei).get_coef(bound.set_var(levels[i]));
      if (coef != 0) {
        int sign = 1;
        if (coef < 0) {
          coef = -coef;
          sign = -1;
        }

        CG_outputRepr *op = NULL;
        for (Constr_Vars_Iter ci(*ei); ci; ci++) {
          switch ((*ci).var->kind()) {
            case Input_Var: {
              if ((*ci).var != bound.set_var(levels[i]))
                if ((*ci).coef * sign == 1)
                  op = ocg1->CreateMinus(op,
                                         ocg1->CreateIdent((*ci).var->name()));
                else if ((*ci).coef * sign == -1)
                  op = ocg1->CreatePlus(op,
                                        ocg1->CreateIdent((*ci).var->name()));
                else if ((*ci).coef * sign > 1) {
                  op = ocg1->CreateMinus(op,
                                         ocg1->CreateTimes(
                                             ocg1->CreateInt(
                                                 abs((*ci).coef)),
                                             ocg1->CreateIdent(
                                                 (*ci).var->name())));
                } else
                  // (*ci).coef*sign < -1
                  op = ocg1->CreatePlus(op,
                                        ocg1->CreateTimes(
                                            ocg1->CreateInt(
                                                abs((*ci).coef)),
                                            ocg1->CreateIdent(
                                                (*ci).var->name())));
              break;
            }
            case Global_Var: {
              Global_Var_ID g = (*ci).var->get_global_var();
              if ((*ci).coef * sign == 1)
                op = ocg1->CreateMinus(op,
                                       ocg1->CreateIdent(g->base_name()));
              else if ((*ci).coef * sign == -1)
                op = ocg1->CreatePlus(op,
                                      ocg1->CreateIdent(g->base_name()));
              else if ((*ci).coef * sign > 1)
                op = ocg1->CreateMinus(op,
                                       ocg1->CreateTimes(
                                           ocg1->CreateInt(abs((*ci).coef)),
                                           ocg1->CreateIdent(g->base_name())));
              else
                // (*ci).coef*sign < -1
                op = ocg1->CreatePlus(op,
                                      ocg1->CreateTimes(
                                          ocg1->CreateInt(abs((*ci).coef)),
                                          ocg1->CreateIdent(g->base_name())));
              break;
            }
            default:
              throw chill::error::loop("unsupported array index expression");
          }
        }
        if ((*ei).get_const() != 0)
          op = ocg1->CreatePlus(op,
                                ocg1->CreateInt(-sign * ((*ei).get_const())));
        if (coef != 1)
          op = ocg1->CreateIntegerFloor(op, ocg1->CreateInt(coef));

        index_lb[i] = op;
        is_index_eq[i] = true;
        break;
      }
    }
    if (is_index_eq[i])
      continue;

    // separate lower and upper bounds
    std::vector<GEQ_Handle> lb_list, ub_list;
    std::set<Variable_ID> excluded_floor_vars;
    excluded_floor_vars.insert(bound.set_var(levels[i]));
    for (GEQ_Iterator gi(c->GEQs()); gi; gi++) {
      int coef = (*gi).get_coef(bound.set_var(levels[i]));
      if (coef != 0 && (*gi).has_wildcards()) {
        bool clean_bound = true;
        GEQ_Handle h;
        for (Constr_Vars_Iter cvi(*gi, true); gi; gi++)
          if (!find_floor_definition(bound, (*cvi).var,
                                     excluded_floor_vars).first) {
            clean_bound = false;
            break;
          } else
            h = find_floor_definition(bound, (*cvi).var,
                                      excluded_floor_vars).second;

        if (!clean_bound)
          continue;
        else {
          if (coef > 0)
            lb_list.push_back(h);
          else if (coef < 0)
            ub_list.push_back(h);
          continue;
        }

      }

      if (coef > 0)
        lb_list.push_back(*gi);
      else if (coef < 0)
        ub_list.push_back(*gi);
    }
    if (lb_list.size() == 0 || ub_list.size() == 0)
      throw chill::error::loop("failed to calcuate array footprint size");

    // build lower bound representation
    std::vector<CG_outputRepr *> lb_repr_list;
    /*     for (int j = 0; j < lb_list.size(); j++){
           if(this->known.n_set() == 0)
           lb_repr_list.push_back(output_lower_bound_repr(ocg1, lb_list[j], bound.set_var(level-1+i+1), result.first, result.second, bound, Relation::True(bound.n_set()), std::vector<std::pair<CG_outputRepr *, int> >(bound.n_set(), std::make_pair(static_cast<CG_outputRepr *>(NULL), 0))));
           else
           lb_repr_list.push_back(output_lower_bound_repr(ocg1, lb_list[j], bound.set_var(level-1+i+1), result.first, result.second, bound, this->known, std::vector<std::pair<CG_outputRepr *, int> >(bound.n_set(), std::make_pair(static_cast<CG_outputRepr *>(NULL), 0))));
           
           }
    */
    if (lb_repr_list.size() > 1)
      index_lb[i] = ocg1->CreateInvoke("max", lb_repr_list);
    else if (lb_repr_list.size() == 1)
      index_lb[i] = lb_repr_list[0];

    // build temporary array size representation
    {
      Relation cal(copy_is.n_set(), 1);
      F_And *f_root = cal.add_and();
      for (int j = 0; j < ub_list.size(); j++)
        for (int k = 0; k < lb_list.size(); k++) {
          GEQ_Handle h = f_root->add_GEQ();

          for (Constr_Vars_Iter ci(ub_list[j]); ci; ci++) {
            switch ((*ci).var->kind()) {
              case Input_Var: {
                int pos = (*ci).var->get_position();
                h.update_coef(cal.input_var(pos), (*ci).coef);
                break;
              }
              case Global_Var: {
                Global_Var_ID g = (*ci).var->get_global_var();
                Variable_ID v;
                if (g->arity() == 0)
                  v = cal.get_local(g);
                else
                  v = cal.get_local(g, (*ci).var->function_of());
                h.update_coef(v, (*ci).coef);
                break;
              }
              default:
                throw chill::error::loop(
                    "cannot calculate temporay array size statically");
            }
          }
          h.update_const(ub_list[j].get_const());

          for (Constr_Vars_Iter ci(lb_list[k]); ci; ci++) {
            switch ((*ci).var->kind()) {
              case Input_Var: {
                int pos = (*ci).var->get_position();
                h.update_coef(cal.input_var(pos), (*ci).coef);
                break;
              }
              case Global_Var: {
                Global_Var_ID g = (*ci).var->get_global_var();
                Variable_ID v;
                if (g->arity() == 0)
                  v = cal.get_local(g);
                else
                  v = cal.get_local(g, (*ci).var->function_of());
                h.update_coef(v, (*ci).coef);
                break;
              }
              default:
                throw chill::error::loop(
                    "cannot calculate temporay array size statically");
            }
          }
          h.update_const(lb_list[k].get_const());

          h.update_const(1);
          h.update_coef(cal.output_var(1), -1);
        }

      cal = Restrict_Domain(cal, copy(copy_is));
      for (int j = 1; j <= cal.n_inp(); j++) {
        cal = Project(cal, j, Input_Var);
      }
      cal.simplify();

      // pad temporary array size
      // TODO: for variable array size, create padding formula
      //int padding_stride = 0;
      Conjunct *c = cal.query_DNF()->single_conjunct();
      bool is_index_bound_const = false;
      if (padding_stride != 0 && i == n_dim - 1) {
        //size = (size + index_stride[i] - 1) / index_stride[i];
        size_repr.push_back(ocg1->CreateInt(padding_stride));
      } else {
        for (GEQ_Iterator gi(c->GEQs()); gi && !is_index_bound_const;
             gi++)
          if ((*gi).is_const(cal.output_var(1))) {
            coef_t size = (*gi).get_const()
                          / (-(*gi).get_coef(cal.output_var(1)));

            if (padding_alignment > 1 && i == n_dim - 1) { // align to boundary for data packing
              int residue = size % padding_alignment;
              if (residue)
                size = size + padding_alignment - residue;
            }

            index_sz.push_back(
                std::make_pair(i, ocg1->CreateInt(size)));
            is_index_bound_const = true;
            size_int.push_back(size);
            size_repr.push_back(ocg1->CreateInt(size));

            //  std::cout << "============================== size :: "
            //      << size << "\n";

          }

        if (!is_index_bound_const) {

          found_non_constant_size_dimension = true;
          Conjunct *c = bound.query_DNF()->single_conjunct();
          for (GEQ_Iterator gi(c->GEQs());
               gi && !is_index_bound_const; gi++) {
            int coef = (*gi).get_coef(bound.set_var(levels[i]));
            if (coef < 0) {

              size_repr.push_back(
                  ocg1->CreatePlus(
                      output_upper_bound_repr(ocg1, *gi,
                                              bound.set_var(levels[i]),
                                              bound,
                                              std::vector<
                                                  std::pair<
                                                      CG_outputRepr *,
                                                      int> >(
                                                  bound.n_set(),
                                                  std::make_pair(
                                                      static_cast<CG_outputRepr *>(NULL),
                                                      0)),
                                              uninterpreted_symbols[stmt_num]),
                      ocg1->CreateInt(1)));

              /*CG_outputRepr *op = NULL;
                for (Constr_Vars_Iter ci(*gi); ci; ci++) {
                if ((*ci).var != cal.output_var(1)) {
                switch ((*ci).var->kind()) {
                case Global_Var: {
                Global_Var_ID g =
                (*ci).var->get_global_var();
                if ((*ci).coef == 1)
                op = ocg1->CreatePlus(op,
                ocg1->CreateIdent(
                g->base_name()));
                else if ((*ci).coef == -1)
                op = ocg1->CreateMinus(op,
                ocg1->CreateIdent(
                g->base_name()));
                else if ((*ci).coef > 1)
                op =
                ocg1->CreatePlus(op,
                ocg1->CreateTimes(
                ocg1->CreateInt(
                (*ci).coef),
                ocg1->CreateIdent(
                g->base_name())));
                else
                // (*ci).coef < -1
                op =
                ocg1->CreateMinus(op,
                ocg1->CreateTimes(
                ocg1->CreateInt(
                -(*ci).coef),
                ocg1->CreateIdent(
                g->base_name())));
                break;
                }
                default:
                throw chill::error::loop(
                "failed to generate array index bound code");
                }
                }
                }
                int c = (*gi).get_const();
                if (c > 0)
                op = ocg1->CreatePlus(op, ocg1->CreateInt(c));
                else if (c < 0)
                op = ocg1->CreateMinus(op, ocg1->CreateInt(-c));
              */
              /*            if (padding_stride != 0) {
                            if (i == fastest_changing_dimension) {
                            coef_t g = gcd(index_stride[i], static_cast<coef_t>(padding_stride));
                            coef_t t1 = index_stride[i] / g;
                            if (t1 != 1)
                            op = ocg->CreateIntegerFloor(ocg->CreatePlus(op, ocg->CreateInt(t1-1)), ocg->CreateInt(t1));
                            coef_t t2 = padding_stride / g;
                            if (t2 != 1)
                            op = ocg->CreateTimes(op, ocg->CreateInt(t2));
                            }
                            else if (index_stride[i] != 1) {
                            op = ocg->CreateIntegerFloor(ocg->CreatePlus(op, ocg->CreateInt(index_stride[i]-1)), ocg->CreateInt(index_stride[i]));
                            }
                            }
              */
              //index_sz.push_back(std::make_pair(i, op));
              //break;
            }
          }
        }
      }
    }
    //size[i] = ub[i];

  }
  /////////////////////////////////////////////////////////////////////////////////////////////////////
  //

  //Anand: Creating IS of new statement

  //for(int l = dim; l < stmt[stmt_num].xform.n_out(); l+=2)
  //std::cout << "In scalar_expand function 3: " << stmt_num << ", " << arrName << "\n";
  //std::cout.flush(); 

  //fprintf(stderr, "\n%d statements\n", stmt.size());
  //for (int i=0; i<stmt.size(); i++) { 
  //  fprintf(stderr, "%2d   ", i); 
  //  ((CG_chillRepr *)stmt[i].code)->Dump();
  //} 
  //fprintf(stderr, "\n"); 


  shiftLexicalOrder(lex, dim + 1, 1);
  Statement s = stmt[stmt_num];
  s.ir_stmt_node = NULL;
  int newStmt_num = stmt.size();

  fprintf(stderr, "loop.cc L3249 adding stmt %d\n", stmt.size());
  stmt.push_back(s);

  fprintf(stderr, "uninterpreted_symbols.push_back()  newStmt_num %d\n", newStmt_num);
  uninterpreted_symbols.push_back(uninterpreted_symbols[stmt_num]);
  uninterpreted_symbols_stringrepr.push_back(uninterpreted_symbols_stringrepr[stmt_num]);
  stmt[newStmt_num].code = stmt[stmt_num].code->clone();
  stmt[newStmt_num].IS = copy(stmt[stmt_num].IS);
  stmt[newStmt_num].xform = xform;
  stmt[newStmt_num].reduction = stmt[stmt_num].reduction;
  stmt[newStmt_num].reductionOp = stmt[stmt_num].reductionOp;


  //fprintf(stderr, "\nafter clone, %d statements\n", stmt.size());
  //for (int i=0; i<stmt.size(); i++) { 
  //  fprintf(stderr, "%2d   ", i); 
  //  ((CG_chillRepr *)stmt[i].code)->Dump();
  //} 
  //fprintf(stderr, "\n"); 



  //assign_const(stmt[newStmt_num].xform, stmt[stmt_num].xform.n_out(), 1);//Anand: change from 2*level + 1 to stmt[stmt_num].xform.size()
  //Anand-End creating IS of new statement

  CG_outputRepr *tmpArrSz;
  CG_outputBuilder *ocg = ir->builder();

  //for(int k =0; k < levels.size(); k++ )
  //    size_repr.push_back(ocg->CreateInt(size[k]));//Anand: copying apply_xform functionality to prevent IS modification
  //due to side effects with uninterpreted function symbols and failures in omega

  //int n = stmt[stmt_num].loop_level.size();

  /*Relation mapping(2 * n + 1, n);
    F_And *f_root = mapping.add_and();
    for (int j = 1; j <= n; j++) {
    EQ_Handle h = f_root->add_EQ();
    h.update_coef(mapping.output_var(j), 1);
    h.update_coef(mapping.input_var(2 * j), -1);
    }
    mapping = Composition(mapping, copy(stmt[stmt_num].xform));
    mapping.simplify();
    
    // match omega input/output variables to variable names in the code
    for (int j = 1; j <= stmt[stmt_num].IS.n_set(); j++)
    mapping.name_input_var(j, stmt[stmt_num].IS.set_var(j)->name());
    for (int j = 1; j <= n; j++)
    mapping.name_output_var(j,
    tmp_loop_var_name_prefix
    + to_string(tmp_loop_var_name_counter + j - 1));
    mapping.setup_names();
    
    Relation size_ = omega::Range(Restrict_Domain(mapping, copy(stmt[stmt_num].IS)));
    size_.simplify();
  */

  //Anand -commenting out tmp sym creation as symbol may have more than one dimension
  //tmp_sym = ir->CreateArraySymbol(tmpArrSz, sym);
  std::vector<CG_outputRepr *> lhs_index;
  CG_outputRepr *arr_ref_repr;
  arr_ref_repr = ocg->CreateIdent(
      stmt[stmt_num].IS.set_var(levels[levels.size() - 1])->name());

  CG_outputRepr *total_size = size_repr[0];
  fprintf(stderr, "total_size = ");
  total_size->dump();
  fflush(stdout);

  for (int i = 1; i < size_repr.size(); i++) {
    fprintf(stderr, "total_size now ");
    total_size->dump();
    fflush(stdout);
    fprintf(stderr, " times  something\n\n");

    total_size = ocg->CreateTimes(total_size->clone(),
                                  size_repr[i]->clone());

  }

  // COMMENT NEEDED 
  //fprintf(stderr, "\nloop.cc COMMENT NEEDED\n"); 
  for (int k = levels.size() - 2; k >= 0; k--) {
    CG_outputRepr *temp_repr = ocg->CreateIdent(stmt[stmt_num].IS.set_var(levels[k])->name());
    for (int l = k + 1; l < levels.size(); l++) {
      //fprintf(stderr, "\nloop.cc CREATETIMES\n"); 
      temp_repr = ocg->CreateTimes(temp_repr->clone(),
                                   size_repr[l]->clone());
    }

    //fprintf(stderr, "\nloop.cc CREATEPLUS\n"); 
    arr_ref_repr = ocg->CreatePlus(arr_ref_repr->clone(),
                                   temp_repr->clone());
  }


  //fprintf(stderr, "loop.cc, about to die\n"); 
  std::vector<CG_outputRepr *> to_push;
  to_push.push_back(total_size);

  if (!found_non_constant_size_dimension) {
    fprintf(stderr, "constant size dimension\n");
    tmp_sym = ir->CreateArraySymbol(sym, to_push, memory_type);
  } else {
    fprintf(stderr, "NON constant size dimension?\n");
    //tmp_sym = ir->CreatePointerSymbol(sym, to_push);
    tmp_sym = ir->CreatePointerSymbol(sym, to_push);

    static_cast<IR_PointerSymbol *>(tmp_sym)->set_size(0, total_size); // ?? 
    ptr_variables.push_back(static_cast<IR_PointerSymbol *>(tmp_sym));
    fprintf(stderr, "ptr_variables now has %d entries\n", ptr_variables.size());
  }

  // add tmp_sym to Loop symtables ??


  //  std::cout << " temp array name == " << tmp_sym->name().c_str() << "\n";

  // get loop index variable at the given "level"
  // Relation R = omega::Range(Restrict_Domain(copy(stmt[stmt_num].xform), copy(stmt[stmt_num].IS)));
  //  stmt[stmt_num].IS.print();
  //stmt[stmt_num].IS.
  //  std::cout << stmt[stmt_num].IS.n_set() << std::endl;
  //  std::string v = stmt[stmt_num].IS.set_var(level)->name();
  //  std::cout << "loop index variable is '" << v.c_str() << "'\n";

  // create a reference for the temporary array
  fprintf(stderr, "create a reference for the temporary array\n");
  //std::cout << "In scalar_expand function 4: " << stmt_num << ", " << arrName << "\n";
  //std::cout.flush(); 

  //fprintf(stderr, "\n%d statements\n", stmt.size());
  //for (int i=0; i<stmt.size(); i++) { 
  //  fprintf(stderr, "%2d   ", i); 
  //  ((CG_chillRepr *)stmt[i].code)->Dump();
  //} 
  //fprintf(stderr, "\n"); 



  std::vector<CG_outputRepr *> to_push2;
  to_push2.push_back(arr_ref_repr); // can have only one entry

  //lhs_index[0] = ocg->CreateIdent(v);


  IR_ArrayRef *tmp_array_ref;
  IR_PointerArrayRef *tmp_ptr_array_ref;  // was IR_PointerArrayref

  if (!found_non_constant_size_dimension) {
    fprintf(stderr, "constant size\n");

    tmp_array_ref = ir->CreateArrayRef(
        static_cast<IR_ArraySymbol *>(tmp_sym), to_push2);
  } else {
    fprintf(stderr, "NON constant size\n");
    tmp_ptr_array_ref = ir->CreatePointerArrayRef(
        static_cast<IR_PointerSymbol *>(tmp_sym), to_push2);
    // TODO static_cast<IR_PointerSymbol *>(tmp_sym), to_push2);
  }
  fflush(stdout);

  //fprintf(stderr, "\n%d statements\n", stmt.size());
  //for (int i=0; i<stmt.size(); i++) { 
  //  fprintf(stderr, "%2d   ", i); 
  //  ((CG_chillRepr *)stmt[i].code)->Dump();
  //} 
  //fprintf(stderr, "\n"); 


  //std::string stemp;
  //stemp = tmp_array_ref->name();
  //std::cout << "Created array reference --> " << stemp.c_str() << "\n";

  // get the RHS expression
  fprintf(stderr, "get the RHS expression   arrName %s\n", arrName.c_str());

  CG_outputRepr *rhs;
  if (arrName == "RHS") {
    rhs = ir->GetRHSExpression(stmt[stmt_num].code);

    std::vector<IR_ArrayRef *> symbols = ir->FindArrayRef(rhs);
  }
  std::set<std::string> sym_names;

  //for (int i = 0; i < symbols.size(); i++)
  //  sym_names.insert(symbols[i]->symbol()->name());

  fflush(stdout);

  //fprintf(stderr, "\nbefore if (arrName == RHS)\n%d statements\n", stmt.size()); // problem is after here 
  //for (int i=0; i<stmt.size(); i++) { 
  //  fprintf(stderr, "%2d   ", i); 
  //  ((CG_chillRepr *)stmt[i].code)->Dump();
  //} 
  //fprintf(stderr, "\n"); 

  if (arrName == "RHS") {

    std::vector<IR_ArrayRef *> symbols = ir->FindArrayRef(rhs);

    for (int i = 0; i < symbols.size(); i++)
      sym_names.insert(symbols[i]->symbol()->name());
  } else {

    fprintf(stderr, "finding array refs in stmt_num %d\n", stmt_num);
    //fprintf(stderr, "\n%d statements\n", stmt.size());
    //for (int i=0; i<stmt.size(); i++) { 
    //  fprintf(stderr, "%2d   ", i); 
    //  ((CG_chillRepr *)stmt[i].code)->Dump();
    //} 
    //fprintf(stderr, "\n"); 

    std::vector<IR_ArrayRef *> refs = ir->FindArrayRef(stmt[stmt_num].code);
    fprintf(stderr, "\n%d refs\n", refs.size());


    bool found = false;

    for (int j = 0; j < refs.size(); j++) {
      CG_outputRepr *to_replace;

      fprintf(stderr, "j %d   build new assignment statement with temporary array\n", j);
      // build new assignment statement with temporary array
      if (!found_non_constant_size_dimension) {
        to_replace = tmp_array_ref->convert();
      } else {
        to_replace = tmp_ptr_array_ref->convert();
      }
      //fprintf(stderr, "to_replace  %p\n", to_replace); 
      //CG_chillRepr *CR = (CG_chillRepr *) to_replace;
      //CR->Dump(); 

      if (refs[j]->name() == arrName) {
        fflush(stdout);
        fprintf(stderr, "loop.cc L353\n");  // problem is after here 
        //fprintf(stderr, "\n%d statements\n", stmt.size());
        //for (int i=0; i<stmt.size(); i++) { 
        //  fprintf(stderr, "%2d   ", i); 
        //  ((CG_chillRepr *)stmt[i].code)->Dump();
        //} 
        //fprintf(stderr, "\n"); 


        sym_names.insert(refs[j]->symbol()->name());

        if (!found) {
          if (!found_non_constant_size_dimension) {
            fprintf(stderr, "constant size2\n");
            omega::CG_outputRepr *t = tmp_array_ref->convert();
            omega::CG_outputRepr *r = refs[j]->convert()->clone();
            //CR = (CG_chillRepr *) t;
            //CR->Dump(); 
            //CR = (CG_chillRepr *) r;
            //CR->Dump(); 

            //fprintf(stderr, "lhs t %p   lhs r %p\n", t, r); 
            stmt[newStmt_num].code =
                ir->builder()->CreateAssignment(0,
                                                t, // tmp_array_ref->convert(),
                                                r); // refs[j]->convert()->clone()
          } else {
            fprintf(stderr, "NON constant size2\n");
            omega::CG_outputRepr *t = tmp_ptr_array_ref->convert(); // this fails
            omega::CG_outputRepr *r = refs[j]->convert()->clone();

            //omega::CG_chillRepr *CR = (omega::CG_chillRepr *) t;
            //CR->Dump(); 
            //CR = (omega::CG_chillRepr *) r;
            //CR->Dump(); 

            //fprintf(stderr, "lhs t %p   lhs r %p\n", t, r); 
            stmt[newStmt_num].code =
                ir->builder()->CreateAssignment(0,
                                                t, // tmp_ptr_array_ref->convert(),
                                                r); // refs[j]->convert()->clone());
          }
          found = true;

        }

        // refs[j] has no parent?
        fprintf(stderr, "replacing refs[%d]\n", j);
        ir->ReplaceExpression(refs[j], to_replace);
      }

    }

  }
  //ToDo need to update the dependence graph
  //Anand adding dependence graph update
  fprintf(stderr, "adding dependence graph update\n");   // problem is before here 
  //fprintf(stderr, "\n%d statements\n", stmt.size());
  //for (int i=0; i<stmt.size(); i++) { 
  //  fprintf(stderr, "%2d   ", i); 
  //  ((CG_chillRepr *)stmt[i].code)->Dump();
  //} 
  //fprintf(stderr, "\n"); 

  dep.insert();

  //Anand:Copying Dependence checks from datacopy code, might need to be a separate function/module
  // in the future

  /*for (int i = 0; i < newStmt_num; i++) {
    std::vector<std::vector<DependenceVector> > D;
    
    for (DependenceGraph::EdgeList::iterator j =
    dep.vertex[i].second.begin(); j != dep.vertex[i].second.end();
    ) {
    if (same_loop.find(i) != same_loop.end()
    && same_loop.find(j->first) == same_loop.end()) {
    std::vector<DependenceVector> dvs1, dvs2;
    for (int k = 0; k < j->second.size(); k++) {
    DependenceVector dv = j->second[k];
    if (dv.sym != NULL
    && sym_names.find(dv.sym->name()) != sym_names.end()
    && (dv.type == DEP_R2R || dv.type == DEP_R2W))
    dvs1.push_back(dv);
    else
    dvs2.push_back(dv);
    }
    j->second = dvs2;
    if (dvs1.size() > 0)
    dep.connect(newStmt_num, j->first, dvs1);
    } else if (same_loop.find(i) == same_loop.end()
    && same_loop.find(j->first) != same_loop.end()) {
    std::vector<DependenceVector> dvs1, dvs2;
    for (int k = 0; k < j->second.size(); k++) {
    DependenceVector dv = j->second[k];
    if (dv.sym != NULL
    && sym_names.find(dv.sym->name()) != sym_names.end()
    && (dv.type == DEP_R2R || dv.type == DEP_W2R))
    dvs1.push_back(dv);
    else
    dvs2.push_back(dv);
    }
    j->second = dvs2;
    if (dvs1.size() > 0)
    D.push_back(dvs1);
    }
    
    if (j->second.size() == 0)
    dep.vertex[i].second.erase(j++);
    else
    j++;
    }
    
    for (int j = 0; j < D.size(); j++)
    dep.connect(i, newStmt_num, D[j]);
    }
  */
  //Anand--end dependence check
  if (arrName == "RHS") {

    // build new assignment statement with temporary array
    if (!found_non_constant_size_dimension) {
      if (assign_then_accumulate) {
        stmt[newStmt_num].code = ir->builder()->CreateAssignment(0,
                                                                 tmp_array_ref->convert(), rhs);
        fprintf(stderr, "ir->ReplaceRHSExpression( stmt_ num %d )\n", stmt_num);
        ir->ReplaceRHSExpression(stmt[stmt_num].code, tmp_array_ref);
      } else {
        CG_outputRepr *temp = tmp_array_ref->convert()->clone();
        if (ir->QueryExpOperation(stmt[stmt_num].code)
            != IR_OP_PLUS_ASSIGNMENT)
          throw chill::error::ir(
              "Statement is not a += accumulation statement");

        fprintf(stderr, "replacing in a +=\n");
        stmt[newStmt_num].code = ir->builder()->CreatePlusAssignment(0,
                                                                     temp->clone(), rhs);

        CG_outputRepr *lhs = ir->GetLHSExpression(stmt[stmt_num].code);

        CG_outputRepr *assignment = ir->builder()->CreateAssignment(0,
                                                                    lhs, temp->clone());
        Statement init_ = stmt[newStmt_num]; // copy ??
        init_.ir_stmt_node = NULL;

        init_.code = stmt[newStmt_num].code->clone();
        init_.IS = copy(stmt[newStmt_num].IS);
        init_.xform = copy(stmt[newStmt_num].xform);
        init_.has_inspector = false; // ?? 

        Relation mapping(init_.IS.n_set(), init_.IS.n_set());

        F_And *f_root = mapping.add_and();

        for (int i = 1; i <= mapping.n_inp(); i++) {
          EQ_Handle h = f_root->add_EQ();
          //if (i < levels[0]) {
          if (i <= levels[levels.size() - 1]) {
            h.update_coef(mapping.input_var(i), 1);
            h.update_coef(mapping.output_var(i), -1);
          } else {
            h.update_const(-1);
            h.update_coef(mapping.output_var(i), 1);
          }

          /*else {
            int j;
            for (j = 0; j < levels.size(); j++)
            if (i == levels[j])
            break;
            
            if (j == levels.size()) {
            
            h.update_coef(mapping.output_var(i), 1);
            h.update_const(-1);
            
            } else {
            
            
            h.update_coef(mapping.input_var(i), 1);
            h.update_coef(mapping.output_var(i), -1);
            
            
            }
          */
          //}
        }

        mapping.simplify();
        // match omega input/output variables to variable names in the code
        for (int j = 1; j <= init_.IS.n_set(); j++)
          mapping.name_output_var(j, init_.IS.set_var(j)->name());
        for (int j = 1; j <= init_.IS.n_set(); j++)
          mapping.name_input_var(j, init_.IS.set_var(j)->name());

        mapping.setup_names();

        init_.IS = omega::Range(
            omega::Restrict_Domain(mapping, init_.IS));
        std::vector<int> lex = getLexicalOrder(newStmt_num);
        int dim = 2 * levels[0] - 1;
        //init_.IS.print();
        //  init_.xform.print();
        //stmt[newStmt_num].xform.print();
        //  shiftLexicalOrder(lex, dim + 1, 1);
        shiftLexicalOrder(lex, dim + 1, 1);
        init_.reduction = stmt[newStmt_num].reduction;
        init_.reductionOp = stmt[newStmt_num].reductionOp;

        init_.code = ir->builder()->CreateAssignment(0, temp->clone(),
                                                     ir->builder()->CreateInt(0));

        fprintf(stderr, "loop.cc L3693 adding stmt %d\n", stmt.size());
        stmt.push_back(init_);

        uninterpreted_symbols.push_back(uninterpreted_symbols[newStmt_num]);
        uninterpreted_symbols_stringrepr.push_back(uninterpreted_symbols_stringrepr[newStmt_num]);
        stmt[stmt_num].code = assignment;
      }
    } else {
      if (assign_then_accumulate) {
        stmt[newStmt_num].code = ir->builder()->CreateAssignment(0,
                                                                 tmp_ptr_array_ref->convert(), rhs);
        ir->ReplaceRHSExpression(stmt[stmt_num].code,
                                 tmp_ptr_array_ref);
      } else {
        CG_outputRepr *temp = tmp_ptr_array_ref->convert()->clone();
        if (ir->QueryExpOperation(stmt[stmt_num].code)
            != IR_OP_PLUS_ASSIGNMENT)
          throw chill::error::ir(
              "Statement is not a += accumulation statement");
        stmt[newStmt_num].code = ir->builder()->CreatePlusAssignment(0,
                                                                     temp->clone(), rhs);

        CG_outputRepr *lhs = ir->GetLHSExpression(stmt[stmt_num].code);

        CG_outputRepr *assignment = ir->builder()->CreateAssignment(0,
                                                                    lhs, temp->clone());

        stmt[stmt_num].code = assignment;
      }
      // call function to replace rhs with temporary array
    }
  }

  //std::cout << "End of scalar_expand function!! \n";

  //  if(arrName == "RHS"){
  DependenceVector dv;
  std::vector<DependenceVector> E;
  dv.lbounds = std::vector<omega::coef_t>(4);
  dv.ubounds = std::vector<omega::coef_t>(4);
  dv.type = DEP_W2R;

  for (int k = 0; k < 4; k++) {
    dv.lbounds[k] = 0;
    dv.ubounds[k] = 0;

  }

  //std::vector<IR_ArrayRef*> array_refs = ir->FindArrayRef(stmt[newStmt_num].code);
  dv.sym = tmp_sym->clone();

  E.push_back(dv);

  dep.connect(newStmt_num, stmt_num, E);
  // }

}


std::pair<Relation, Relation> createCSRstyleISandXFORM(CG_outputBuilder *ocg,
                                                       std::vector<Relation> &outer_loop_bounds, std::string index_name,
                                                       std::map<int, Relation> &zero_loop_bounds,
                                                       std::map<std::string, std::vector<omega::CG_outputRepr *> > &uninterpreted_symbols,
                                                       std::map<std::string, std::vector<omega::CG_outputRepr *> > &uninterpreted_symbols_string,
                                                       Loop *this_loop) {

  Relation IS(outer_loop_bounds.size() + 1 + zero_loop_bounds.size());
  Relation XFORM(outer_loop_bounds.size() + 1 + zero_loop_bounds.size(),
                 2 * (outer_loop_bounds.size() + 1 + zero_loop_bounds.size()) + 1);

  F_And *f_r_ = IS.add_and();
  F_And *f_root = XFORM.add_and();

  if (outer_loop_bounds.size() > 0) {
    for (int it = 0; it < IS.n_set(); it++) {
      IS.name_set_var(it + 1,
                      const_cast<Relation &>(outer_loop_bounds[0]).set_var(it + 1)->name());
      XFORM.name_input_var(it + 1,
                           const_cast<Relation &>(outer_loop_bounds[0]).set_var(it + 1)->name());

    }
  } else if (zero_loop_bounds.size() > 0) {
    for (int it = 0; it < IS.n_set(); it++) {
      IS.name_set_var(it + 1,
                      const_cast<Relation &>(zero_loop_bounds.begin()->second).set_var(
                          it + 1)->name());
      XFORM.name_input_var(it + 1,
                           const_cast<Relation &>(zero_loop_bounds.begin()->second).set_var(
                               it + 1)->name());

    }

  }

  for (int i = 0; i < outer_loop_bounds.size(); i++)
    IS = replace_set_var_as_another_set_var(IS, outer_loop_bounds[i], i + 1,
                                            i + 1);

  int count = 1;
  for (std::map<int, Relation>::iterator i = zero_loop_bounds.begin();
       i != zero_loop_bounds.end(); i++, count++)
    IS = replace_set_var_as_another_set_var(IS, i->second,
                                            outer_loop_bounds.size() + 1 + count, i->first);

  if (outer_loop_bounds.size() > 0) {
    Free_Var_Decl *lb = new Free_Var_Decl(index_name + "_", 1); // index_
    Variable_ID csr_lb = IS.get_local(lb, Input_Tuple);

    Free_Var_Decl *ub = new Free_Var_Decl(index_name + "__", 1); // index__
    Variable_ID csr_ub = IS.get_local(ub, Input_Tuple);

    //lower bound

    F_And *f_r = IS.and_with_and();
    GEQ_Handle lower_bound = f_r->add_GEQ();
    lower_bound.update_coef(csr_lb, -1);
    lower_bound.update_coef(IS.set_var(outer_loop_bounds.size() + 1), 1);

    //upper bound

    GEQ_Handle upper_bound = f_r->add_GEQ();
    upper_bound.update_coef(csr_ub, 1);
    upper_bound.update_coef(IS.set_var(outer_loop_bounds.size() + 1), -1);
    upper_bound.update_const(-1);

    omega::CG_stringBuilder *ocgs = new CG_stringBuilder;

    std::vector<omega::CG_outputRepr *> reprs;
    std::vector<omega::CG_outputRepr *> reprs2;

    std::vector<omega::CG_outputRepr *> reprs3;
    std::vector<omega::CG_outputRepr *> reprs4;

    reprs.push_back(
        ocg->CreateIdent(IS.set_var(outer_loop_bounds.size())->name()));
    reprs2.push_back(
        ocgs->CreateIdent(
            IS.set_var(outer_loop_bounds.size())->name()));
    uninterpreted_symbols.insert(
        std::pair<std::string, std::vector<CG_outputRepr *> >(
            index_name + "_", reprs));
    uninterpreted_symbols_string.insert(
        std::pair<std::string, std::vector<CG_outputRepr *> >(
            index_name + "_", reprs2));

    std::string arg = "(" + IS.set_var(outer_loop_bounds.size())->name()
                      + ")";
    std::vector<std::string> argvec;
    argvec.push_back(arg);

    CG_outputRepr *repr = ocg->CreateArrayRefExpression(index_name,
                                                        ocg->CreateIdent(IS.set_var(outer_loop_bounds.size())->name()));

    //fprintf(stderr, "( VECTOR _)\n"); 
    //fprintf(stderr, "loop.cc calling CreateDefineMacro( %s, argvec, repr)\n", (index_name + "_").c_str()); 
    this_loop->ir->CreateDefineMacro(index_name + "_", argvec, repr);

    Relation known_(copy(IS).n_set());
    known_.copy_names(copy(IS));
    known_.setup_names();
    Variable_ID index_lb = known_.get_local(lb, Input_Tuple);
    Variable_ID index_ub = known_.get_local(ub, Input_Tuple);
    F_And *fr = known_.add_and();
    GEQ_Handle g = fr->add_GEQ();
    g.update_coef(index_ub, 1);
    g.update_coef(index_lb, -1);
    g.update_const(-1);
    this_loop->addKnown(known_);

    reprs3.push_back(

        ocg->CreateIdent(IS.set_var(outer_loop_bounds.size())->name()));
    reprs4.push_back(

        ocgs->CreateIdent(IS.set_var(outer_loop_bounds.size())->name()));

    CG_outputRepr *repr2 = ocg->CreateArrayRefExpression(index_name,
                                                         ocg->CreatePlus(
                                                             ocg->CreateIdent(
                                                                 IS.set_var(outer_loop_bounds.size())->name()),
                                                             ocg->CreateInt(1)));

    //fprintf(stderr, "( VECTOR __)\n"); 
    //fprintf(stderr, "loop.cc calling CreateDefineMacro( %s, argvec, repr)\n", (index_name + "__").c_str()); 

    this_loop->ir->CreateDefineMacro(index_name + "__", argvec, repr2);

    uninterpreted_symbols.insert(
        std::pair<std::string, std::vector<CG_outputRepr *> >(
            index_name + "__", reprs3));
    uninterpreted_symbols_string.insert(
        std::pair<std::string, std::vector<CG_outputRepr *> >(
            index_name + "__", reprs4));
  } else {
    Free_Var_Decl *ub = new Free_Var_Decl(index_name);
    Variable_ID csr_ub = IS.get_local(ub);
    F_And *f_r = IS.and_with_and();
    GEQ_Handle upper_bound = f_r->add_GEQ();
    upper_bound.update_coef(csr_ub, 1);
    upper_bound.update_coef(IS.set_var(outer_loop_bounds.size() + 1), -1);
    upper_bound.update_const(-1);

    GEQ_Handle lower_bound = f_r->add_GEQ();
    lower_bound.update_coef(IS.set_var(outer_loop_bounds.size() + 1), 1);

  }

  for (int j = 1; j <= XFORM.n_inp(); j++) {
    omega::EQ_Handle h = f_root->add_EQ();
    h.update_coef(XFORM.output_var(2 * j), 1);
    h.update_coef(XFORM.input_var(j), -1);
  }

  for (int j = 1; j <= XFORM.n_out(); j += 2) {
    omega::EQ_Handle h = f_root->add_EQ();
    h.update_coef(XFORM.output_var(j), 1);
  }

  if (_DEBUG_) {
    IS.print();
    XFORM.print();

  }

  return std::pair<Relation, Relation>(IS, XFORM);

}

std::pair<Relation, Relation> construct_reduced_IS_And_XFORM(IR_Code *ir,
                                                             const Relation &is, const Relation &xform,
                                                             const std::vector<int> loops,
                                                             std::vector<int> &lex_order, Relation &known,
                                                             std::map<std::string, std::vector<CG_outputRepr *> > &uninterpreted_symbols) {

  Relation IS(loops.size());
  Relation XFORM(loops.size(), 2 * loops.size() + 1);
  int count_ = 1;
  std::map<int, int> pos_mapping;

  int n = is.n_set();
  Relation is_and_known = Intersection(copy(is),
                                       Extend_Set(copy(known), n - known.n_set()));

  for (int it = 0; it < loops.size(); it++, count_++) {
    IS.name_set_var(count_,
                    const_cast<Relation &>(is).set_var(loops[it])->name());
    XFORM.name_input_var(count_,
                         const_cast<Relation &>(xform).input_var(loops[it])->name());
    XFORM.name_output_var(2 * count_,
                          const_cast<Relation &>(xform).output_var((loops[it]) * 2)->name());
    XFORM.name_output_var(2 * count_ - 1,
                          const_cast<Relation &>(xform).output_var((loops[it]) * 2 - 1)->name());
    pos_mapping.insert(std::pair<int, int>(count_, loops[it]));
  }

  XFORM.name_output_var(2 * loops.size() + 1,
                        const_cast<Relation &>(xform).output_var(is.n_set() * 2 + 1)->name());

  F_And *f_r = IS.add_and();
  for (std::map<int, int>::iterator it = pos_mapping.begin();
       it != pos_mapping.end(); it++)
    IS = replace_set_var_as_another_set_var(IS, is_and_known, it->first,
                                            it->second);
  /*
    for (std::map<std::string, std::vector<CG_outputRepr *> >::iterator it2 =
    uninterpreted_symbols.begin();
    it2 != uninterpreted_symbols.end(); it2++) {
    std::vector<CG_outputRepr *> reprs_ = it2->second;
    //std::vector<CG_outputRepr *> reprs_2;
    
    for (int k = 0; k < reprs_.size(); k++) {
    std::vector<IR_ScalarRef *> refs = ir->FindScalarRef(reprs_[k]);
    bool exception_found = false;
    for (int m = 0; m < refs.size(); m++){
    
    if (refs[m]->name()
    == const_cast<Relation &>(is).set_var(it->second)->name())
    try {
    ir->ReplaceExpression(refs[m],
    ir->builder()->CreateIdent(
    IS.set_var(it->first)->name()));
    } catch (ir_error &e) {
    
    reprs_[k] = ir->builder()->CreateIdent(
    IS.set_var(it->first)->name());
    exception_found = true;
    }
    if(exception_found)
    break;
    }
    
    }
    it2->second = reprs_;
    }
    
    }
  */
  CHILL_DEBUG_BEGIN
    std::cout << "relation debug" << std::endl;
    IS.print();
  CHILL_DEBUG_END

  F_And *f_root = XFORM.add_and();

  count_ = 1;

  for (int j = 1; j <= loops.size(); j++) {
    omega::EQ_Handle h = f_root->add_EQ();
    h.update_coef(XFORM.output_var(2 * j), 1);
    h.update_coef(XFORM.input_var(j), -1);
  }
  for (int j = 0; j < loops.size(); j++, count_++) {
    omega::EQ_Handle h = f_root->add_EQ();
    h.update_coef(XFORM.output_var(count_ * 2 - 1), 1);
    h.update_const(-lex_order[count_ * 2 - 2]);
  }

  omega::EQ_Handle h = f_root->add_EQ();
  h.update_coef(XFORM.output_var((loops.size()) * 2 + 1), 1);
  h.update_const(-lex_order[xform.n_out() - 1]);

  CHILL_DEBUG_BEGIN
    std::cout << "relation debug" << std::endl;
    IS.print();
    XFORM.print();
  CHILL_DEBUG_END

  return std::pair<Relation, Relation>(IS, XFORM);

}

std::set<std::string> inspect_repr_for_scalars(IR_Code *ir,
                                               CG_outputRepr *repr, std::set<std::string> ignore) {

  std::vector<IR_ScalarRef *> refs = ir->FindScalarRef(repr);
  std::set<std::string> loop_vars;

  for (int i = 0; i < refs.size(); i++)
    if (ignore.find(refs[i]->name()) == ignore.end())
      loop_vars.insert(refs[i]->name());

  return loop_vars;

}

std::set<std::string> inspect_loop_bounds(IR_Code *ir, const Relation &R,
                                          int pos,
                                          std::map<std::string, std::vector<omega::CG_outputRepr *> > &uninterpreted_symbols) {

  if (!R.is_set())
    throw chill::error::loop("Input R has to be a set not a relation!");

  std::set<std::string> vars;

  std::vector<CG_outputRepr *> refs;
  Variable_ID v = const_cast<Relation &>(R).set_var(pos);
  for (DNF_Iterator di(const_cast<Relation &>(R).query_DNF()); di; di++) {
    for (GEQ_Iterator gi = (*di)->GEQs(); gi; gi++) {
      if ((*gi).get_coef(v) != 0 && (*gi).is_const_except_for_global(v)) {
        for (Constr_Vars_Iter cvi(*gi); cvi; cvi++) {
          Variable_ID v = cvi.curr_var();
          switch (v->kind()) {

            case Global_Var: {
              Global_Var_ID g = v->get_global_var();
              Variable_ID v2;
              if (g->arity() > 0) {

                std::string s = g->base_name();
                std::copy(
                    uninterpreted_symbols.find(s)->second.begin(),
                    uninterpreted_symbols.find(s)->second.end(),
                    back_inserter(refs));

              }

              break;
            }
            default:
              break;
          }
        }

      }
    }
  }

  for (int i = 0; i < refs.size(); i++) {
    std::vector<IR_ScalarRef *> refs_ = ir->FindScalarRef(refs[i]);

    for (int j = 0; j < refs_.size(); j++)
      vars.insert(refs_[j]->name());

  }
  return vars;
}

CG_outputRepr *create_counting_loop_body(IR_Code *ir, const Relation &R,
                                         int pos, CG_outputRepr *count,
                                         std::map<std::string, std::vector<omega::CG_outputRepr *> > &uninterpreted_symbols) {

  if (!R.is_set())
    throw chill::error::loop("Input R has to be a set not a relation!");

  CG_outputRepr *ub, *lb;
  ub = NULL;
  lb = NULL;
  std::vector<CG_outputRepr *> refs;
  Variable_ID v = const_cast<Relation &>(R).set_var(pos);
  for (DNF_Iterator di(const_cast<Relation &>(R).query_DNF()); di; di++) {
    for (GEQ_Iterator gi = (*di)->GEQs(); gi; gi++) {
      if ((*gi).get_coef(v) != 0 && (*gi).is_const_except_for_global(v)) {
        bool same_ge_1 = false;
        bool same_ge_2 = false;
        for (Constr_Vars_Iter cvi(*gi); cvi; cvi++) {
          Variable_ID v = cvi.curr_var();
          switch (v->kind()) {

            case Global_Var: {
              Global_Var_ID g = v->get_global_var();
              Variable_ID v2;
              if (g->arity() > 0) {

                std::string s = g->base_name();

                if ((*gi).get_coef(v) > 0) {
                  if (ub != NULL)
                    throw chill::error::ir(
                        "bound expression too complex!");

                  ub = ir->builder()->CreateInvoke(s,
                                                   uninterpreted_symbols.find(s)->second);
                  //ub = ir->builder()->CreateMinus(ub->clone(), ir->builder()->CreateInt(-(*gi).get_const()));
                  same_ge_1 = true;

                } else {
                  if (lb != NULL)
                    throw chill::error::ir(
                        "bound expression too complex!");
                  lb = ir->builder()->CreateInvoke(s,
                                                   uninterpreted_symbols.find(s)->second);
                  same_ge_2 = true;

                }
              }

              break;
            }
            default:
              break;
          }
        }

        if (same_ge_1 && same_ge_2)
          lb = ir->builder()->CreatePlus(lb->clone(),
                                         ir->builder()->CreateInt(-(*gi).get_const()));
        else if (same_ge_1)
          ub = ir->builder()->CreatePlus(ub->clone(),
                                         ir->builder()->CreateInt(-(*gi).get_const()));
        else if (same_ge_2)
          lb = ir->builder()->CreatePlus(lb->clone(),
                                         ir->builder()->CreateInt(-(*gi).get_const()));
      }
    }

  }

  return ir->builder()->CreatePlusAssignment(0, count,
                                             ir->builder()->CreatePlus(
                                                 ir->builder()->CreateMinus(ub->clone(), lb->clone()),
                                                 ir->builder()->CreateInt(1)));
}


std::map<std::string, std::vector<std::string> > recurse_on_exp_for_arrays(
    IR_Code *ir, CG_outputRepr *exp) {

  std::map<std::string, std::vector<std::string> > arr_index_to_ref;
  switch (ir->QueryExpOperation(exp)) {

    case IR_OP_ARRAY_VARIABLE: {
      IR_ArrayRef *ref = dynamic_cast<IR_ArrayRef *>(ir->Repr2Ref(exp));
      IR_PointerArrayRef *ref_ =
          dynamic_cast<IR_PointerArrayRef *>(ir->Repr2Ref(exp));
      if (ref == NULL && ref_ == NULL)
        throw chill::error::loop("Array symbol unidentifiable!");

      if (ref != NULL) {
        std::vector<std::string> s0;

        for (int i = 0; i < ref->n_dim(); i++) {
          CG_outputRepr *index = ref->index(i);
          std::map<std::string, std::vector<std::string> > a0 =
              recurse_on_exp_for_arrays(ir, index);
          std::vector<std::string> s;
          for (std::map<std::string, std::vector<std::string> >::iterator j =
              a0.begin(); j != a0.end(); j++) {
            if (j->second.size() != 1 && (j->second)[0] != "")
              throw chill::error::loop(
                  "indirect array references not allowed in guard!");
            s.push_back(j->first);
          }
          std::copy(s.begin(), s.end(), back_inserter(s0));
        }
        arr_index_to_ref.insert(
            std::pair<std::string, std::vector<std::string> >(
                ref->name(), s0));
      } else {
        std::vector<std::string> s0;
        for (int i = 0; i < ref_->n_dim(); i++) {
          CG_outputRepr *index = ref_->index(i);
          std::map<std::string, std::vector<std::string> > a0 =
              recurse_on_exp_for_arrays(ir, index);
          std::vector<std::string> s;
          for (std::map<std::string, std::vector<std::string> >::iterator j =
              a0.begin(); j != a0.end(); j++) {
            if (j->second.size() != 1 && (j->second)[0] != "")
              throw chill::error::loop(
                  "indirect array references not allowed in guard!");
            s.push_back(j->first);
          }
          std::copy(s.begin(), s.end(), back_inserter(s0));
        }
        arr_index_to_ref.insert(
            std::pair<std::string, std::vector<std::string> >(
                ref_->name(), s0));
      }
      break;
    }
    case IR_OP_PLUS:
    case IR_OP_MINUS:
    case IR_OP_MULTIPLY:
    case IR_OP_DIVIDE: {
      std::vector<CG_outputRepr *> v = ir->QueryExpOperand(exp);
      std::map<std::string, std::vector<std::string> > a0 =
          recurse_on_exp_for_arrays(ir, v[0]);
      std::map<std::string, std::vector<std::string> > a1 =
          recurse_on_exp_for_arrays(ir, v[1]);
      arr_index_to_ref.insert(a0.begin(), a0.end());
      arr_index_to_ref.insert(a1.begin(), a1.end());
      break;

    }
    case IR_OP_POSITIVE:
    case IR_OP_NEGATIVE: {
      std::vector<CG_outputRepr *> v = ir->QueryExpOperand(exp);
      std::map<std::string, std::vector<std::string> > a0 =
          recurse_on_exp_for_arrays(ir, v[0]);

      arr_index_to_ref.insert(a0.begin(), a0.end());
      break;

    }
    case IR_OP_VARIABLE: {
      std::vector<CG_outputRepr *> v = ir->QueryExpOperand(exp);
      IR_ScalarRef *ref = static_cast<IR_ScalarRef *>(ir->Repr2Ref(v[0]));

      std::string s = ref->name();
      std::vector<std::string> to_insert;
      to_insert.push_back("");
      arr_index_to_ref.insert(
          std::pair<std::string, std::vector<std::string> >(s,
                                                            to_insert));
      break;
    }
    case IR_OP_CONSTANT:
      break;

    default: {
      std::vector<CG_outputRepr *> v = ir->QueryExpOperand(exp);

      for (int i = 0; i < v.size(); i++) {
        std::map<std::string, std::vector<std::string> > a0 =
            recurse_on_exp_for_arrays(ir, v[i]);

        arr_index_to_ref.insert(a0.begin(), a0.end());
      }

      break;
    }
  }
  return arr_index_to_ref;
}


std::vector<CG_outputRepr *> find_guards(IR_Code *ir, IR_Control *code) {
  CHILL_DEBUG_PRINT("find_guards()\n");
  std::vector<CG_outputRepr *> guards;
  switch (code->type()) {
    case IR_CONTROL_IF: {
      CHILL_DEBUG_PRINT("find_guards() it's an if\n");
      CG_outputRepr *cond = dynamic_cast<IR_If *>(code)->condition();

      std::vector<CG_outputRepr *> then_body;
      std::vector<CG_outputRepr *> else_body;
      IR_Block *ORTB = dynamic_cast<IR_If *>(code)->then_body();
      if (ORTB != NULL) {
        CHILL_DEBUG_PRINT("recursing on then\n");
        then_body = find_guards(ir, ORTB);
        //dynamic_cast<IR_If*>(code)->then_body());
      }
      if (dynamic_cast<IR_If *>(code)->else_body() != NULL) {
        CHILL_DEBUG_PRINT("recursing on then\n");
        else_body = find_guards(ir,
                                dynamic_cast<IR_If *>(code)->else_body());
      }

      guards.push_back(cond);
      if (then_body.size() > 0)
        std::copy(then_body.begin(), then_body.end(),
                  back_inserter(guards));
      if (else_body.size() > 0)
        std::copy(else_body.begin(), else_body.end(),
                  back_inserter(guards));
      break;
    }
    case IR_CONTROL_BLOCK: {
      CHILL_DEBUG_PRINT("it's a control block\n");
      IR_Block *IRCB = dynamic_cast<IR_Block *>(code);
      CHILL_DEBUG_PRINT("calling ir->FindOneLevelControlStructure(IRCB);\n");
      std::vector<IR_Control *> stmts = ir->FindOneLevelControlStructure(IRCB);

      for (int i = 0; i < stmts.size(); i++) {
        std::vector<CG_outputRepr *> stmt_repr = find_guards(ir, stmts[i]);
        std::copy(stmt_repr.begin(), stmt_repr.end(),
                  back_inserter(guards));
      }
      break;
    }
    case IR_CONTROL_LOOP: {
      CHILL_DEBUG_PRINT("it's a control loop\n");
      std::vector<CG_outputRepr *> body = find_guards(ir,
                                                      dynamic_cast<IR_Loop *>(code)->body());
      if (body.size() > 0)
        std::copy(body.begin(), body.end(), back_inserter(guards));
      break;
    } // loop
  } // switch 
  return guards;
}

bool sort_helper(std::pair<std::string, std::vector<std::string> > i,
                 std::pair<std::string, std::vector<std::string> > j) {
  int c1 = 0;
  int c2 = 0;
  for (int k = 0; k < i.second.size(); k++)
    if (i.second[k] != "")
      c1++;

  for (int k = 0; k < j.second.size(); k++)
    if (j.second[k] != "")
      c2++;
  return (c1 < c2);

}

bool sort_helper_2(std::pair<int, int> i, std::pair<int, int> j) {

  return (i.second < j.second);

}

std::vector<std::string> construct_iteration_order(
    std::map<std::string, std::vector<std::string> > &input) {
  std::vector<std::string> arrays;
  std::vector<std::string> scalars;
  std::vector<std::pair<std::string, std::vector<std::string> > > input_aid;

  for (std::map<std::string, std::vector<std::string> >::iterator j =
      input.begin(); j != input.end(); j++)
    input_aid.push_back(
        std::pair<std::string, std::vector<std::string> >(j->first,
                                                          j->second));

  std::sort(input_aid.begin(), input_aid.end(), sort_helper);

  for (int j = 0; j < input_aid[input_aid.size() - 1].second.size(); j++)
    if (input_aid[input_aid.size() - 1].second[j] != "") {
      arrays.push_back(input_aid[input_aid.size() - 1].second[j]);

    }

  if (arrays.size() > 0) {
    for (int i = input_aid.size() - 2; i >= 0; i--) {

      int max_count = 0;
      for (int j = 0; j < input_aid[i].second.size(); j++)
        if (input_aid[i].second[j] != "") {
          max_count++;
        }
      if (max_count > 0) {
        for (int j = 0; j < max_count; j++) {
          std::string s = input_aid[i].second[j];
          bool found = false;
          for (int k = 0; k < max_count; k++)
            if (s == arrays[k])
              found = true;
          if (!found)
            throw chill::error::loop("guard condition not solvable");
        }
      } else {
        bool found = false;
        for (int k = 0; k < arrays.size(); k++)
          if (arrays[k] == input_aid[i].first)
            found = true;
        if (!found)
          arrays.push_back(input_aid[i].first);
      }
    }
  } else {

    for (int i = input_aid.size() - 1; i >= 0; i--) {
      arrays.push_back(input_aid[i].first);
    }
  }
  return arrays;
}



