#include "chilldebug.h"

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include "loop.hh"
#include <omega.h>
#include "ir_code.hh"
#include "ir_rose.hh"

#include "chillmodule.hh" // Python wrapper functions for CHiLL

//---
// CHiLL globals
//---
Loop *myloop = NULL;
IR_Code *ir_code = NULL;
bool repl_stop = false;
bool is_interactive = false;

std::vector<IR_Control *> ir_controls;
std::vector<int> loops;

//---
// CHiLL program main
// Initialize state and run script or interactive mode
//---
int main( int argc, char* argv[] )
{
  DEBUG_PRINT("%s  main()\n", argv[0]);
  if (argc > 2) {
    fprintf(stderr, "Usage: %s [script_file]\n", argv[0]);
    exit(-1);
  }
  
  int fail = 0;
  
  // Create PYTHON interpreter
  /* Pass argv[0] to the Python interpreter */
  Py_SetProgramName(argv[0]);
  
  /* Initialize the Python interpreter.  Required. */
  Py_Initialize();
  
  /* Add a static module */
  initchill();
  
  if (argc == 2) {
    FILE* f = fopen(argv[1], "r");
    if(!f){
      printf("can't open script file \"%s\"\n", argv[1]);
      exit(-1);
    }
    PyRun_SimpleFile(f, argv[1]);
    fclose(f);
  }
  if (argc == 1) {
    //---
    // Run a CHiLL interpreter
    //---
    printf("CHiLL v" CHILL_BUILD_VERSION " (built on " CHILL_BUILD_DATE ")\n");
    printf("Copyright (C) 2008 University of Southern California\n");
    printf("Copyright (C) 2009-2012 University of Utah\n");
    //is_interactive = true; // let the lua interpreter know.
    fflush(stdout);
    // TODO: read lines of python code.
    //Not sure if we should set fail from interactive mode
    printf("CHiLL ending...\n");
    fflush(stdout);
  }

  //printf("DONE with PyRun_SimpleString()\n");
  
  if (!fail && ir_code != NULL && myloop != NULL && myloop->stmt.size() != 0 && !myloop->stmt[0].xform.is_null()) {
    int lnum_start;
    int lnum_end;
    lnum_start = get_loop_num_start();
    lnum_end = get_loop_num_end();
    DEBUG_PRINT("calling ROSE code gen?    loop num %d\n", lnum);
    finalize_loop(lnum_start, lnum_end);
    ((IR_roseCode*)(ir_code))->finalizeRose();
    delete ir_code;
  }
  Py_Finalize();
  return 0;
}
