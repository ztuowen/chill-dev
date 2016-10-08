/*****************************************************************************
 Copyright (C) 2008 University of Southern California. 
 All Rights Reserved.

 Purpose:
   omega holder for chill AST  implementaion

 Notes:

 History:
   02/01/06 - Chun Chen - created
   LLVM/CLANG interface created by Saurav Muralidharan
*****************************************************************************/

#include <code_gen/CG_chillRepr.h>
#include <stdio.h>
#include <stdlib.h>
#include <code_gen/CGdebug.h>

namespace omega { 

  CG_chillRepr::~CG_chillRepr() {
  }
  
  chillAST_Node * CG_chillRepr::GetCode() {
    if (0 == chillnodes.size()) {
      CG_ERROR("No contained chillnodes\n");
      return NULL; // error?
    }

    if (1 == chillnodes.size()) return chillnodes[0];

    CG_DEBUG_PRINT("Creating a compound statements\n");
    chillAST_CompoundStmt *CS = new chillAST_CompoundStmt();
    for (int i=0; i<chillnodes.size(); i++) {
      CS->addChild( chillnodes[i] );
    }
    return CS; 
  }

  CG_outputRepr* CG_chillRepr::clone() const {
    CG_chillRepr *newrepr = new  CG_chillRepr();
    
    for (int i=0; i<chillnodes.size(); i++)
      newrepr->addStatement( chillnodes[i]->clone() );

    return newrepr;
  }

  void CG_chillRepr::clear() {
    chillnodes.clear();
  }

  void CG_chillRepr::dump() const {
    for (int i = 0; i < chillnodes.size(); i++)
      chillnodes[i]->print(0,stdout);
  }

} // namespace
