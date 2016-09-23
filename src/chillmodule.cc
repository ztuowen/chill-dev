#include "chilldebug.h"

#include <parseRel.hh>

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <omega.h>
#include "loop.hh"
#include "ir_code.hh"

#include "chillmodule.hh"

using namespace omega;

extern Loop *myloop;
extern IR_Code *ir_code;
extern bool is_interactive;

std::string procedure_name;
std::string source_filename;

int loop_start_num;
int loop_end_num;

extern std::vector<IR_Control *> ir_controls;
extern std::vector<int> loops;

// ----------------------- //
// CHiLL support functions //
// ----------------------- //
// not sure yet if this actually needs to be exposed to the python interface
// these four functions are here to maintain similarity to the Lua interface
int get_loop_num_start() {
  return loop_start_num;
}

int get_loop_num_end() {
  return loop_end_num;
}

static void set_loop_num_start(int start_num) {
  loop_start_num = start_num;
}

static void set_loop_num_end(int end_num) {
  loop_end_num = end_num;
}

void finalize_loop(int loop_num_start, int loop_num_end) {
  if (loop_num_start == loop_num_end) {
    ir_code->ReplaceCode(ir_controls[loops[loop_num_start]], myloop->getCode());
    ir_controls[loops[loop_num_start]] = NULL;
  } else {
    std::vector<IR_Control *> parm;
    for (int i = loops[loop_num_start]; i <= loops[loop_num_end]; i++)
      parm.push_back(ir_controls[i]);
    IR_Block *block = ir_code->MergeNeighboringControlStructures(parm);
    ir_code->ReplaceCode(block, myloop->getCode());
    for (int i = loops[loop_num_start]; i <= loops[loop_num_end]; i++) {
      delete ir_controls[i];
      ir_controls[i] = NULL;
    }
  }
  delete myloop;
}

void finalize_loop() {
  int loop_num_start = get_loop_num_start();
  int loop_num_end = get_loop_num_end();
  finalize_loop(loop_num_start, loop_num_end);
}

static void init_loop(int loop_num_start, int loop_num_end) {
  if (source_filename.empty()) {
    CHILL_ERROR("source file not set when initializing the loop");
    if (!is_interactive)
      exit(2);
  } else {
    if (ir_code == NULL) {
      ir_code = new IR_clangCode(source_filename.c_str(), procedure_name.c_str());
      IR_Block *block = ir_code->GetCode();
      ir_controls = ir_code->FindOneLevelControlStructure(block);
      for (int i = 0; i < ir_controls.size(); i++) {
        if (ir_controls[i]->type() == IR_CONTROL_LOOP)
          loops.push_back(i);
      }
      delete block;
    }
    if (myloop != NULL && myloop->isInitialized()) {
      finalize_loop();
    }
  }
  set_loop_num_start(loop_num_start);
  set_loop_num_end(loop_num_end);
  if (loop_num_end < loop_num_start) {
    CHILL_ERROR("the last loop must be after the start loop");
    if (!is_interactive)
      exit(2);
  }
  if (loop_num_end >= loops.size()) {
    CHILL_ERROR("loop %d does not exist", loop_num_end);
    if (!is_interactive)
      exit(2);
  }
  std::vector<IR_Control *> parm;
  for (int i = loops[loop_num_start]; i <= loops[loop_num_end]; i++) {
    if (ir_controls[i] == NULL) {
      CHILL_ERROR("loop has already been processed");
      if (!is_interactive)
        exit(2);
    }
    parm.push_back(ir_controls[i]);
  }
  IR_Block *block = ir_code->MergeNeighboringControlStructures(parm);
  myloop = new Loop(block);
  delete block;
}

// ----------------------- //
// Python support funcions //
// ----------------------- //

// -- CHiLL support -- //
static void strict_arg_num(PyObject *args, int arg_num, const char *fname = NULL) {
  int arg_given = PyTuple_Size(args);
  char msg[128];
  if (arg_num != arg_given) {
    if (fname)
      sprintf(msg, "%s: expected %i arguments, was given %i.", fname, arg_num, arg_given);
    else
      sprintf(msg, "Expected %i argumets, was given %i.", arg_num, arg_given);
    throw std::runtime_error(msg);
  }
}

