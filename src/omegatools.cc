/*****************************************************************************
 Copyright (C) 2008 University of Southern California
 Copyright (C) 2009-2010 University of Utah
 All Rights Reserved.

 Purpose:
   Useful tools involving Omega manipulation.

 Notes:

 History:
   01/2006 Created by Chun Chen.
   03/2009 Upgrade Omega's interaction with compiler to IR_Code, by Chun Chen.
*****************************************************************************/

#include <code_gen/codegen.h>
#include "omegatools.hh"
#include "ir_code.hh"
#include "chill_error.hh"

using namespace omega;

namespace {
  struct DependenceLevel {
    Relation r;
    int level;
    int dir; // direction upto current level:
    // -1:negative, 0: undetermined, 1: postive
    std::vector<coef_t> lbounds;
    std::vector<coef_t> ubounds;
    DependenceLevel(const Relation &_r, int _dims):
      r(_r), level(0), dir(0), lbounds(_dims), ubounds(_dims) {}
  };
}




std::string tmp_e() {
  static int counter = 1;
  return std::string("e")+to_string(counter++);
}

void exp2formula(IR_Code *ir, Relation &r, F_And *f_root, std::vector<Free_Var_Decl*> &freevars,
                 CG_outputRepr *repr, Variable_ID lhs, char side, IR_CONDITION_TYPE rel, bool destroy) {
  
  switch (ir->QueryExpOperation(repr)) {
  case IR_OP_CONSTANT:
  {
    std::vector<CG_outputRepr *> v = ir->QueryExpOperand(repr);
    IR_ConstantRef *ref = static_cast<IR_ConstantRef *>(ir->Repr2Ref(v[0]));
    if (!ref->is_integer())
      throw ir_exp_error("non-integer constant coefficient");
    
    coef_t c = ref->integer();
    if (rel == IR_COND_GE || rel == IR_COND_GT) {
      GEQ_Handle h = f_root->add_GEQ();
      h.update_coef(lhs, 1);
      if (rel == IR_COND_GE)
        h.update_const(-c);
      else
        h.update_const(-c-1);
    }
    else if (rel == IR_COND_LE || rel == IR_COND_LT) {
      GEQ_Handle h = f_root->add_GEQ();
      h.update_coef(lhs, -1);
      if (rel == IR_COND_LE)
        h.update_const(c);
      else
        h.update_const(c-1);
    }
    else if (rel == IR_COND_EQ) {
      EQ_Handle h = f_root->add_EQ();
      h.update_coef(lhs, 1);
      h.update_const(-c);
    }
    else
      throw std::invalid_argument("unsupported condition type");
    
    delete v[0];
    delete ref;
    if (destroy)
      delete repr;
    break;
  }
  case IR_OP_VARIABLE:
  {
    std::vector<CG_outputRepr *> v = ir->QueryExpOperand(repr);
    IR_ScalarRef *ref = static_cast<IR_ScalarRef *>(ir->Repr2Ref(v[0]));
    
    std::string s = ref->name();
    Variable_ID e = find_index(r, s, side);
    
    if (e == NULL) { // must be free variable
      Free_Var_Decl *t = NULL;
      for (unsigned i = 0; i < freevars.size(); i++) {
        std::string ss = freevars[i]->base_name();
        if (s == ss) {
          t = freevars[i];
          break;
        }
      }
      
      if (t == NULL) {
        t = new Free_Var_Decl(s);
        freevars.insert(freevars.end(), t);
      }
      
      e = r.get_local(t);
    }
    
    if (rel == IR_COND_GE || rel == IR_COND_GT) {
      GEQ_Handle h = f_root->add_GEQ();
      h.update_coef(lhs, 1);
      h.update_coef(e, -1);
      if (rel == IR_COND_GT)
        h.update_const(-1);
    }
    else if (rel == IR_COND_LE || rel == IR_COND_LT) {
      GEQ_Handle h = f_root->add_GEQ();
      h.update_coef(lhs, -1);
      h.update_coef(e, 1);
      if (rel == IR_COND_LT)
        h.update_const(-1);
    }
    else if (rel == IR_COND_EQ) {
      EQ_Handle h = f_root->add_EQ();
      h.update_coef(lhs, 1);
      h.update_coef(e, -1);
    }
    else
      throw std::invalid_argument("unsupported condition type");
    
    //  delete v[0];
    delete ref;
    if (destroy)
      delete repr;
    break;
  }
  case IR_OP_ASSIGNMENT:
  {
    std::vector<CG_outputRepr *> v = ir->QueryExpOperand(repr);
    exp2formula(ir, r, f_root, freevars, v[0], lhs, side, rel, true);
    if (destroy)
      delete repr;
    break;
  }
  case IR_OP_PLUS:
  {
    F_Exists *f_exists = f_root->add_exists();
    Variable_ID e1 = f_exists->declare(tmp_e());
    Variable_ID e2 = f_exists->declare(tmp_e());
    F_And *f_and = f_exists->add_and();
    
    if (rel == IR_COND_GE || rel == IR_COND_GT) {
      GEQ_Handle h = f_and->add_GEQ();
      h.update_coef(lhs, 1);
      h.update_coef(e1, -1);
      h.update_coef(e2, -1);
      if (rel == IR_COND_GT)
        h.update_const(-1);
    }
    else if (rel == IR_COND_LE || rel == IR_COND_LT) {
      GEQ_Handle h = f_and->add_GEQ();
      h.update_coef(lhs, -1);
      h.update_coef(e1, 1);
      h.update_coef(e2, 1);
      if (rel == IR_COND_LT)
        h.update_const(-1);
    }
    else if (rel == IR_COND_EQ) {
      EQ_Handle h = f_and->add_EQ();
      h.update_coef(lhs, 1);
      h.update_coef(e1, -1);
      h.update_coef(e2, -1);
    }
    else
      throw std::invalid_argument("unsupported condition type");
    
    std::vector<CG_outputRepr *> v = ir->QueryExpOperand(repr);
    exp2formula(ir, r, f_and, freevars, v[0], e1, side, IR_COND_EQ, true);
    exp2formula(ir, r, f_and, freevars, v[1], e2, side, IR_COND_EQ, true);
    if (destroy)
      delete repr;
    break;
  }
  case IR_OP_MINUS:
  {
    F_Exists *f_exists = f_root->add_exists();
    Variable_ID e1 = f_exists->declare(tmp_e());
    Variable_ID e2 = f_exists->declare(tmp_e());
    F_And *f_and = f_exists->add_and();
    
    if (rel == IR_COND_GE || rel == IR_COND_GT) {
      GEQ_Handle h = f_and->add_GEQ();
      h.update_coef(lhs, 1);
      h.update_coef(e1, -1);
      h.update_coef(e2, 1);
      if (rel == IR_COND_GT)
        h.update_const(-1);
    }
    else if (rel == IR_COND_LE || rel == IR_COND_LT) {
      GEQ_Handle h = f_and->add_GEQ();
      h.update_coef(lhs, -1);
      h.update_coef(e1, 1);
      h.update_coef(e2, -1);
      if (rel == IR_COND_LT)
        h.update_const(-1);
    }
    else if (rel == IR_COND_EQ) {
      EQ_Handle h = f_and->add_EQ();
      h.update_coef(lhs, 1);
      h.update_coef(e1, -1);
      h.update_coef(e2, 1);
    }
    else
      throw std::invalid_argument("unsupported condition type");
    
    std::vector<CG_outputRepr *> v = ir->QueryExpOperand(repr);
    exp2formula(ir, r, f_and, freevars, v[0], e1, side, IR_COND_EQ, true);
    exp2formula(ir, r, f_and, freevars, v[1], e2, side, IR_COND_EQ, true);
    if (destroy)
      delete repr;
    break;
  }
  case IR_OP_MULTIPLY:
  {
    std::vector<CG_outputRepr *> v = ir->QueryExpOperand(repr);
    
    coef_t coef;
    CG_outputRepr *term;
    if (ir->QueryExpOperation(v[0]) == IR_OP_CONSTANT) {
      IR_ConstantRef *ref = static_cast<IR_ConstantRef *>(ir->Repr2Ref(v[0]));
      coef = ref->integer();
      delete v[0];
      delete ref;
      term = v[1];
    }
    else if (ir->QueryExpOperation(v[1]) == IR_OP_CONSTANT) {
      IR_ConstantRef *ref = static_cast<IR_ConstantRef *>(ir->Repr2Ref(v[1]));
      coef = ref->integer();
      delete v[1];
      delete ref;
      term = v[0];
    }
    else
      throw ir_exp_error("not presburger expression");
    
    F_Exists *f_exists = f_root->add_exists();
    Variable_ID e = f_exists->declare(tmp_e());
    F_And *f_and = f_exists->add_and();
    
    if (rel == IR_COND_GE || rel == IR_COND_GT) {
      GEQ_Handle h = f_and->add_GEQ();
      h.update_coef(lhs, 1);
      h.update_coef(e, -coef);
      if (rel == IR_COND_GT)
        h.update_const(-1);
    }
    else if (rel == IR_COND_LE || rel == IR_COND_LT) {
      GEQ_Handle h = f_and->add_GEQ();
      h.update_coef(lhs, -1);
      h.update_coef(e, coef);
      if (rel == IR_COND_LT)
        h.update_const(-1);
    }
    else if (rel == IR_COND_EQ) {
      EQ_Handle h = f_and->add_EQ();
      h.update_coef(lhs, 1);
      h.update_coef(e, -coef);
    }
    else
      throw std::invalid_argument("unsupported condition type");
    
    exp2formula(ir, r, f_and, freevars, term, e, side, IR_COND_EQ, true);
    if (destroy)
      delete repr;
    break;
  }
  case IR_OP_DIVIDE:
  {
    std::vector<CG_outputRepr *> v = ir->QueryExpOperand(repr);
    
    assert(ir->QueryExpOperation(v[1]) == IR_OP_CONSTANT);
    IR_ConstantRef *ref = static_cast<IR_ConstantRef *>(ir->Repr2Ref(v[1]));
    coef_t coef = ref->integer();
    delete v[1];
    delete ref;
    
    F_Exists *f_exists = f_root->add_exists();
    Variable_ID e = f_exists->declare(tmp_e());
    F_And *f_and = f_exists->add_and();
    
    if (rel == IR_COND_GE || rel == IR_COND_GT) {
      GEQ_Handle h = f_and->add_GEQ();
      h.update_coef(lhs, coef);
      h.update_coef(e, -1);
      if (rel == IR_COND_GT)
        h.update_const(-1);
    }
    else if (rel == IR_COND_LE || rel == IR_COND_LT) {
      GEQ_Handle h = f_and->add_GEQ();
      h.update_coef(lhs, -coef);
      h.update_coef(e, 1);
      if (rel == IR_COND_LT)
        h.update_const(-1);
    }
    else if (rel == IR_COND_EQ) {
      EQ_Handle h = f_and->add_EQ();
      h.update_coef(lhs, coef);
      h.update_coef(e, -1);
    }
    else
      throw std::invalid_argument("unsupported condition type");
    
    exp2formula(ir, r, f_and, freevars, v[0], e, side, IR_COND_EQ, true);
    if (destroy)
      delete repr;
    break;
  }
  case IR_OP_POSITIVE:
  {
    std::vector<CG_outputRepr *> v = ir->QueryExpOperand(repr);
    
    exp2formula(ir, r, f_root, freevars, v[0], lhs, side, rel, true);
    if (destroy)
      delete repr;
    break;
  }
  case IR_OP_NEGATIVE:
  {
    std::vector<CG_outputRepr *> v = ir->QueryExpOperand(repr);
    
    F_Exists *f_exists = f_root->add_exists();
    Variable_ID e = f_exists->declare(tmp_e());
    F_And *f_and = f_exists->add_and();
    
    if (rel == IR_COND_GE || rel == IR_COND_GT) {
      GEQ_Handle h = f_and->add_GEQ();
      h.update_coef(lhs, 1);
      h.update_coef(e, 1);
      if (rel == IR_COND_GT)
        h.update_const(-1);
    }
    else if (rel == IR_COND_LE || rel == IR_COND_LT) {
      GEQ_Handle h = f_and->add_GEQ();
      h.update_coef(lhs, -1);
      h.update_coef(e, -1);
      if (rel == IR_COND_LT)
        h.update_const(-1);
    }
    else if (rel == IR_COND_EQ) {
      EQ_Handle h = f_and->add_EQ();
      h.update_coef(lhs, 1);
      h.update_coef(e, 1);
    }
    else
      throw std::invalid_argument("unsupported condition type");
    
    exp2formula(ir, r, f_and, freevars, v[0], e, side, IR_COND_EQ, true);
    if (destroy)
      delete repr;
    break;
  }
  case IR_OP_MIN:
  {
    std::vector<CG_outputRepr *> v = ir->QueryExpOperand(repr);
    
    F_Exists *f_exists = f_root->add_exists();
    
    if (rel == IR_COND_GE || rel == IR_COND_GT) {
      F_Or *f_or = f_exists->add_and()->add_or();
      for (int i = 0; i < v.size(); i++) {
        Variable_ID e = f_exists->declare(tmp_e());
        F_And *f_and = f_or->add_and();
        GEQ_Handle h = f_and->add_GEQ();
        h.update_coef(lhs, 1);
        h.update_coef(e, -1);
        if (rel == IR_COND_GT)
          h.update_const(-1);
        
        exp2formula(ir, r, f_and, freevars, v[i], e, side, IR_COND_EQ, true);
      }
    }
    else if (rel == IR_COND_LE || rel == IR_COND_LT) {
      F_And *f_and = f_exists->add_and();
      for (int i = 0; i < v.size(); i++) {
        Variable_ID e = f_exists->declare(tmp_e());        
        GEQ_Handle h = f_and->add_GEQ();
        h.update_coef(lhs, -1);
        h.update_coef(e, 1);
        if (rel == IR_COND_LT)
          h.update_const(-1);
        
        exp2formula(ir, r, f_and, freevars, v[i], e, side, IR_COND_EQ, true);
      }
    }
    else if (rel == IR_COND_EQ) {
      F_Or *f_or = f_exists->add_and()->add_or();
      for (int i = 0; i < v.size(); i++) {
        Variable_ID e = f_exists->declare(tmp_e());
        F_And *f_and = f_or->add_and();
        
        EQ_Handle h = f_and->add_EQ();
        h.update_coef(lhs, 1);
        h.update_coef(e, -1);
        
        exp2formula(ir, r, f_and, freevars, v[i], e, side, IR_COND_EQ, false);
        
        for (int j = 0; j < v.size(); j++)
          if (j != i) {
            Variable_ID e2 = f_exists->declare(tmp_e());
            GEQ_Handle h2 = f_and->add_GEQ();
            h2.update_coef(e, -1);
            h2.update_coef(e2, 1);
            
            exp2formula(ir, r, f_and, freevars, v[j], e2, side, IR_COND_EQ, false);
          }
      }
      
      for (int i = 0; i < v.size(); i++)
        delete v[i];
    }
    else
      throw std::invalid_argument("unsupported condition type");
    
    if (destroy)
      delete repr;
  }
  case IR_OP_MAX:
  {
    std::vector<CG_outputRepr *> v = ir->QueryExpOperand(repr);
    
    F_Exists *f_exists = f_root->add_exists();
    
    if (rel == IR_COND_LE || rel == IR_COND_LT) {
      F_Or *f_or = f_exists->add_and()->add_or();
      for (int i = 0; i < v.size(); i++) {
        Variable_ID e = f_exists->declare(tmp_e());
        F_And *f_and = f_or->add_and();
        GEQ_Handle h = f_and->add_GEQ();
        h.update_coef(lhs, -1);
        h.update_coef(e, 1);
        if (rel == IR_COND_LT)
          h.update_const(-1);
        
        exp2formula(ir, r, f_and, freevars, v[i], e, side, IR_COND_EQ, true);
      }
    }
    else if (rel == IR_COND_GE || rel == IR_COND_GT) {
      F_And *f_and = f_exists->add_and();
      for (int i = 0; i < v.size(); i++) {
        Variable_ID e = f_exists->declare(tmp_e());        
        GEQ_Handle h = f_and->add_GEQ();
        h.update_coef(lhs, 1);
        h.update_coef(e, -1);
        if (rel == IR_COND_GT)
          h.update_const(-1);
        
        exp2formula(ir, r, f_and, freevars, v[i], e, side, IR_COND_EQ, true);
      }
    }
    else if (rel == IR_COND_EQ) {
      F_Or *f_or = f_exists->add_and()->add_or();
      for (int i = 0; i < v.size(); i++) {
        Variable_ID e = f_exists->declare(tmp_e());
        F_And *f_and = f_or->add_and();
        
        EQ_Handle h = f_and->add_EQ();
        h.update_coef(lhs, 1);
        h.update_coef(e, -1);
        
        exp2formula(ir, r, f_and, freevars, v[i], e, side, IR_COND_EQ, false);
        
        for (int j = 0; j < v.size(); j++)
          if (j != i) {
            Variable_ID e2 = f_exists->declare(tmp_e());
            GEQ_Handle h2 = f_and->add_GEQ();
            h2.update_coef(e, 1);
            h2.update_coef(e2, -1);
            
            exp2formula(ir, r, f_and, freevars, v[j], e2, side, IR_COND_EQ, false);
          }
      }
      
      for (int i = 0; i < v.size(); i++)
        delete v[i];
    }
    else
      throw std::invalid_argument("unsupported condition type");
    
    if (destroy)
      delete repr;
  }
  case IR_OP_NULL:
    break;
  default:
    throw ir_exp_error("unsupported operand type");
  }
}

