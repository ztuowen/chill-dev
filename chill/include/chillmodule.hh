#ifndef CHILLMODULE_HH
#define CHILLMODULE_HH

#include <Python.h>

void finalize_loop(int loop_num_start, int loop_num_end);
int get_loop_num_start();
int get_loop_num_end();
//! pass C methods to python
PyMODINIT_FUNC initchill();

#endif
