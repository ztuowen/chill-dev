
#ifndef CG_chillRepr_h
#define CG_chillRepr_h

//                                               Repr using chillAst internally
#include <stdio.h>
#include <string.h>
#include <code_gen/CG_outputRepr.h>

#ifndef __STDC_CONSTANT_MACROS
#define __STDC_CONSTANT_MACROS 
#endif


#include "chillAST/chillASTs.hh"


namespace omega {

  class CG_chillRepr : public CG_outputRepr {
    friend class CG_chillBuilder;

  public:
    CG_chillRepr() { stmtclassname = strdup("NOTHING"); }


    char *type() const { return strdup("chill"); };
    //
    std::vector<chillAST_Node *> chillnodes;  // TODO make private
    void printChillNodes() const {
      for (int i = 0; i < chillnodes.size(); i++)
        chillnodes[i]->print();
      fflush(stdout);
    };

    CG_chillRepr(std::vector<chillAST_Node *> cnodes) {
      chillnodes = cnodes;
    }

    CG_chillRepr(chillAST_Node *chillast) {
      stmtclassname = strdup(chillast->getTypeString());
      //fprintf(stderr, "made new chillRepr of class %s\n", stmtclassname);
      if (chillast->getType() == CHILLAST_NODE_COMPOUNDSTMT) {
        std::vector<chillAST_Node *> &children = *(chillast->getChildren());
        int numchildren = children.size();
        for (int i = 0; i < numchildren; i++) {
          chillnodes.push_back(children[i]);
          //fprintf(stderr, "adding a statement from a CompoundStmt\n");
        }
      } else { // for now, assume it's a single statement
        chillnodes.push_back(chillast);  // ??
      }
    }

    void addStatement(chillAST_Node *s) { chillnodes.push_back(s); };

    std::vector<chillAST_Node *> getChillCode() const { return chillnodes; };

    chillAST_Node *GetCode();


    ~CG_chillRepr();

    CG_outputRepr *clone() const;

    void clear();


    //---------------------------------------------------------------------------
    // Dump operations
    //---------------------------------------------------------------------------
    void dump() const { printChillNodes(); };

    void Dump() const;
    //void DumpToFile(FILE *fp = stderr) const;
  private:


    char *stmtclassname;    // chill

  };


} // namespace

#endif