Relation arrays2relation(IR_Code *ir, std::vector<Free_Var_Decl*> &freevars,
                         const IR_ArrayRef *ref_src, const Relation &IS_w,
                         const IR_ArrayRef *ref_dst, const Relation &IS_r) {
  Relation &IS1 = const_cast<Relation &>(IS_w);
  Relation &IS2 = const_cast<Relation &>(IS_r);
  
  Relation r(IS1.n_set(), IS2.n_set());
  
  for (int i = 1; i <= IS1.n_set(); i++)
    r.name_input_var(i, IS1.set_var(i)->name());
  
  for (int i = 1; i <= IS2.n_set(); i++)
    r.name_output_var(i, IS2.set_var(i)->name()+"'");
  
  IR_Symbol *sym_src = ref_src->symbol();
  IR_Symbol *sym_dst = ref_dst->symbol();
  if (*sym_src != *sym_dst) {
    r.add_or(); // False Relation
    delete sym_src;
    delete sym_dst;
    return r;
  }
  else {
    delete sym_src;
    delete sym_dst;
  }
  
  F_And *f_root = r.add_and();
  
  for (int i = 0; i < ref_src->n_dim(); i++) {
    F_Exists *f_exists = f_root->add_exists();
    Variable_ID e1 = f_exists->declare(tmp_e());
    Variable_ID e2 = f_exists->declare(tmp_e());
    F_And *f_and = f_exists->add_and();
    
    CG_outputRepr *repr_src = ref_src->index(i);
    CG_outputRepr *repr_dst = ref_dst->index(i);
    
    bool has_complex_formula = false;
    try {
      exp2formula(ir, r, f_and, freevars, repr_src, e1, 'w', IR_COND_EQ, false);
      exp2formula(ir, r, f_and, freevars, repr_dst, e2, 'r', IR_COND_EQ, false);
    }
    catch (const ir_exp_error &e) {
      has_complex_formula = true;
    }
    
    if (!has_complex_formula) {
      EQ_Handle h = f_and->add_EQ();
      h.update_coef(e1, 1);
      h.update_coef(e2, -1);
    }
    
    repr_src->clear();
    repr_dst->clear();
    delete repr_src;
    delete repr_dst;
  }
  
  // add iteration space restriction
  r = Restrict_Domain(r, copy(IS1));
  r = Restrict_Range(r, copy(IS2));
  
  // reset the output variable names lost in restriction
  for (int i = 1; i <= IS2.n_set(); i++)
    r.name_output_var(i, IS2.set_var(i)->name()+"'");
  
  return r;
}