static int strict_arg_range(PyObject *args, int arg_min, int arg_max, const char *fname = NULL) {
  int arg_given = PyTuple_Size(args);
  char msg[128];
  if (arg_given < arg_min || arg_given > arg_max) {
    if (fname)
      sprintf(msg, "%s: expected %i to %i arguments, was given %i.", fname, arg_min, arg_max, arg_given);
    else
      sprintf(msg, "Expected %i to %i, argumets, was given %i.", arg_min, arg_max, arg_given);
    throw std::runtime_error(msg);
  }
  return arg_given;
}

static int intArg(PyObject *args, int index, int dval = 0) {
  if (PyTuple_Size(args) <= index)
    return dval;
  int ival;
  PyObject *item = PyTuple_GetItem(args, index);
  Py_INCREF(item);
  if (PyInt_Check(item)) ival = PyInt_AsLong(item);
  else {
    CHILL_ERROR("argument at index %i is not an int\n", index);
    exit(-1);
  }
  return ival;
}

static std::string strArg(PyObject *args, int index, const char *dval = NULL) {
  if (PyTuple_Size(args) <= index)
    return dval;
  std::string strval;
  PyObject *item = PyTuple_GetItem(args, index);
  Py_INCREF(item);
  if (PyString_Check(item)) strval = strdup(PyString_AsString(item));
  else {
    CHILL_ERROR("argument at index %i is not an string\n", index);
    exit(-1);
  }
  return strval;
}

static bool boolArg(PyObject *args, int index, bool dval = false) {
  if (PyTuple_Size(args) <= index)
    return dval;
  bool bval;
  PyObject *item = PyTuple_GetItem(args, index);
  Py_INCREF(item);
  return (bool) PyObject_IsTrue(item);
}

static bool tostringintmapvector(PyObject *args, int index, std::vector<std::map<std::string, int> > &vec) {
  if (PyTuple_Size(args) <= index)
    return false;
  PyObject *seq = PyTuple_GetItem(args, index);
  //TODO: Typecheck
  int seq_len = PyList_Size(seq);
  for (int i = 0; i < seq_len; i++) {
    std::map<std::string, int> map;
    PyObject *dict = PyList_GetItem(seq, i);
    PyObject *keys = PyDict_Keys(dict);
    //TODO: Typecheck
    int dict_len = PyList_Size(keys);
    for (int j = 0; j < dict_len; j++) {
      PyObject *key = PyList_GetItem(keys, j);
      PyObject *value = PyDict_GetItem(dict, key);
      std::string str_key = strdup(PyString_AsString(key));
      int int_value = PyInt_AsLong(value);
      map[str_key] = int_value;
    }
    vec.push_back(map);
  }
  return true;
}

static bool tointvector(PyObject *seq, std::vector<int> &vec) {
  //TODO: Typecheck
  int seq_len = PyList_Size(seq);
  for (int i = 0; i < seq_len; i++) {
    PyObject *item = PyList_GetItem(seq, i);
    vec.push_back(PyInt_AsLong(item));
  }
  return true;
}

static bool tointvector(PyObject *args, int index, std::vector<int> &vec) {
  if (PyTuple_Size(args) <= index)
    return false;
  PyObject *seq = PyTuple_GetItem(args, index);
  return tointvector(seq, vec);
}

static bool tointset(PyObject *args, int index, std::set<int> &set) {
  if (PyTuple_Size(args) <= index)
    return false;
  PyObject *seq = PyTuple_GetItem(args, index);
  //TODO: Typecheck
  int seq_len = PyList_Size(seq);
  for (int i = 0; i < seq_len; i++) {
    PyObject *item = PyList_GetItem(seq, i);
    set.insert(PyInt_AsLong(item));
  }
  return true;
}

static bool tointmatrix(PyObject *args, int index, std::vector<std::vector<int> > &mat) {
  if (PyTuple_Size(args) <= index)
    return false;
  PyObject *seq_one = PyTuple_GetItem(args, index);
  int seq_one_len = PyList_Size(seq_one);
  for (int i = 0; i < seq_one_len; i++) {
    std::vector<int> vec;
    PyObject *seq_two = PyList_GetItem(seq_one, i);
    int seq_two_len = PyList_Size(seq_two);
    for (int j = 0; j < seq_two_len; j++) {
      PyObject *item = PyList_GetItem(seq_two, j);
      vec.push_back(PyInt_AsLong(item));
    }
    mat.push_back(vec);
  }
  return true;
}

// ------------------------- //
// CHiLL interface functions //
// ------------------------- //

