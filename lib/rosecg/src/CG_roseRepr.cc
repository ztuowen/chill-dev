/*****************************************************************************
 Copyright (C) 2008 University of Southern California. 
 All Rights Reserved.

 Purpose:
   omega holder for suif implementaion

 Notes:

 History:
   02/01/06 - Chun Chen - created
*****************************************************************************/

#include "code_gen/CG_roseRepr.h"
#include "code_gen/rose_attributes.h"
#include <stdio.h>
#include <string.h>
#include <cstring>
namespace omega {




CG_roseRepr::CG_roseRepr(): tnl_(NULL), op_(NULL), list_(NULL){

}

CG_roseRepr::CG_roseRepr(SgNode *tnl): tnl_(tnl), op_(NULL),list_(NULL) {
}

CG_roseRepr::CG_roseRepr(SgExpression* op): tnl_(NULL), op_(op),list_(NULL){
}
CG_roseRepr::CG_roseRepr(SgStatementPtrList* stmtlist):tnl_(NULL), op_(NULL), list_(stmtlist){
}
  
CG_roseRepr::~CG_roseRepr() {
  // delete nothing here. operand or tree_node_list should already be
  // grafted to other expression tree or statement list
}

CG_outputRepr* CG_roseRepr::clone()  const {

  if( tnl_ != NULL) {
    SgTreeCopy  tc;      
    SgNode *tnl = tnl_->copy(tc);
    copyAttributes(tnl_, tnl);
    
    tnl->set_parent(tnl_->get_parent());
    return new CG_roseRepr(tnl);
  }
  else if(op_ != NULL)
  {
     SgTreeCopy tc1;
     SgNode* op =  isSgNode(op_)->copy(tc1);
     copyAttributes(op_, op);
  
     op->set_parent(isSgNode(op_)->get_parent());   
     return new CG_roseRepr(isSgExpression(op));
  }
  else if(list_ != NULL) 
  {
     SgStatementPtrList* list2 = new SgStatementPtrList;
     
     for(SgStatementPtrList::iterator it = (*list_).begin(); it != (*list_).end(); it++){
        SgTreeCopy  tc3;      
        SgNode *tnl2  =  isSgNode(*it)->copy(tc3);
        copyAttributes(*it, tnl2);
        
        tnl2->set_parent(isSgNode(*it)->get_parent());
       
        (*list2).push_back(isSgStatement(tnl2));       
    }   
    return new CG_roseRepr(list2);
  }
  
  return NULL;
}

void CG_roseRepr::clear() {
  if(tnl_ != NULL) {
    delete tnl_;
    tnl_ = NULL;
  }
}

SgNode* CG_roseRepr::GetCode() const {
  return tnl_;
}

SgStatementPtrList* CG_roseRepr::GetList() const {
  return list_;
}

SgExpression* CG_roseRepr::GetExpression() const {
   return op_;
} 
void CG_roseRepr::Dump() const {
SgNode* tnl = tnl_;
SgExpression* op = op_ ;
 if(tnl != NULL)
  DumpFileHelper(tnl, stdout);
 else if(op != NULL)
   DumpFileHelper(isSgNode(op), stdout);             

}

void  CG_roseRepr::DumpFileHelper(SgNode* node, FILE *fp) const{
  std::string x;
  size_t numberOfSuccessors = node->get_numberOfTraversalSuccessors();
 if(numberOfSuccessors == 0){
    x = node->unparseToString ();   
    fprintf(fp, "%s", x.c_str());
 }
 else{     
   for (size_t idx = 0; idx < numberOfSuccessors; idx++)
   {
       SgNode *child = NULL;
       child = node->get_traversalSuccessorByIndex(idx);
       DumpFileHelper(child, fp);                 
   }
 
}
}

} // namespace