std::pair<std::vector<DependenceVector>, std::vector<DependenceVector> > relation2dependences (const IR_ArrayRef *ref_src, const IR_ArrayRef *ref_dst, const Relation &r) {
  assert(r.n_inp() == r.n_out());
  
  std::vector<DependenceVector> dependences1, dependences2;  
  std::stack<DependenceLevel> working;
  working.push(DependenceLevel(r, r.n_inp()));
  
  while (!working.empty()) {
    DependenceLevel dep = working.top();
    working.pop();
    
    // No dependence exists, move on.
    if (!dep.r.is_satisfiable())
      continue;
    
    if (dep.level == r.n_inp()) {
      DependenceVector dv;
      
      // for loop independent dependence, use lexical order to
      // determine the correct source and destination
      if (dep.dir == 0) {
        if (*ref_src == *ref_dst)
          continue; // trivial self zero-dependence
        
        if (ref_src->is_write()) {
          if (ref_dst->is_write())
            dv.type = DEP_W2W;
          else
            dv.type = DEP_W2R;
        }
        else {
          if (ref_dst->is_write())
            dv.type = DEP_R2W;
          else
            dv.type = DEP_R2R;
        }
        
      }
      else if (dep.dir == 1) {
        if (ref_src->is_write()) {
          if (ref_dst->is_write())
            dv.type = DEP_W2W;
          else
            dv.type = DEP_W2R;
        }
        else {
          if (ref_dst->is_write())
            dv.type = DEP_R2W;
          else
            dv.type = DEP_R2R;
        }
      }
      else { // dep.dir == -1
        if (ref_dst->is_write()) {
          if (ref_src->is_write())
            dv.type = DEP_W2W;
          else
            dv.type = DEP_W2R;
        }
        else {
          if (ref_src->is_write())
            dv.type = DEP_R2W;
          else
            dv.type = DEP_R2R;
        }
      }
      
      dv.lbounds = dep.lbounds;
      dv.ubounds = dep.ubounds;
      dv.sym = ref_src->symbol();
      
      if (dep.dir == 0 || dep.dir == 1)
        dependences1.push_back(dv);
      else
        dependences2.push_back(dv);
    }
    else {
      // now work on the next dimension level
      int level = ++dep.level;
      
      coef_t lbound, ubound;
      Relation delta = Deltas(copy(dep.r));
      delta.query_variable_bounds(delta.set_var(level), lbound, ubound);
      
      if (dep.dir == 0) {
        if (lbound > 0) {
          dep.dir = 1;
          dep.lbounds[level-1] = lbound;
          dep.ubounds[level-1] = ubound;
          
          working.push(dep);
        }
        else if (ubound < 0) {
          dep.dir = -1;
          dep.lbounds[level-1] = -ubound;
          dep.ubounds[level-1] = -lbound;
          
          working.push(dep);
        }
        else {
          // split the dependence vector into flow- and anti-dependence
          // for the first non-zero distance, also separate zero distance
          // at this level.
          {
            DependenceLevel dep2 = dep;
            
            dep2.lbounds[level-1] =  0;
            dep2.ubounds[level-1] =  0;
            
            F_And *f_root = dep2.r.and_with_and();
            EQ_Handle h = f_root->add_EQ();
            h.update_coef(dep2.r.input_var(level), 1);
            h.update_coef(dep2.r.output_var(level), -1);
            
            working.push(dep2);
          }
          
          if (lbound < 0 && *ref_src != *ref_dst) {
            DependenceLevel dep2 = dep;
            
            F_And *f_root = dep2.r.and_with_and();
            GEQ_Handle h = f_root->add_GEQ();
            h.update_coef(dep2.r.input_var(level), 1);
            h.update_coef(dep2.r.output_var(level), -1);
            h.update_const(-1);
            
            // get tighter bounds under new constraints
            coef_t lbound, ubound;
            delta = Deltas(copy(dep2.r));
            delta.query_variable_bounds(delta.set_var(level),
                                        lbound, ubound);
            
            dep2.dir = -1;            
            dep2.lbounds[level-1] = max(-ubound,static_cast<coef_t>(1)); // use max() to avoid Omega retardness
            dep2.ubounds[level-1] = -lbound;
            
            working.push(dep2);
          }
          
          if (ubound > 0) {
            DependenceLevel dep2 = dep;
            
            F_And *f_root = dep2.r.and_with_and();
            GEQ_Handle h = f_root->add_GEQ();
            h.update_coef(dep2.r.input_var(level), -1);
            h.update_coef(dep2.r.output_var(level), 1);
            h.update_const(-1);
            
            // get tighter bonds under new constraints
            coef_t lbound, ubound;
            delta = Deltas(copy(dep2.r));
            delta.query_variable_bounds(delta.set_var(level),
                                        lbound, ubound);
            dep2.dir = 1;
            dep2.lbounds[level-1] = max(lbound,static_cast<coef_t>(1)); // use max() to avoid Omega retardness
            dep2.ubounds[level-1] = ubound;
            
            working.push(dep2);
          }
        }
      }
      // now deal with dependence vector with known direction
      // determined at previous levels
      else {
        // For messy bounds, further test to see if the dependence distance
        // can be reduced to positive/negative.  This is an omega hack.
        if (lbound == negInfinity && ubound == posInfinity) {
          {
            Relation t = dep.r;
            F_And *f_root = t.and_with_and();
            GEQ_Handle h = f_root->add_GEQ();
            h.update_coef(t.input_var(level), 1);
            h.update_coef(t.output_var(level), -1);
            h.update_const(-1);
            
            if (!t.is_satisfiable()) {
              lbound = 0;
            }
          }
          {
            Relation t = dep.r;
            F_And *f_root = t.and_with_and();
            GEQ_Handle h = f_root->add_GEQ();
            h.update_coef(t.input_var(level), -1);
            h.update_coef(t.output_var(level), 1);
            h.update_const(-1);
            
            if (!t.is_satisfiable()) {
              ubound = 0;
            }
          }
        }
        
        // Same thing as above, test to see if zero dependence
        // distance possible.
        if (lbound == 0 || ubound == 0) {
          Relation t = dep.r;
          F_And *f_root = t.and_with_and();
          EQ_Handle h = f_root->add_EQ();
          h.update_coef(t.input_var(level), 1);
          h.update_coef(t.output_var(level), -1);
          
          if (!t.is_satisfiable()) {
            if (lbound == 0)
              lbound = 1;
            if (ubound == 0)
              ubound = -1;
          }
        }
        
        if (dep.dir == -1) {
          dep.lbounds[level-1] = -ubound;
          dep.ubounds[level-1] = -lbound;
        }
        else { // dep.dir == 1
          dep.lbounds[level-1] = lbound;
          dep.ubounds[level-1] = ubound;
        }
        
        working.push(dep);
      }
    }
  }
  
  return std::make_pair(dependences1, dependences2);
}