static PyObject *chill_source(PyObject *self, PyObject *args) {
  strict_arg_num(args, 1, "source");
  source_filename = strArg(args, 0);
  Py_RETURN_NONE;
}

static PyObject *chill_procedure(PyObject *self, PyObject *args) {
  if (!procedure_name.empty()) {
    CHILL_ERROR("only one procedure can be handled in a script");
    if (!is_interactive)
      exit(2);
  }
  procedure_name = strArg(args, 0);
  Py_RETURN_NONE;
}

static PyObject *chill_loop(PyObject *self, PyObject *args) {
  // loop (n)
  // loop (n:m)

  int nargs = PyTuple_Size(args);
  int start_num;
  int end_num;
  if (nargs == 1) {
    start_num = intArg(args, 0);
    end_num = start_num;
  } else if (nargs == 2) {
    start_num = intArg(args, 0);
    end_num = intArg(args, 1);
  } else {
    CHILL_ERROR("loop takes one or two arguments");
    if (!is_interactive)
      exit(2);
  }
  set_loop_num_start(start_num);
  set_loop_num_end(end_num);
  init_loop(start_num, end_num);
  Py_RETURN_NONE;
}

static PyObject *chill_print_code(PyObject *self, PyObject *args) {
  strict_arg_num(args, 0, "print_code");
  myloop->printCode();
  printf("\n");
  Py_RETURN_NONE;
}

static PyObject *chill_print_dep(PyObject *self, PyObject *args) {
  strict_arg_num(args, 0, "print_dep");
  myloop->printDependenceGraph();
  Py_RETURN_NONE;
}

static PyObject *chill_print_space(PyObject *self, PyObject *args) {
  strict_arg_num(args, 0, "print_space");
  myloop->printIterationSpace();
  Py_RETURN_NONE;
}

static void add_known(std::string cond_expr) {
  int num_dim = myloop->known.n_set();
  std::vector<std::map<std::string, int> > *cond;
  cond = parse_relation_vector(cond_expr.c_str());

  Relation rel(num_dim);
  F_And *f_root = rel.add_and();
  for (int j = 0; j < cond->size(); j++) {
    GEQ_Handle h = f_root->add_GEQ();
    for (std::map<std::string, int>::iterator it = (*cond)[j].begin(); it != (*cond)[j].end(); it++) {
      try {
        int dim = from_string<int>(it->first);
        if (dim == 0)
          h.update_const(it->second);
        else
          throw std::invalid_argument("only symbolic variables are allowed in known condition");
      }
      catch (std::ios::failure e) {
        Free_Var_Decl *g = NULL;
        for (unsigned i = 0; i < myloop->freevar.size(); i++) {
          std::string name = myloop->freevar[i]->base_name();
          if (name == it->first) {
            g = myloop->freevar[i];
            break;
          }
        }
        if (g == NULL)
          throw std::invalid_argument("symbolic variable " + it->first + " not found");
        else
          h.update_coef(rel.get_local(g), it->second);
      }
    }
  }
  myloop->addKnown(rel);
}

static PyObject *chill_known(PyObject *self, PyObject *args) {
  strict_arg_num(args, 1, "known");
  if (PyList_Check(PyTuple_GetItem(args, 0))) {
    PyObject *list = PyTuple_GetItem(args, 0);
    for (int i = 0; i < PyList_Size(list); i++) {
      add_known(std::string(PyString_AsString(PyList_GetItem(list, i))));
    }
  } else {
    add_known(strArg(args, 0));
  }
  Py_RETURN_NONE;
}

static PyObject *chill_remove_dep(PyObject *self, PyObject *args) {
  strict_arg_num(args, 0, "remove_dep");
  int from = intArg(args, 0);
  int to = intArg(args, 1);
  myloop->removeDependence(from, to);
  Py_RETURN_NONE;
}

static PyObject *chill_original(PyObject *self, PyObject *args) {
  strict_arg_num(args, 0, "original");
  myloop->original();
  Py_RETURN_NONE;
}

