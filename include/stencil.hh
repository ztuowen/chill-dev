
#pragma once

#include "ir_chill.hh"
#include "chillAST/chillASTs.hh"

//#include <omega.h>
#include <omegatools.hh>
#include "code_gen/CG_chillRepr.h"

// #include "loop.hh"
#include "ir_code.hh"



// *extern Loop *myloop;  // ???


//void stencil( chillAST_Node *topstatement, class stencilInfo &SI ); // stencilInfo was originally a struct, and this was the way it got filled in.  TODO remove


class stencilInfo {
public:
  uint dimensions;        // number of dimensions in the stencil
  uint elements;          // total number of elements in the stencil
  chillAST_VarDecl *srcArrayVariable;   // the variable of the source array
  chillAST_VarDecl *dstArrayVariable;   // the variable of the destination array

  int minOffset[3];         // the minimum offset for each dimension  NOTE hardcoded to 3 dimensions TODO 
  int maxOffset[3];         // the maximum offset for each dimension

  chillAST_VarDecl *indexVariables[3];  // the variable used for indexing each dimension

  std::vector<std::vector<int> > offsets;  //  k+1, j-47, i+0   etc


  std::vector<std::vector<chillAST_Node *> > coefficients;  // the coefficients of for each element, NOT IN ANY SET ORDER
  chillAST_Node *find_coefficient(int i, int j, int k); // hardcoded 3 dimensions TODO

  // constructors
  stencilInfo();

  stencilInfo(chillAST_Node *topstatement);

  void walktree(chillAST_Node *node, std::vector<chillAST_Node *> &coeffstohere);

  void print(FILE *fp = stdout);

  int radius();

  bool isSymmetric();

};  // class stencilInfo 