void exp2constraint(IR_Code *ir, Relation &r, F_And *f_root,
                    std::vector<Free_Var_Decl *> &freevars,
                    CG_outputRepr *repr, bool destroy) {
  IR_CONDITION_TYPE cond = ir->QueryBooleanExpOperation(repr);
  switch (cond) {
  case IR_COND_LT:
  case IR_COND_LE:
  case IR_COND_EQ:
  case IR_COND_GT:
  case IR_COND_GE: {
    F_Exists *f_exist = f_root->add_exists();
    Variable_ID e = f_exist->declare();
    F_And *f_and = f_exist->add_and();
    std::vector<omega::CG_outputRepr *> op = ir->QueryExpOperand(repr);
    exp2formula(ir, r, f_and, freevars, op[0], e, 's', IR_COND_EQ, true);
    exp2formula(ir, r, f_and, freevars, op[1], e, 's', cond, true);
    if (destroy)
      delete repr;
    break;
  }
  case IR_COND_NE: {
    F_Exists *f_exist = f_root->add_exists();
    Variable_ID e = f_exist->declare();
    F_Or *f_or = f_exist->add_or();
    F_And *f_and = f_or->add_and();
    std::vector<omega::CG_outputRepr *> op = ir->QueryExpOperand(repr);
    exp2formula(ir, r, f_and, freevars, op[0], e, 's', IR_COND_EQ, false);
    exp2formula(ir, r, f_and, freevars, op[1], e, 's', IR_COND_GT, false);
    
    f_and = f_or->add_and();
    exp2formula(ir, r, f_and, freevars, op[0], e, 's', IR_COND_EQ, true);
    exp2formula(ir, r, f_and, freevars, op[1], e, 's', IR_COND_LT, true);
    
    if (destroy)
      delete repr;
    break;
  }    
  default:
    throw ir_exp_error("unrecognized conditional expression");
  }
}