static PyObject *chill_permute(PyObject *self, PyObject *args) {
  int nargs = strict_arg_range(args, 1, 3, "permute");
  if ((nargs < 1) || (nargs > 3))
    throw std::runtime_error("incorrect number of arguments in permute");
  if (nargs == 1) {
    // premute ( vector )
    std::vector<int> pi;
    if (!tointvector(args, 0, pi))
      throw std::runtime_error("first arg in permute(pi) must be an int vector");
    myloop->permute(pi);
  } else if (nargs == 2) {
    // permute ( set, vector )
    std::set<int> active;
    std::vector<int> pi;
    if (!tointset(args, 0, active))
      throw std::runtime_error("the first argument in permute(active, pi) must be an int set");
    if (!tointvector(args, 1, pi))
      throw std::runtime_error("the second argument in permute(active, pi) must be an int vector");
    myloop->permute(active, pi);
  } else if (nargs == 3) {
    int stmt_num = intArg(args, 1);
    int level = intArg(args, 2);
    std::vector<int> pi;
    if (!tointvector(args, 3, pi))
      throw std::runtime_error("the third argument in permute(stmt_num, level, pi) must be an int vector");
    myloop->permute(stmt_num, level, pi);
  }
  Py_RETURN_NONE;
}

static PyObject *chill_pragma(PyObject *self, PyObject *args) {
  strict_arg_num(args, 3, "pragma");
  int stmt_num = intArg(args, 1);
  int level = intArg(args, 1);
  std::string pragmaText = strArg(args, 2);
  myloop->pragma(stmt_num, level, pragmaText);
  Py_RETURN_NONE;
}

static PyObject *chill_prefetch(PyObject *self, PyObject *args) {
  strict_arg_num(args, 3, "prefetch");
  int stmt_num = intArg(args, 0);
  int level = intArg(args, 1);
  std::string prefetchText = strArg(args, 2);
  int hint = intArg(args, 3);
  myloop->prefetch(stmt_num, level, prefetchText, hint);
  Py_RETURN_NONE;
}

static PyObject *chill_tile(PyObject *self, PyObject *args) {
  int nargs = strict_arg_range(args, 3, 7, "tile");
  int stmt_num = intArg(args, 0);
  int level = intArg(args, 1);
  int tile_size = intArg(args, 2);
  if (nargs == 3) {
    myloop->tile(stmt_num, level, tile_size);
  } else if (nargs >= 4) {
    int outer_level = intArg(args, 3);
    if (nargs >= 5) {
      TilingMethodType method = StridedTile;
      int imethod = intArg(args, 4, 2); //< don't know if a default value is needed
      // check method input against expected values
      if (imethod == 0)
        method = StridedTile;
      else if (imethod == 1)
        method = CountedTile;
      else
        throw std::runtime_error("5th argument must be either strided or counted");
      if (nargs >= 6) {
        int alignment_offset = intArg(args, 5);
        if (nargs == 7) {
          int alignment_multiple = intArg(args, 6, 1);
          myloop->tile(stmt_num, level, tile_size, outer_level, method, alignment_offset, alignment_multiple);
        }
        if (nargs == 6)
          myloop->tile(stmt_num, level, tile_size, outer_level, method, alignment_offset);
      }
      if (nargs == 5)
        myloop->tile(stmt_num, level, tile_size, outer_level, method);
    }
    if (nargs == 4)
      myloop->tile(stmt_num, level, tile_size, outer_level);
  }
  Py_RETURN_NONE;
}

static void chill_datacopy_vec(PyObject *args) {
  // Overload 1: bool datacopy(
  //    const std::vector<std::pair<int, std::vector<int> > > &array_ref_nums,
  //    int level,
  //    bool allow_extra_read = false,
  //    int fastest_changing_dimension = -1,
  //    int padding_stride = 1,
  //    int padding_alignment = 4,
  //    int memory_type = 0);
  std::vector<std::pair<int, std::vector<int> > > array_ref_nums;
  // expect list(tuple(int,list(int)))
  // or dict(int,list(int))
  if (PyList_CheckExact(PyTuple_GetItem(args, 0))) {
    PyObject *list = PyTuple_GetItem(args, 0);
    for (int i = 0; i < PyList_Size(list); i++) {
      PyObject *tup = PyList_GetItem(list, i);
      int index = PyLong_AsLong(PyTuple_GetItem(tup, 0));
      std::vector<int> vec;
      tointvector(PyTuple_GetItem(tup, 1), vec);
      array_ref_nums.push_back(std::pair<int, std::vector<int> >(index, vec));
    }
  } else if (PyList_CheckExact(PyTuple_GetItem(args, 0))) {
    PyObject *dict = PyTuple_GetItem(args, 0);
    PyObject *klist = PyDict_Keys(dict);
    for (int ki = 0; ki < PyList_Size(klist); ki++) {
      PyObject *index = PyList_GetItem(klist, ki);
      std::vector<int> vec;
      tointvector(PyDict_GetItem(dict, index), vec);
      array_ref_nums.push_back(std::pair<int, std::vector<int> >(PyLong_AsLong(index), vec));
    }
    Py_DECREF(klist);
  } else {
    //TODO: this should never happen
  }
  int level = intArg(args, 1);
  bool allow_extra_read = boolArg(args, 2, false);
  int fastest_changing_dimension = intArg(args, 3, -1);
  int padding_stride = intArg(args, 4, 1);
  int padding_alignment = intArg(args, 5, 4);
  int memory_type = intArg(args, 6, 0);
  myloop->datacopy(array_ref_nums, level, allow_extra_read, fastest_changing_dimension, padding_stride,
                   padding_alignment, memory_type);
}

