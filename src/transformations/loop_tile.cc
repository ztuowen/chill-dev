/*
 * loop_tile.cc
 *
 *  Created on: Nov 12, 2012
 *      Author: anand
 */

#include <code_gen/codegen.h>
#include "loop.hh"
#include "omegatools.hh"
#include "ir_code.hh"
#include "chill_error.hh"

using namespace omega;


void Loop::tile(int stmt_num, int level, int tile_size, int outer_level,
                TilingMethodType method, int alignment_offset, int alignment_multiple) {
  // check for sanity of parameters
  if (tile_size < 0)
    throw std::invalid_argument("invalid tile size");
  if (alignment_multiple < 1 || alignment_offset < 0)
    throw std::invalid_argument("invalid alignment for tile");
  if (stmt_num < 0 || stmt_num >= stmt.size())
    throw std::invalid_argument("invalid statement " + to_string(stmt_num));
  if (level <= 0)
    throw std::invalid_argument("invalid loop level " + to_string(level));
  if (level > stmt[stmt_num].loop_level.size())
    throw std::invalid_argument(
        "there is no loop level " + to_string(level) + " for statement "
        + to_string(stmt_num));
  if (outer_level <= 0 || outer_level > level)
    throw std::invalid_argument(
        "invalid tile controlling loop level "
        + to_string(outer_level));

  // invalidate saved codegen computation
  delete last_compute_cgr_;
  last_compute_cgr_ = NULL;
  delete last_compute_cg_;
  last_compute_cg_ = NULL;

  int dim = 2 * level - 1;
  int outer_dim = 2 * outer_level - 1;
  std::vector<int> lex = getLexicalOrder(stmt_num);
  std::set<int> same_tiled_loop = getStatements(lex, dim - 1);
  std::set<int> same_tile_controlling_loop = getStatements(lex,
                                                           outer_dim - 1);

  for (std::set<int>::iterator i = same_tiled_loop.begin();
       i != same_tiled_loop.end(); i++) {
    for (DependenceGraph::EdgeList::iterator j =
        dep.vertex[*i].second.begin(); j != dep.vertex[*i].second.end();
         j++) {
      if (same_tiled_loop.find(j->first) != same_tiled_loop.end())
        for (int k = 0; k < j->second.size(); k++) {
          DependenceVector dv = j->second[k];
          int dim2 = level - 1;
          if ((dv.type != DEP_CONTROL) && (dv.type != DEP_UNKNOWN)) {
            while (stmt[*i].loop_level[dim2].type == LoopLevelTile) {
              dim2 = stmt[*i].loop_level[dim2].payload - 1;
            }
            dim2 = stmt[*i].loop_level[dim2].payload;

            if (dv.hasNegative(dim2) && (!dv.quasi)) {
              for (int l = outer_level; l < level; l++)
                if (stmt[*i].loop_level[l - 1].type
                    != LoopLevelTile) {
                  if (dv.isCarried(
                      stmt[*i].loop_level[l - 1].payload)
                      && dv.hasPositive(
                      stmt[*i].loop_level[l - 1].payload))
                    throw loop_error(
                        "loop error: Tiling is illegal, dependence violation!");
                } else {

                  int dim3 = l - 1;
                  while (stmt[*i].loop_level[l - 1].type
                         != LoopLevelTile) {
                    dim3 =
                        stmt[*i].loop_level[l - 1].payload
                        - 1;

                  }

                  dim3 = stmt[*i].loop_level[l - 1].payload;
                  if (dim3 < level - 1)
                    if (dv.isCarried(dim3)
                        && dv.hasPositive(dim3))
                      throw loop_error(
                          "loop error: Tiling is illegal, dependence violation!");
                }
            }
          }
        }
    }
  }
  // special case for no tiling
  if (tile_size == 0) {
    for (std::set<int>::iterator i = same_tile_controlling_loop.begin();
         i != same_tile_controlling_loop.end(); i++) {
      Relation r(stmt[*i].xform.n_out(), stmt[*i].xform.n_out() + 2);
      F_And *f_root = r.add_and();
      for (int j = 1; j <= 2 * outer_level - 1; j++) {
        EQ_Handle h = f_root->add_EQ();
        h.update_coef(r.input_var(j), 1);
        h.update_coef(r.output_var(j), -1);
      }
      EQ_Handle h1 = f_root->add_EQ();
      h1.update_coef(r.output_var(2 * outer_level), 1);
      EQ_Handle h2 = f_root->add_EQ();
      h2.update_coef(r.output_var(2 * outer_level + 1), 1);
      for (int j = 2 * outer_level; j <= stmt[*i].xform.n_out(); j++) {
        EQ_Handle h = f_root->add_EQ();
        h.update_coef(r.input_var(j), 1);
        h.update_coef(r.output_var(j + 2), -1);
      }

      stmt[*i].xform = Composition(copy(r), stmt[*i].xform);
    }
  }
    // normal tiling
  else {
    std::set<int> private_stmt;
    for (std::set<int>::iterator i = same_tile_controlling_loop.begin();
         i != same_tile_controlling_loop.end(); i++) {
      // should test dim's value directly but it is ok for now
      if (same_tiled_loop.find(*i) == same_tiled_loop.end()
          && overflow.find(*i) != overflow.end())
        private_stmt.insert(*i);
    }

    // extract the union of the iteration space to be considered
    Relation hull;
    {
      std::vector<Relation> r_list;

      for (std::set<int>::iterator i = same_tile_controlling_loop.begin();
           i != same_tile_controlling_loop.end(); i++)
        if (private_stmt.find(*i) == private_stmt.end()) {
          Relation r = getNewIS(*i);
          for (int j = dim + 2; j <= r.n_set(); j++)
            r = Project(r, r.set_var(j));
          for (int j = outer_dim; j < dim; j++)
            r = Project(r, j + 1, Set_Var);
          for (int j = 0; j < outer_dim; j += 2)
            r = Project(r, j + 1, Set_Var);
          r.simplify(2, 4);
          r_list.push_back(r);
        }

      hull = SimpleHull(r_list);
    }

    // extract the bound of the dimension to be tiled
    Relation bound = get_loop_bound(hull, dim);
    if (!bound.has_single_conjunct()) {
      // further simplify the bound
      hull = Approximate(hull);
      bound = get_loop_bound(hull, dim);

      int i = outer_dim - 2;
      while (!bound.has_single_conjunct() && i >= 0) {
        hull = Project(hull, i + 1, Set_Var);
        bound = get_loop_bound(hull, dim);
        i -= 2;
      }

      if (!bound.has_single_conjunct())
        throw loop_error("cannot handle tile bounds");
    }

    // separate lower and upper bounds
    std::vector<GEQ_Handle> lb_list, ub_list;
    {
      Conjunct *c = bound.query_DNF()->single_conjunct();
      for (GEQ_Iterator gi(c->GEQs()); gi; gi++) {
        int coef = (*gi).get_coef(bound.set_var(dim + 1));
        if (coef < 0)
          ub_list.push_back(*gi);
        else if (coef > 0)
          lb_list.push_back(*gi);
      }
    }
    if (lb_list.size() == 0)
      throw loop_error(
          "unable to calculate tile controlling loop lower bound");
    if (ub_list.size() == 0)
      throw loop_error(
          "unable to calculate tile controlling loop upper bound");

    // find the simplest lower bound for StridedTile or simplest iteration count for CountedTile
    int simplest_lb = 0, simplest_ub = 0;
    if (method == StridedTile) {
      int best_cost = INT_MAX;
      for (int i = 0; i < lb_list.size(); i++) {
        int cost = 0;
        for (Constr_Vars_Iter ci(lb_list[i]); ci; ci++) {
          switch ((*ci).var->kind()) {
            case Input_Var: {
              cost += 5;
              break;
            }
            case Global_Var: {
              cost += 2;
              break;
            }
            default:
              cost += 15;
              break;
          }
        }

        if (cost < best_cost) {
          best_cost = cost;
          simplest_lb = i;
        }
      }
    } else if (method == CountedTile) {
      std::map<Variable_ID, coef_t> s1, s2, s3;
      int best_cost = INT_MAX;
      for (int i = 0; i < lb_list.size(); i++)
        for (int j = 0; j < ub_list.size(); j++) {
          int cost = 0;

          for (Constr_Vars_Iter ci(lb_list[i]); ci; ci++) {
            switch ((*ci).var->kind()) {
              case Input_Var: {
                s1[(*ci).var] += (*ci).coef;
                break;
              }
              case Global_Var: {
                s2[(*ci).var] += (*ci).coef;
                break;
              }
              case Exists_Var:
              case Wildcard_Var: {
                s3[(*ci).var] += (*ci).coef;
                break;
              }
              default:
                cost = INT_MAX - 2;
                break;
            }
          }

          for (Constr_Vars_Iter ci(ub_list[j]); ci; ci++) {
            switch ((*ci).var->kind()) {
              case Input_Var: {
                s1[(*ci).var] += (*ci).coef;
                break;
              }
              case Global_Var: {
                s2[(*ci).var] += (*ci).coef;
                break;
              }
              case Exists_Var:
              case Wildcard_Var: {
                s3[(*ci).var] += (*ci).coef;
                break;
              }
              default:
                if (cost == INT_MAX - 2)
                  cost = INT_MAX - 1;
                else
                  cost = INT_MAX - 3;
                break;
            }
          }

          if (cost == 0) {
            for (std::map<Variable_ID, coef_t>::iterator k =
                s1.begin(); k != s1.end(); k++)
              if ((*k).second != 0)
                cost += 5;
            for (std::map<Variable_ID, coef_t>::iterator k =
                s2.begin(); k != s2.end(); k++)
              if ((*k).second != 0)
                cost += 2;
            for (std::map<Variable_ID, coef_t>::iterator k =
                s3.begin(); k != s3.end(); k++)
              if ((*k).second != 0)
                cost += 15;
          }

          if (cost < best_cost) {
            best_cost = cost;
            simplest_lb = i;
            simplest_ub = j;
          }
        }
    }

    // prepare the new transformation relations
    for (std::set<int>::iterator i = same_tile_controlling_loop.begin();
         i != same_tile_controlling_loop.end(); i++) {
      Relation r(stmt[*i].xform.n_out(), stmt[*i].xform.n_out() + 2);
      F_And *f_root = r.add_and();
      for (int j = 0; j < outer_dim - 1; j++) {
        EQ_Handle h = f_root->add_EQ();
        h.update_coef(r.output_var(j + 1), 1);
        h.update_coef(r.input_var(j + 1), -1);
      }

      for (int j = outer_dim - 1; j < stmt[*i].xform.n_out(); j++) {
        EQ_Handle h = f_root->add_EQ();
        h.update_coef(r.output_var(j + 3), 1);
        h.update_coef(r.input_var(j + 1), -1);
      }

      EQ_Handle h = f_root->add_EQ();
      h.update_coef(r.output_var(outer_dim), 1);
      h.update_const(-lex[outer_dim - 1]);

      stmt[*i].xform = Composition(r, stmt[*i].xform);
    }

    // add tiling constraints.
    for (std::set<int>::iterator i = same_tile_controlling_loop.begin();
         i != same_tile_controlling_loop.end(); i++) {
      F_And *f_super_root = stmt[*i].xform.and_with_and();
      F_Exists *f_exists = f_super_root->add_exists();
      F_And *f_root = f_exists->add_and();

      // create a lower bound variable for easy formula creation later
      Variable_ID aligned_lb;
      {
        Variable_ID lb = f_exists->declare();
        coef_t coef = lb_list[simplest_lb].get_coef(
            bound.set_var(dim + 1));
        if (coef == 1) { // e.g. if i >= m+5, then LB = m+5
          EQ_Handle h = f_root->add_EQ();
          h.update_coef(lb, 1);
          for (Constr_Vars_Iter ci(lb_list[simplest_lb]); ci; ci++) {
            switch ((*ci).var->kind()) {
              case Input_Var: {
                int pos = (*ci).var->get_position();
                if (pos != dim + 1)
                  h.update_coef(stmt[*i].xform.output_var(pos),
                                (*ci).coef);
                break;
              }
              case Global_Var: {
                Global_Var_ID g = (*ci).var->get_global_var();
                Variable_ID v;
                if (g->arity() == 0)
                  v = stmt[*i].xform.get_local(g);
                else
                  v = stmt[*i].xform.get_local(g,
                                               (*ci).var->function_of());
                h.update_coef(v, (*ci).coef);
                break;
              }
              default:
                throw loop_error("cannot handle tile bounds");
            }
          }
          h.update_const(lb_list[simplest_lb].get_const());
        } else { // e.g. if 2i >= m+5, then m+5 <= 2*LB < m+5+2
          GEQ_Handle h1 = f_root->add_GEQ();
          GEQ_Handle h2 = f_root->add_GEQ();
          for (Constr_Vars_Iter ci(lb_list[simplest_lb]); ci; ci++) {
            switch ((*ci).var->kind()) {
              case Input_Var: {
                int pos = (*ci).var->get_position();
                if (pos == dim + 1) {
                  h1.update_coef(lb, (*ci).coef);
                  h2.update_coef(lb, -(*ci).coef);
                } else {
                  h1.update_coef(stmt[*i].xform.output_var(pos),
                                 (*ci).coef);
                  h2.update_coef(stmt[*i].xform.output_var(pos),
                                 -(*ci).coef);
                }
                break;
              }
              case Global_Var: {
                Global_Var_ID g = (*ci).var->get_global_var();
                Variable_ID v;
                if (g->arity() == 0)
                  v = stmt[*i].xform.get_local(g);
                else
                  v = stmt[*i].xform.get_local(g,
                                               (*ci).var->function_of());
                h1.update_coef(v, (*ci).coef);
                h2.update_coef(v, -(*ci).coef);
                break;
              }
              default:
                throw loop_error("cannot handle tile bounds");
            }
          }
          h1.update_const(lb_list[simplest_lb].get_const());
          h2.update_const(-lb_list[simplest_lb].get_const());
          h2.update_const(coef - 1);
        }

        Variable_ID offset_lb;
        if (alignment_offset == 0)
          offset_lb = lb;
        else {
          EQ_Handle h = f_root->add_EQ();
          offset_lb = f_exists->declare();
          h.update_coef(offset_lb, 1);
          h.update_coef(lb, -1);
          h.update_const(alignment_offset);
        }

        if (alignment_multiple == 1) { // trivial
          aligned_lb = offset_lb;
        } else { // e.g. to align at 4, aligned_lb = 4*alpha && LB-4 < 4*alpha <= LB
          aligned_lb = f_exists->declare();
          Variable_ID e = f_exists->declare();

          EQ_Handle h = f_root->add_EQ();
          h.update_coef(aligned_lb, 1);
          h.update_coef(e, -alignment_multiple);

          GEQ_Handle h1 = f_root->add_GEQ();
          GEQ_Handle h2 = f_root->add_GEQ();
          h1.update_coef(e, alignment_multiple);
          h2.update_coef(e, -alignment_multiple);
          h1.update_coef(offset_lb, -1);
          h2.update_coef(offset_lb, 1);
          h1.update_const(alignment_multiple - 1);
        }
      }

      // create an upper bound variable for easy formula creation later
      Variable_ID ub = f_exists->declare();
      {
        coef_t coef = -ub_list[simplest_ub].get_coef(
            bound.set_var(dim + 1));
        if (coef == 1) { // e.g. if i <= m+5, then UB = m+5
          EQ_Handle h = f_root->add_EQ();
          h.update_coef(ub, -1);
          for (Constr_Vars_Iter ci(ub_list[simplest_ub]); ci; ci++) {
            switch ((*ci).var->kind()) {
              case Input_Var: {
                int pos = (*ci).var->get_position();
                if (pos != dim + 1)
                  h.update_coef(stmt[*i].xform.output_var(pos),
                                (*ci).coef);
                break;
              }
              case Global_Var: {
                Global_Var_ID g = (*ci).var->get_global_var();
                Variable_ID v;
                if (g->arity() == 0)
                  v = stmt[*i].xform.get_local(g);
                else
                  v = stmt[*i].xform.get_local(g,
                                               (*ci).var->function_of());
                h.update_coef(v, (*ci).coef);
                break;
              }
              default:
                throw loop_error("cannot handle tile bounds");
            }
          }
          h.update_const(ub_list[simplest_ub].get_const());
        } else { // e.g. if 2i <= m+5, then m+5-2 < 2*UB <= m+5
          GEQ_Handle h1 = f_root->add_GEQ();
          GEQ_Handle h2 = f_root->add_GEQ();
          for (Constr_Vars_Iter ci(ub_list[simplest_ub]); ci; ci++) {
            switch ((*ci).var->kind()) {
              case Input_Var: {
                int pos = (*ci).var->get_position();
                if (pos == dim + 1) {
                  h1.update_coef(ub, -(*ci).coef);
                  h2.update_coef(ub, (*ci).coef);
                } else {
                  h1.update_coef(stmt[*i].xform.output_var(pos),
                                 -(*ci).coef);
                  h2.update_coef(stmt[*i].xform.output_var(pos),
                                 (*ci).coef);
                }
                break;
              }
              case Global_Var: {
                Global_Var_ID g = (*ci).var->get_global_var();
                Variable_ID v;
                if (g->arity() == 0)
                  v = stmt[*i].xform.get_local(g);
                else
                  v = stmt[*i].xform.get_local(g,
                                               (*ci).var->function_of());
                h1.update_coef(v, -(*ci).coef);
                h2.update_coef(v, (*ci).coef);
                break;
              }
              default:
                throw loop_error("cannot handle tile bounds");
            }
          }
          h1.update_const(-ub_list[simplest_ub].get_const());
          h2.update_const(ub_list[simplest_ub].get_const());
          h1.update_const(coef - 1);
        }
      }

      // insert tile controlling loop constraints
      if (method == StridedTile) { // e.g. ii = LB + 32 * alpha && alpha >= 0
        Variable_ID e = f_exists->declare();
        GEQ_Handle h1 = f_root->add_GEQ();
        h1.update_coef(e, 1);

        EQ_Handle h2 = f_root->add_EQ();
        h2.update_coef(stmt[*i].xform.output_var(outer_dim + 1), 1);
        h2.update_coef(e, -tile_size);
        h2.update_coef(aligned_lb, -1);
      } else if (method == CountedTile) { // e.g. 0 <= ii < ceiling((UB-LB+1)/32)
        GEQ_Handle h1 = f_root->add_GEQ();
        h1.update_coef(stmt[*i].xform.output_var(outer_dim + 1), 1);

        GEQ_Handle h2 = f_root->add_GEQ();
        h2.update_coef(stmt[*i].xform.output_var(outer_dim + 1),
                       -tile_size);
        h2.update_coef(aligned_lb, -1);
        h2.update_coef(ub, 1);
      }

      // special care for private statements like overflow assignment
      if (private_stmt.find(*i) != private_stmt.end()) { // e.g. ii <= UB
        GEQ_Handle h = f_root->add_GEQ();
        h.update_coef(stmt[*i].xform.output_var(outer_dim + 1), -1);
        h.update_coef(ub, 1);
      }

        // restrict original loop index inside the tile
      else {
        if (method == StridedTile) { // e.g. ii <= i < ii + tile_size
          GEQ_Handle h1 = f_root->add_GEQ();
          h1.update_coef(stmt[*i].xform.output_var(dim + 3), 1);
          h1.update_coef(stmt[*i].xform.output_var(outer_dim + 1),
                         -1);

          GEQ_Handle h2 = f_root->add_GEQ();
          h2.update_coef(stmt[*i].xform.output_var(dim + 3), -1);
          h2.update_coef(stmt[*i].xform.output_var(outer_dim + 1), 1);
          h2.update_const(tile_size - 1);
        } else if (method == CountedTile) { // e.g. LB+32*ii <= i < LB+32*ii+tile_size
          GEQ_Handle h1 = f_root->add_GEQ();
          h1.update_coef(stmt[*i].xform.output_var(outer_dim + 1),
                         -tile_size);
          h1.update_coef(stmt[*i].xform.output_var(dim + 3), 1);
          h1.update_coef(aligned_lb, -1);

          GEQ_Handle h2 = f_root->add_GEQ();
          h2.update_coef(stmt[*i].xform.output_var(outer_dim + 1),
                         tile_size);
          h2.update_coef(stmt[*i].xform.output_var(dim + 3), -1);
          h2.update_const(tile_size - 1);
          h2.update_coef(aligned_lb, 1);
        }
      }
    }
  }

  // update loop level information
  for (std::set<int>::iterator i = same_tile_controlling_loop.begin();
       i != same_tile_controlling_loop.end(); i++) {
    for (int j = 1; j <= stmt[*i].loop_level.size(); j++)
      switch (stmt[*i].loop_level[j - 1].type) {
        case LoopLevelOriginal:
          break;
        case LoopLevelTile:
          if (stmt[*i].loop_level[j - 1].payload >= outer_level)
            stmt[*i].loop_level[j - 1].payload++;
          break;
        default:
          throw loop_error(
              "unknown loop level type for statement "
              + to_string(*i));
      }

    LoopLevel ll;
    ll.type = LoopLevelTile;
    ll.payload = level + 1;
    ll.parallel_level = 0;
    stmt[*i].loop_level.insert(
        stmt[*i].loop_level.begin() + (outer_level - 1), ll);
  }
}