bool is_single_loop_iteration(const Relation &r, int level, const Relation &known) {
  int n = r.n_set();
  Relation r1 = Intersection(copy(r), Extend_Set(copy(known), n-known.n_set()));
  
  Relation mapping(n, n);
  F_And *f_root = mapping.add_and();
  for (int i = 1; i <= level; i++) {
    EQ_Handle h = f_root->add_EQ();
    h.update_coef(mapping.input_var(i), 1);
    h.update_coef(mapping.output_var(i), -1);
  }
  r1 = Range(Restrict_Domain(mapping, r1));
  r1.simplify();
  
  Variable_ID v = r1.set_var(level);
  for (DNF_Iterator di(r1.query_DNF()); di; di++) {
    bool is_single = false;
    for (EQ_Iterator ei((*di)->EQs()); ei; ei++)
      if ((*ei).get_coef(v) != 0 && !(*ei).has_wildcards()) {
        is_single = true;
        break;
      }
    
    if (!is_single)
      return false;
  }
  
  return true;
}


bool is_single_iteration(const Relation &r, int dim) {
  assert(r.is_set());
  const int n = r.n_set();
  
  if (dim >= n)
    return true;
  
  Relation bound = get_loop_bound(r, dim);
  
  for (DNF_Iterator di(bound.query_DNF()); di; di++) {
    bool is_single = false;
    for (EQ_Iterator ei((*di)->EQs()); ei; ei++)
      if (!(*ei).has_wildcards()) {
        is_single = true;
        break;
      }
    
    if (!is_single)
      return false;
  }
  
  return true;
}