static void chill_datacopy_int(PyObject *args) {
  int stmt_num = intArg(args, 0);
  int level = intArg(args, 1);
  std::string array_name = strArg(args, 2, 0);
  bool allow_extra_read = boolArg(args, 3, false);
  int fastest_changing_dimension = intArg(args, 4, -1);
  int padding_stride = intArg(args, 5, 1);
  int padding_alignment = intArg(args, 6, 4);
  int memory_type = intArg(args, 7, 0);
  myloop->datacopy(stmt_num, level, array_name, allow_extra_read, fastest_changing_dimension, padding_stride,
                   padding_alignment, memory_type);
}

static PyObject *chill_datacopy(PyObject *self, PyObject *args) {
  // Overload 2: bool datacopy(int stmt_num, int level, const std::string &array_name, bool allow_extra_read = false, int fastest_changing_dimension = -1, int padding_stride = 1, int padding_alignment = 4, int memory_type = 0);
  int nargs = strict_arg_range(args, 3, 7, "datacopy");
  if (PyList_CheckExact(PyTuple_GetItem(args, 0)) || PyDict_CheckExact(PyTuple_GetItem(args, 0))) {
    chill_datacopy_vec(args);
  } else {
    chill_datacopy_int(args);
  }
  Py_RETURN_NONE;
}

static PyObject *chill_datacopy_privatized(PyObject *self, PyObject *args) {
  //  bool datacopy_privatized(int stmt_num, int level, const std::string &array_name, const std::vector<int> &privatized_levels, bool allow_extra_read = false, int fastest_changing_dimension = -1, int padding_stride = 1, int padding_alignment = 1, int memory_type = 0);
  int nargs = strict_arg_range(args, 4, 9, "datacopy_privatized");
  int stmt_num = intArg(args, 0);
  int level = intArg(args, 1);
  std::string array_name = strArg(args, 2);
  std::vector<int> privatized_levels;
  tointvector(args, 3, privatized_levels);
  bool allow_extra_read = boolArg(args, 4, false);
  int fastest_changing_dimension = intArg(args, 5, -1);
  int padding_stride = intArg(args, 6, 1);
  int padding_alignment = intArg(args, 7, 1);
  int memory_type = intArg(args, 8);
  myloop->datacopy_privatized(stmt_num, level, array_name, privatized_levels, allow_extra_read,
                              fastest_changing_dimension, padding_stride, padding_alignment, memory_type);
  Py_RETURN_NONE;
}

static PyObject *chill_unroll(PyObject *self, PyObject *args) {
  int nargs = strict_arg_range(args, 3, 4, "unroll");
  //std::set<int> unroll(int stmt_num, int level, int unroll_amount, std::vector< std::vector<std::string> >idxNames= std::vector< std::vector<std::string> >(), int cleanup_split_level = 0);
  int stmt_num = intArg(args, 0);
  int level = intArg(args, 1);
  int unroll_amount = intArg(args, 2);
  std::vector<std::vector<std::string> > idxNames = std::vector<std::vector<std::string> >();
  int cleanup_split_level = intArg(args, 3);
  myloop->unroll(stmt_num, level, unroll_amount, idxNames, cleanup_split_level);
  Py_RETURN_NONE;
}