void assign_const(Relation &r, int dim, int val) {
  const int n = r.n_out();
  
  Relation mapping(n, n);
  F_And *f_root = mapping.add_and();
  
  for (int i = 1; i <= n; i++) {
    if (i != dim+1) {
      EQ_Handle h = f_root->add_EQ();
      h.update_coef(mapping.output_var(i), 1);
      h.update_coef(mapping.input_var(i), -1);
    }
    else {
      EQ_Handle h = f_root->add_EQ();
      h.update_coef(mapping.output_var(i), 1);
      h.update_const(-val);
    }
  }
  
  r = Composition(mapping, r);
}


int get_const(const Relation &r, int dim, Var_Kind type) {
  Relation &rr = const_cast<Relation &>(r);
  
  Variable_ID v;
  switch (type) {
  case Input_Var:
    v = rr.input_var(dim+1);
    break;
  case Output_Var:
    v = rr.output_var(dim+1);
    break;
  default:
    throw std::invalid_argument("unsupported variable type");
  }
  
  for (DNF_Iterator di(rr.query_DNF()); di; di++)
    for (EQ_Iterator ei = (*di)->EQs(); ei; ei++)
      if ((*ei).is_const(v))
        return (*ei).get_const();
  
  throw std::runtime_error("cannot get variable's constant value");
}