static PyObject *chill_unroll_extra(PyObject *self, PyObject *args) {
  int nargs = strict_arg_range(args, 3, 4, "unroll_extra");
  int stmt_num = intArg(args, 0);
  int level = intArg(args, 1);
  int unroll_amount = intArg(args, 2);
  int cleanup_split_level = intArg(args, 3, 0);
  myloop->unroll_extra(stmt_num, level, unroll_amount, cleanup_split_level);
  Py_RETURN_NONE;
}

static PyObject *chill_split(PyObject *self, PyObject *args) {
  strict_arg_num(args, 3, "split");
  int stmt_num = intArg(args, 0);
  int level = intArg(args, 1);
  int num_dim = myloop->stmt[stmt_num].xform.n_out();

  std::vector<std::map<std::string, int> > *cond;
  std::string cond_expr = strArg(args, 2);
  cond = parse_relation_vector(cond_expr.c_str());

  Relation rel((num_dim - 1) / 2);
  F_And *f_root = rel.add_and();
  for (int j = 0; j < cond->size(); j++) {
    GEQ_Handle h = f_root->add_GEQ();
    for (std::map<std::string, int>::iterator it = (*cond)[j].begin(); it != (*cond)[j].end(); it++) {
      try {
        int dim = from_string<int>(it->first);
        if (dim == 0)
          h.update_const(it->second);
        else {
          if (dim > (num_dim - 1) / 2)
            throw std::invalid_argument("invalid loop level " + to_string(dim) + " in split condition");
          h.update_coef(rel.set_var(dim), it->second);
        }
      }
      catch (std::ios::failure e) {
        Free_Var_Decl *g = NULL;
        for (unsigned i = 0; i < myloop->freevar.size(); i++) {
          std::string name = myloop->freevar[i]->base_name();
          if (name == it->first) {
            g = myloop->freevar[i];
            break;
          }
        }
        if (g == NULL)
          throw std::invalid_argument("unrecognized variable " + to_string(it->first.c_str()));
        h.update_coef(rel.get_local(g), it->second);
      }
    }
  }
  myloop->split(stmt_num, level, rel);
  Py_RETURN_NONE;
}

static PyObject *chill_nonsingular(PyObject *self, PyObject *args) {
  std::vector<std::vector<int> > mat;
  tointmatrix(args, 0, mat);
  myloop->nonsingular(mat);
  Py_RETURN_NONE;
}

static PyObject *chill_skew(PyObject *self, PyObject *args) {
  std::set<int> stmt_nums;
  std::vector<int> skew_amounts;
  int level = intArg(args, 1);
  tointset(args, 0, stmt_nums);
  tointvector(args, 2, skew_amounts);
  myloop->skew(stmt_nums, level, skew_amounts);
  Py_RETURN_NONE;
}

static PyObject *chill_scale(PyObject *self, PyObject *args) {
  strict_arg_num(args, 3);
  std::set<int> stmt_nums;
  int level = intArg(args, 1);
  int scale_amount = intArg(args, 2);
  tointset(args, 0, stmt_nums);
  myloop->scale(stmt_nums, level, scale_amount);
  Py_RETURN_NONE;
}

static PyObject *chill_reverse(PyObject *self, PyObject *args) {
  strict_arg_num(args, 2);
  std::set<int> stmt_nums;
  int level = intArg(args, 1);
  tointset(args, 0, stmt_nums);
  myloop->reverse(stmt_nums, level);
  Py_RETURN_NONE;
}

static PyObject *chill_shift(PyObject *self, PyObject *args) {
  strict_arg_num(args, 3);
  std::set<int> stmt_nums;
  int level = intArg(args, 1);
  int shift_amount = intArg(args, 2);
  tointset(args, 0, stmt_nums);
  myloop->shift(stmt_nums, level, shift_amount);
  Py_RETURN_NONE;
}

static PyObject *chill_shift_to(PyObject *self, PyObject *args) {
  strict_arg_num(args, 3);
  int stmt_num = intArg(args, 0);
  int level = intArg(args, 1);
  int absolute_pos = intArg(args, 2);
  myloop->shift_to(stmt_num, level, absolute_pos);
  Py_RETURN_NONE;
}

static PyObject *chill_peel(PyObject *self, PyObject *args) {
  strict_arg_range(args, 2, 3);
  int stmt_num = intArg(args, 0);
  int level = intArg(args, 1);
  int amount = intArg(args, 2);
  myloop->peel(stmt_num, level, amount);
  Py_RETURN_NONE;
}

static PyObject *chill_fuse(PyObject *self, PyObject *args) {
  strict_arg_num(args, 2);
  std::set<int> stmt_nums;
  int level = intArg(args, 1);
  tointset(args, 0, stmt_nums);
  myloop->fuse(stmt_nums, level);
  Py_RETURN_NONE;
}

static PyObject *chill_distribute(PyObject *self, PyObject *args) {
  strict_arg_num(args, 2);
  std::set<int> stmts;
  int level = intArg(args, 1);
  tointset(args, 0, stmts);
  myloop->distribute(stmts, level);
  Py_RETURN_NONE;
}

static PyObject *
chill_num_statements(PyObject *self, PyObject *args) {
  //DEBUG_PRINT("\nC chill_num_statements() called from python\n"); 
  int num = myloop->stmt.size();
  //DEBUG_PRINT("C num_statement() = %d\n", num); 
  return Py_BuildValue("i", num); // BEWARE "d" is DOUBLE, not int
}

static PyMethodDef ChillMethods[] = {

    //python name           C routine                  parameter passing comment
    {"source",              chill_source,              METH_VARARGS, "set source file for chill script"},
    {"procedure",           chill_procedure,           METH_VARARGS, "set the name of the procedure"},
    {"loop",                chill_loop,                METH_VARARGS, "indicate which loop to optimize"},
    {"print_code",          chill_print_code,          METH_VARARGS, "print generated code"},
    {"print_dep",           chill_print_dep,           METH_VARARGS, "print the dependencies graph"},
    {"print_space",         chill_print_space,         METH_VARARGS, "print space"},
    {"known",               chill_known,               METH_VARARGS, "knwon"},
    {"remove_dep",          chill_remove_dep,          METH_VARARGS, "remove dependency i suppose"},
    {"original",            chill_original,            METH_VARARGS, "original"},
    {"permute",             chill_permute,             METH_VARARGS, "permute"},
    {"pragma",              chill_pragma,              METH_VARARGS, "pragma"},
    {"prefetch",            chill_prefetch,            METH_VARARGS, "prefetch"},
    {"tile",                chill_tile,                METH_VARARGS, "tile"},
    {"datacopy",            chill_datacopy,            METH_VARARGS, "datacopy"},
    {"datacopy_privitized", chill_datacopy_privatized, METH_VARARGS, "datacopy_privatized"},
    {"unroll",              chill_unroll,              METH_VARARGS, "unroll"},
    {"unroll_extra",        chill_unroll_extra,        METH_VARARGS, "unroll_extra"},
    {"split",               chill_split,               METH_VARARGS, "split"},
    {"nonsingular",         chill_nonsingular,         METH_VARARGS, "nonsingular"},
    {"skew",                chill_skew,                METH_VARARGS, "skew"},
    {"scale",               chill_scale,               METH_VARARGS, "scale"},
    {"reverse",             chill_reverse,             METH_VARARGS, "reverse"},
    {"shift",               chill_shift,               METH_VARARGS, "shift"},
    {"shift_to",            chill_shift_to,            METH_VARARGS, "shift_to"},
    {"peel",                chill_peel,                METH_VARARGS, "peel"},
    {"fuse",                chill_fuse,                METH_VARARGS, "fuse"},
    {"distribute",          chill_distribute,          METH_VARARGS, "distribute"},
    {"num_statements",      chill_num_statements,      METH_VARARGS, "number of statements in the current loop"},
    {NULL, NULL, 0, NULL}
};

static void register_globals(PyObject *m) {
  // Preset globals
  PyModule_AddStringConstant(m, "VERSION", CHILL_BUILD_VERSION);
  PyModule_AddStringConstant(m, "dest", "C");
  PyModule_AddStringConstant(m, "C", "C");
  // Tile method
  PyModule_AddIntConstant(m, "strided", 0);
  PyModule_AddIntConstant(m, "counted", 1);
  // Memory mode
  PyModule_AddIntConstant(m, "global", 0);
  PyModule_AddIntConstant(m, "shared", 1);
  PyModule_AddIntConstant(m, "textured", 2);
  // Bool flags
  PyModule_AddIntConstant(m, "sync", 1);
}

PyMODINIT_FUNC
initchill(void)    // pass C methods to python 
{
  CHILL_DEBUG_PRINT("set up C methods to be called from python\n");
  PyObject *m = Py_InitModule("chill", ChillMethods);
  register_globals(m);
}