Relation get_loop_bound(const Relation &r, int dim) {
  assert(r.is_set());
  const int n = r.n_set();
  
  Relation mapping(n,n);
  F_And *f_root = mapping.add_and();
  for (int i = 1; i <= dim+1; i++) {
    EQ_Handle h = f_root->add_EQ();
    h.update_coef(mapping.input_var(i), 1);
    h.update_coef(mapping.output_var(i), -1);
  }
  Relation r1 = Range(Restrict_Domain(mapping, copy(r)));
  for (int i = 1; i <= n; i++)
    r1.name_set_var(i, const_cast<Relation &>(r).set_var(i)->name());
  r1.setup_names();
  Relation r2 = Project(copy(r1), dim+1, Set_Var);
  
  return Gist(r1, r2, 1);
}

Relation get_loop_bound(const Relation &r, int level, const Relation &known) {
  int n = r.n_set();
  Relation r1 = Intersection(copy(r), Extend_Set(copy(known), n-known.n_set()));
  
  Relation mapping(n, n);
  F_And *f_root = mapping.add_and();
  for (int i = 1; i <= level; i++) {
    EQ_Handle h = f_root->add_EQ();
    h.update_coef(mapping.input_var(i), 1);
    h.update_coef(mapping.output_var(i), -1);
  }
  r1 = Range(Restrict_Domain(mapping, r1));
  Relation r2 = Project(copy(r1), level, Set_Var);
  r1 = Gist(r1, r2, 1);
  
  for (int i = 1; i <= n; i++)
    r1.name_set_var(i, const_cast<Relation &>(r).set_var(i)->name());
  r1.setup_names();
  
  return r1;
}



Relation get_max_loop_bound(const std::vector<Relation> &r, int dim) {
  if (r.size() == 0)
    return Relation::Null();
  
  const int n = r[0].n_set();
  Relation res(Relation::False(n));
  for (int i = 0; i < r.size(); i++) {
    Relation &t = const_cast<Relation &>(r[i]);
    if (t.is_satisfiable())
      res = Union(get_loop_bound(t, dim), res);
  }
  
  res.simplify();
  
  return res;
}

Relation get_min_loop_bound(const std::vector<Relation> &r, int dim) {
  if (r.size() == 0)
    return Relation::Null();
  
  const int n = r[0].n_set();
  Relation res(Relation::True(n));
  for (int i = 0; i < r.size(); i++) {
    Relation &t = const_cast<Relation &>(r[i]);
    if (t.is_satisfiable())
      res = Intersection(get_loop_bound(t, dim), res);
  }
  
  res.simplify();
  
  return res;
}

void add_loop_stride(Relation &r, const Relation &bound_, int dim, int stride) {
  F_And *f_root = r.and_with_and();
  Relation &bound = const_cast<Relation &>(bound_);
  for (DNF_Iterator di(bound.query_DNF()); di; di++) {
    F_Exists *f_exists = f_root->add_exists();
    Variable_ID e1 = f_exists->declare(tmp_e());
    Variable_ID e2 = f_exists->declare(tmp_e());
    F_And *f_and = f_exists->add_and();
    EQ_Handle stride_eq = f_and->add_EQ();
    stride_eq.update_coef(e1, 1);
    stride_eq.update_coef(e2, stride);
    if (!r.is_set())
      stride_eq.update_coef(r.output_var(dim+1), -1);
    else
      stride_eq.update_coef(r.set_var(dim+1), -1);
    F_Or *f_or = f_and->add_or();
    
    for (GEQ_Iterator gi = (*di)->GEQs(); gi; gi++) {
      if ((*gi).get_coef(bound.set_var(dim+1)) > 0) {
        // copy the lower bound constraint
        EQ_Handle h1 = f_or->add_and()->add_EQ();
        GEQ_Handle h2 = f_and->add_GEQ();
        for (Constr_Vars_Iter ci(*gi); ci; ci++) {
          switch ((*ci).var->kind()) {
            // case Set_Var:
          case Input_Var: {
            int pos = (*ci).var->get_position();
            if (pos == dim + 1) {
              h1.update_coef(e1, (*ci).coef);
              h2.update_coef(e1, (*ci).coef);
            }
            else {
              if (!r.is_set()) {
                h1.update_coef(r.output_var(pos), (*ci).coef);
                h2.update_coef(r.output_var(pos), (*ci).coef);
              }
              else {
                h1.update_coef(r.set_var(pos), (*ci).coef);
                h2.update_coef(r.set_var(pos), (*ci).coef);
              }                
            }
            break;
          }
          case Global_Var: {
            Global_Var_ID g = (*ci).var->get_global_var();
            h1.update_coef(r.get_local(g, (*ci).var->function_of()), (*ci).coef);
            h2.update_coef(r.get_local(g, (*ci).var->function_of()), (*ci).coef);
            break;
          }
          default:
            break;
          }
        }
        h1.update_const((*gi).get_const());
        h2.update_const((*gi).get_const());
      }
    }
  }
}


bool is_inner_loop_depend_on_level(const Relation &r, int level, const Relation &known) {
  Relation r1 = Intersection(copy(r), Extend_Set(copy(known), r.n_set()-known.n_set()));
  Relation r2 = copy(r1);
  for (int i = level+1; i <= r2.n_set(); i++)
    r2 = Project(r2, r2.set_var(i));
  r2.simplify(2, 4);
  Relation r3 = Gist(r1, r2);
  
  Variable_ID v = r3.set_var(level);
  for (DNF_Iterator di(r3.query_DNF()); di; di++) {
    for (EQ_Iterator ei = (*di)->EQs(); ei; ei++)
      if ((*ei).get_coef(v) != 0)
        return true;
    
    for (GEQ_Iterator gi = (*di)->GEQs(); gi; gi++)
      if ((*gi).get_coef(v) != 0)
        return true;
  }
  
  return false;
}

Relation adjust_loop_bound(const Relation &r, int level, int adjustment) {
  if (adjustment == 0)
    return copy(r);
  
  const int n = r.n_set();
  Relation r1 = copy(r);
  for (int i = level+1; i <= r1.n_set(); i++)
    r1 = Project(r1, r1.set_var(i));
  r1.simplify(2, 4);
  Relation r2 = Gist(copy(r), copy(r1));
  
  Relation mapping(n, n);
  F_And *f_root = mapping.add_and();
  for (int i = 1; i <= n; i++)
    if (i == level) {
      EQ_Handle h = f_root->add_EQ();
      h.update_coef(mapping.input_var(level), -1);
      h.update_coef(mapping.output_var(level), 1);
      h.update_const(static_cast<coef_t>(adjustment));
    }
    else {
      EQ_Handle h = f_root->add_EQ();
      h.update_coef(mapping.input_var(i), -1);
      h.update_coef(mapping.output_var(i), 1);
    }
  
  r2 = Range(Restrict_Domain(mapping, r2));
  r1 = Intersection(r1, r2);
  r1.simplify();
  
  for (int i = 1; i <= n; i++)
    r1.name_set_var(i, const_cast<Relation &>(r).set_var(i)->name());
  r1.setup_names();
  return r1;
}

Relation permute_relation(const std::vector<int> &pi) {
  const int n = pi.size();
  
  Relation r(n, n);
  F_And *f_root = r.add_and();
  
  for (int i = 0; i < n; i++) {    
    EQ_Handle h = f_root->add_EQ();
    h.update_coef(r.output_var(i+1), 1);
    h.update_coef(r.input_var(pi[i]+1), -1);
  }
  
  return r;
}

Variable_ID find_index(Relation &r, const std::string &s, char side) {
  // Omega quirks: assure the names are propagated inside the relation
  r.setup_names();
  
  if (r.is_set()) { // side == 's'
    for (int i = 1; i <= r.n_set(); i++) {
      std::string ss = r.set_var(i)->name();
      if (s == ss) {
        return r.set_var(i);
      }
    }
  }
  else if (side == 'w') {
    for (int i = 1; i <= r.n_inp(); i++) {
      std::string ss = r.input_var(i)->name();
      if (s == ss) {
        return r.input_var(i);
      }
    }
  }
  else { // side == 'r'
    for (int i = 1; i <= r.n_out(); i++) {
      std::string ss = r.output_var(i)->name();
      if (s+"'" == ss) {
        return r.output_var(i);
      }
    }
  }
  
  return NULL;
}

