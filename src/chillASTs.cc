#include <chilldebug.h>
#include <stack>
#include <fstream>
#include "chillAST.h"
#include "printer/dump.h"
#include "printer/cfamily.h"

using namespace std;

bool streq(const char *a, const char *b) {
  return !strcmp(a, b);
}

char *parseUnderlyingType(const char *sometype) {
  int len = strlen(sometype);
  CHILL_DEBUG_PRINT("parseUnderlyingType( %s )\n", sometype);
  char *underlying = strdup(sometype);
  char *p;

  p = &underlying[len - 1];

  while (p > underlying)
    if (*p == ' ' || *p == '*')
      --p;
    else if (*p == ']') {
      while (*p != '[') --p;
      --p;
    }
    else break;

  *(p + 1) = '\0';

  return underlying;
}

char *splitVariableName(char *name) {
  char *cdot = strstr(name, ".");
  char *carrow = strstr(name, "->");  // initial 'c' for const - can't change those

  int pos = INT16_MAX, pend;

  if (cdot) pend = pos = (int) (cdot - name);
  if (carrow) {
    pos = min(pos, (int) (carrow - name));
    pend = pos + 1;
  }

  if (pos == INT16_MAX)
    return NULL;

  name[pos] = '\0';
  return &name[pend + 1];

}

chillAST_VarDecl *symbolTableFindName(chillAST_SymbolTable *table, const char *name) {
  if (!table) return NULL;
  int numvars = table->size();
  for (int i = 0; i < numvars; i++) {
    chillAST_VarDecl *vd = (*table)[i];
    if (vd->nameis(name)) return vd;
  }
  return NULL;
}

chillAST_TypedefDecl *typedefTableFindName(chillAST_TypedefTable *table, const char *name) {
  if (!table) return NULL;
  int numvars = table->size();
  for (int i = 0; i < numvars; i++) {
    chillAST_TypedefDecl *td = (*table)[i];
    if (td->nameis(name)) return td;
  }
  return NULL;
}

chillAST_VarDecl *variableDeclFindSubpart(chillAST_VarDecl *decl, const char *name) {
  char *varname = strdup(name), *subpart;

  subpart = splitVariableName(varname);

  if (decl->isAStruct()) {
    CHILL_DEBUG_PRINT("Finding %s in a struct of type %s\n", varname, decl->getTypeString());
    if (decl->isVarDecl()) {
      chillAST_RecordDecl *rd = decl->getStructDef();
      if (rd) {
        chillAST_VarDecl *sp = rd->findSubpart(varname);
        if (sp) CHILL_DEBUG_PRINT("found a struct member named %s\n", varname);
        else
          CHILL_DEBUG_PRINT("DIDN'T FIND a struct member named %s\n", varname);
        if (!subpart)
          return sp;
        return variableDeclFindSubpart(sp, subpart);  // return the subpart??
      } else {
        CHILL_ERROR("no recordDecl\n");
        exit(-1);
      }
    } else {
      CHILL_ERROR("NOT a VarDecl???\n"); // impossible
    }
  } else {
    CHILL_DEBUG_PRINT("false alarm. %s is a variable, but doesn't have subparts\n", varname);
    return NULL;
  }
}

chillAST_VarDecl *symbolTableFindVariableNamed(chillAST_SymbolTable *table, const char *name) {
  if (!table) return NULL;

  char *varname = strdup(name), *subpart;

  subpart = splitVariableName(varname);

  chillAST_VarDecl *vd = symbolTableFindName(table, varname);

  if (!subpart)
    return vd;
  return variableDeclFindSubpart(vd, subpart);
}

char *ulhack(char *brackets) {
  CHILL_DEBUG_PRINT("ulhack( \"%s\"  -> \n", brackets);
  int len = strlen(brackets);
  for (int i = 0; i < len - 2; i++) {
    if (isdigit(brackets[i])) {
      if (brackets[i + 1] == 'U' && brackets[i + 2] == 'L') { // remove
        for (int j = i + 3; j < len; j++) brackets[j - 2] = brackets[j];
        len -= 2;
        brackets[len] = '\0';
      }
    }
  }
  return brackets;
}

char *restricthack(char *typeinfo) {
  CHILL_DEBUG_PRINT("restricthack( \"%s\"  -> \n", typeinfo);
  std::string r("__restrict__");
  std::string t(typeinfo);
  size_t index = t.find(r);

  if (index == std::string::npos) return typeinfo;

  char *c = &(typeinfo[index]);
  char *after = c + 12;
  if (*after == ' ') after++;

  while (*after != '\0') *c++ = *after++;
  *c = '\0';

  return typeinfo;

}

// TODO DOC
char *parseArrayParts(char *sometype) {
  int len = strlen(sometype);
  char *arraypart = (char *) calloc(1 + strlen(sometype), sizeof(char));// leak

  int c = 0;
  for (int i = 0; i < strlen(sometype);) {
    if (sometype[i] == '*') arraypart[c++] = '*';
    if (sometype[i] == '[') {
      while (sometype[i] != ']') {
        arraypart[c++] = sometype[i++];
      }
      arraypart[c++] = ']';
    }
    i += 1;
  }
  ulhack(arraypart);
  restricthack(arraypart);

  CHILL_DEBUG_PRINT("parseArrayParts( %s ) => %s\n", sometype, arraypart);
  return arraypart;
}


//! return the bracketed part of a type, int *buh[32] to int
char *splitTypeInfo(char *underlyingtype) {
  char *ap = ulhack(parseArrayParts(underlyingtype));

  strcpy(underlyingtype, parseUnderlyingType(underlyingtype));
  return ap;  // leak unless caller frees this
}


bool isRestrict(const char *sometype) { // does not modify sometype
  string r("__restrict__");
  string t(sometype);
  return (std::string::npos != t.find(r));
}

void chillindent(int howfar, FILE *fp) { for (int i = 0; i < howfar; i++) fprintf(fp, "  "); }

chillAST_SourceFile::chillAST_SourceFile(const char *filename) {
  if (filename) SourceFileName = strdup(filename);
  else SourceFileName = strdup("No Source File");
  symbolTable = new chillAST_SymbolTable();
  typedefTable = new chillAST_TypedefTable();
  FileToWrite = NULL;
  frontend = strdup("unknown");
};

void chillAST_SourceFile::printToFile(char *filename) {
  char fn[1024];

  if (NULL == filename) {  // build up a filename using original name and frontend if known
    if (FileToWrite) {
      strcpy(fn, FileToWrite);
    } else {
      // input name with name of frontend compiler prepended
      if (frontend) sprintf(fn, "%s_%s\0", frontend, SourceFileName);
      else sprintf(fn, "UNKNOWNFRONTEND_%s\0", SourceFileName); // should never happen
    }
  } else strcpy(fn, filename);

  std::ofstream fo(fn);
  if (fo.fail()) {
    CHILL_ERROR("can't open file '%s' for writing\n", fn);
    exit(-1);
  }
  chill::printer::CFamily cp;
  cp.print("",this,fo);

}

chillAST_MacroDefinition *chillAST_SourceFile::findMacro(const char *name) {
  int numMacros = macrodefinitions.size();
  for (int i = 0; i < numMacros; i++) {
    if (!strcmp(macrodefinitions[i]->macroName, name)) return macrodefinitions[i];
  }
  return NULL; // not found
}


chillAST_FunctionDecl *chillAST_SourceFile::findFunction(const char *name) {
  int numFuncs = functions.size();
  for (int i = 0; i < numFuncs; i++) {
    if (!strcmp(functions[i]->functionName, name)) return functions[i];
  }
  return NULL;
}


chillAST_Node *chillAST_SourceFile::findCall(const char *name) {
  chillAST_MacroDefinition *macro = findMacro(name);
  if (macro) return (chillAST_Node *) macro;
  chillAST_FunctionDecl *func = findFunction(name);
  return func;
}

chillAST_TypedefDecl::chillAST_TypedefDecl() {
  underlyingtype = newtype = arraypart = NULL;
  parent = NULL;
  metacomment = NULL;
  isStruct = isUnion = false;
  structname = NULL;
  rd = NULL;
  isFromSourceFile = true; // default
  filename = NULL;
};


chillAST_TypedefDecl::chillAST_TypedefDecl(char *t, const char *nt) {
  underlyingtype = strdup(t);
  newtype = strdup(nt);
  arraypart = NULL;
  metacomment = NULL;
  isStruct = isUnion = false;
  structname = NULL;
  rd = NULL;
  isFromSourceFile = true; // default
  filename = NULL;
};


chillAST_TypedefDecl::chillAST_TypedefDecl(char *t, const char *a, char *p) {
  underlyingtype = strdup(t);
  newtype = strdup(a);

  arraypart = strdup(p);
  // splitarraypart(); // TODO

  metacomment = NULL;
  isStruct = isUnion = false;
  structname = NULL;
  rd = NULL;
  isFromSourceFile = true; // default
  filename = NULL;
};

chillAST_VarDecl *chillAST_TypedefDecl::findSubpart(const char *name) {
  if (rd) { // we have a record decl look there
    chillAST_VarDecl *sub = rd->findSubpart(name);
    return sub;
  }

  // can this ever happen now ???
  int nsub = subparts.size();
  for (int i = 0; i < nsub; i++) {
    if (!strcmp(name, subparts[i]->varname)) return subparts[i];
  }
  CHILL_ERROR("subpart not found\n");
  return NULL;
}


chillAST_RecordDecl *chillAST_TypedefDecl::getStructDef() {
  if (rd) return rd;
  return NULL;
}

chillAST_RecordDecl::chillAST_RecordDecl(const char *nam, const char *orig) {
  if (nam) name = strdup(nam);
  else name = strdup("unknown"); // ??

  originalname = NULL;
  symbolTable = new chillAST_SymbolTable();
  if (orig) originalname = strdup(orig);

  isStruct = isUnion = false;
  isFromSourceFile = true; // default
  filename = NULL;
}


chillAST_VarDecl *chillAST_RecordDecl::findSubpart(const char *nam) {
  int nsub = subparts.size();
  for (int i = 0; i < nsub; i++) {
    if (!strcmp(nam, subparts[i]->varname)) return subparts[i];
  }
  CHILL_DEBUG_PRINT("chillAST_RecordDecl::findSubpart() couldn't find member NAMED %s in ", nam);
  print();
  return NULL;
}


chillAST_VarDecl *chillAST_RecordDecl::findSubpartByType(const char *typ) {
  int nsub = subparts.size();
  for (int i = 0; i < nsub; i++) {
    if (!strcmp(typ, subparts[i]->vartype)) return subparts[i];
  }
  return NULL;
}

chillAST_SymbolTable *chillAST_RecordDecl::addVariableToSymbolTable(chillAST_VarDecl *vd) {
  // for now, just bail. or do we want the struct to have an actual symbol table?
  return NULL; // damn, I hope nothing uses this!
}

chillAST_FunctionDecl::chillAST_FunctionDecl(const char *rt, const char *fname, void *unique) {
  CHILL_DEBUG_PRINT("chillAST_FunctionDecl::chillAST_FunctionDecl with unique %p\n", unique);
  if (fname)
    functionName = strdup(fname);
  else
    functionName = strdup("YouScrewedUp");
  forwarddecl = externfunc = builtin = false;
  parameters = new chillAST_SymbolTable();
  this->setFunctionCPU();
  returnType = strdup(rt);
  this->setFunctionCPU();
  children.push_back(new chillAST_CompoundStmt());
  uniquePtr = unique; // a quick way to check equivalence. DO NOT ACCESS THROUGH THIS
};

void chillAST_FunctionDecl::gatherVarDecls(vector<chillAST_VarDecl *> &decls) {
  for (int i = 0; i < getParameters()->size(); i++) (*getParameters())[i]->gatherVarDecls(decls);
  for (int i = 0; i < children.size(); i++) children[i]->gatherVarDecls(decls);
}


void chillAST_FunctionDecl::gatherScalarVarDecls(vector<chillAST_VarDecl *> &decls) {
  for (int i = 0; i < getParameters()->size(); i++) (*getSymbolTable())[i]->gatherScalarVarDecls(decls);
  for (int i = 0; i < children.size(); i++) children[i]->gatherScalarVarDecls(decls);
}


void chillAST_FunctionDecl::gatherArrayVarDecls(vector<chillAST_VarDecl *> &decls) {
  for (int i = 0; i < getParameters()->size(); i++) (*getSymbolTable())[i]->gatherArrayVarDecls(decls);
  for (int i = 0; i < children.size(); i++) children[i]->gatherArrayVarDecls(decls);
}


chillAST_VarDecl *chillAST_FunctionDecl::findArrayDecl(const char *name) {
  chillAST_VarDecl *p = getVariableDeclaration(name);
  if (p && p->isArray()) return p;
  chillAST_VarDecl *v = getBody()->getVariableDeclaration(name);
  if (v && v->isArray()) return v;
  // declared variables that may not be in symbol table but probably should be
  vector<chillAST_VarDecl *> decls;
  gatherArrayVarDecls(decls);
  for (int i = 0; i < decls.size(); i++) {
    chillAST_VarDecl *vd = decls[i];
    if (0 == strcmp(vd->varname, name) && vd->isArray()) return vd;
  }
  return NULL;
}

void chillAST_FunctionDecl::cleanUpVarDecls() {
  vector<chillAST_VarDecl *> used;
  vector<chillAST_VarDecl *> defined;
  vector<chillAST_VarDecl *> deletethese;

  gatherVarUsage(used);
  gatherVarDecls(defined);

  for (int j = 0; j < defined.size(); j++) {
    bool definedandused = false;
    for (int i = 0; i < used.size(); i++) {
      if (used[i] == defined[j]) {
        definedandused = true;
        break;
      }
    }

    if (!definedandused) {
      if (!defined[j]->isParmVarDecl()) {
        deletethese.push_back(defined[j]);
      }
    }
  }

  for (int i = 0; i < deletethese.size(); i++) {
    chillAST_Node *par = deletethese[i]->parent;
    par->removeChild(par->findChild(deletethese[i]));
  }

  // now check for vars used but not defined?
  for (int j = 0; j < used.size(); j++) {
    bool definedandused = false;
    for (int i = 0; i < defined.size(); i++) {
      if (used[j] == defined[i]) {
        definedandused = true;
        break;
      }
    }
    if (!definedandused) {
      insertChild(0, used[j]);
    }
  }

}


bool chillAST_FunctionDecl::findLoopIndexesToReplace(chillAST_SymbolTable *symtab, bool forcesync) {
  if (getBody()) getBody()->findLoopIndexesToReplace(symtab, false);
  return false;
}

chillAST_MacroDefinition::chillAST_MacroDefinition(const char *mname = NULL, const char *rhs = NULL) {
  if (mname) macroName = strdup(mname); else macroName = strdup("UNDEFINEDMACRO");
  metacomment = NULL;
  parameters = new chillAST_SymbolTable();
  isFromSourceFile = true; // default
  filename = NULL;
};


chillAST_Node *chillAST_MacroDefinition::clone() {
  CHILL_ERROR("cloning a macro makes no sense\n");
  return this;
  chillAST_MacroDefinition *clo = new chillAST_MacroDefinition(macroName);
  clo->setParent(parent);
  for (int i = 0; i < parameters->size(); i++) clo->addVariableToScope((*parameters)[i]);
  clo->setBody(body->clone());
  return clo;
}


void chillAST_MacroDefinition::setBody(chillAST_Node *bod) {
  bod->setParent(this);  // well, ...
}

chillAST_ForStmt::chillAST_ForStmt() {
  children.push_back(NULL); // init
  children.push_back(NULL); // cond
  children.push_back(NULL); // incr
  children.push_back(new chillAST_CompoundStmt()); // Body

  conditionoperator = IR_COND_UNKNOWN;
  symbolTable = new chillAST_SymbolTable();
}


chillAST_ForStmt::chillAST_ForStmt(chillAST_Node *ini, chillAST_Node *con, chillAST_Node *inc, chillAST_Node *bod)
    : chillAST_ForStmt() {
  setInit(ini);
  setCond(con);
  setInc(inc);
  setBody(bod);

  if (!con->isBinaryOperator()) {
    CHILL_ERROR("ForStmt conditional is of type %s. Expecting a BinaryOperator\n", con->getTypeString());
    exit(-1);
  }
  chillAST_BinaryOperator *bo = (chillAST_BinaryOperator *) con;
  char *condstring = bo->op;
  if (!strcmp(condstring, "<")) conditionoperator = IR_COND_LT;
  else if (!strcmp(condstring, "<=")) conditionoperator = IR_COND_LE;
  else if (!strcmp(condstring, ">")) conditionoperator = IR_COND_GT;
  else if (!strcmp(condstring, ">=")) conditionoperator = IR_COND_GE;
  else {
    CHILL_ERROR("ForStmt, illegal/unhandled end condition \"%s\"\n", condstring);
    CHILL_ERROR("currently can only handle <, >, <=, >=\n");
    exit(1);
  }
}

chillAST_Node *chillAST_ForStmt::clone() {
  chillAST_ForStmt *fs = new chillAST_ForStmt(getInit()->clone(), getCond()->clone(), getInc()->clone(),
                                              getBody()->clone());
  fs->isFromSourceFile = isFromSourceFile;
  if (filename) fs->filename = strdup(filename);
  fs->setParent(getParent());
  return fs;
}

void chillAST_ForStmt::addSyncs() {
  if (!parent) {
    CHILL_ERROR("uhoh, chillAST_ForStmt::addSyncs() ForStmt has no parent!\n");

    return; // exit?
  }

  if (parent->isCompoundStmt()) {
    vector<chillAST_Node *> *chillin = parent->getChildren();
    int numc = chillin->size();
    for (int i = 0; i < numc; i++) {
      if (this == parent->getChild(i)) {
        chillAST_CudaSyncthreads *ST = new chillAST_CudaSyncthreads();
        parent->insertChild(i + 1, ST);  // corrupts something ...
      }
    }
    chillin = parent->getChildren();
    int nowc = chillin->size();
  } else {
    fprintf(stderr, "chillAST_ForStmt::addSyncs() unhandled parent type %s\n", parent->getTypeString());
    exit(-1);
  }
}


void chillAST_ForStmt::removeSyncComment() {
  if (metacomment && strstr(metacomment, "~cuda~") && strstr(metacomment, "preferredIdx: ")) {
    char *ptr = strlen("preferredIdx: ") + strstr(metacomment, "preferredIdx: ");
    *ptr = '\0';
  }
}


bool chillAST_ForStmt::findLoopIndexesToReplace(chillAST_SymbolTable *symtab, bool forcesync) {
  CHILL_DEBUG_PRINT("\nchillAST_ForStmt::findLoopIndexesToReplace( force = %d )\n", forcesync);
  bool force = forcesync;
  bool didasync = false;
  if (forcesync) {
    addSyncs();
    didasync = true;
  }
  if (metacomment && strstr(metacomment, "~cuda~") && strstr(metacomment, "preferredIdx: ")) {
    char *copy = strdup(metacomment);
    char *ptr = strstr(copy, "preferredIdx: ");
    char *vname = ptr + strlen("preferredIdx: ");
    char *space = strstr(vname, " "); // TODO index()
    if (space)
      force = true;
    if ((!didasync) && force) {
      addSyncs();
      removeSyncComment();
      didasync = true;
    }

    if (space) *space = '\0'; // if this is multiple words, grab the first one

    vector<chillAST_VarDecl *> decls;
    getInit()->gatherVarLHSUsage(decls);
    //cond->gatherVarUsage( decls );
    //incr->gatherVarUsage( decls );

    if (1 != decls.size()) {
      CHILL_ERROR("uhoh, preferred index in for statement, but multiple variables used\n");
      print(0, stderr);
      fprintf(stderr, "\nvariables are:\n");
      for (int i = 0; i < decls.size(); i++) {
        decls[i]->print(0, stderr);
        fprintf(stderr, "\n");
      }
      exit(0);
    }
    chillAST_VarDecl *olddecl = decls[0];

    // RIGHT NOW, change all the references that this loop wants swapped out
    // find vardecl for named preferred index.  it has to already exist
    chillAST_VarDecl *newguy = findVariableDecleration(vname); // recursive
    if (!newguy)
      CHILL_ERROR("there was no variable named %s anywhere I could find\n", vname);

    // wrong - this only looks at variables defined in the forstmt, not
    // in parents of the forstmt
    if (!newguy) {
      CHILL_DEBUG_PRINT("chillAST_ForStmt::findLoopIndexesToReplace() there is no defined variable %s\n", vname);

      // make one ??  seems like this should never happen
      newguy = new chillAST_VarDecl(olddecl->vartype, vname, ""/*?*/, NULL);
      // insert actual declaration in code location?   how?

      // find parent of the ForStmt?
      // find parent^n of the ForStmt that is not a Forstmt?
      // find parent^n of the Forstmt that is a FunctionDecl?
      chillAST_Node *contain = findContainingNonLoop();
      if (contain == NULL) {
        CHILL_ERROR("nothing but loops all the way up?\n");
        exit(0);
      }
      contain->print(0, stderr);
      contain->insertChild(0, newguy); // TODO ugly order
      contain->addVariableToScope(newguy); // adds to first enclosing symbolTable

      if (!symbolTableFindName(contain->getSymbolTable(), vname)) {
        CHILL_ERROR("container doesn't have a var names %s afterwards???\n", vname);
        exit(-1);
      }
    }
    // swap out old for new in init, cond, incr, body
    if (newguy)
      replaceVarDecls(olddecl, newguy);
    metacomment = NULL; // memleak
  }
  // check for more loops.  We may have already swapped variables out in body (right above here)
  getBody()->findLoopIndexesToReplace(symtab, false);
  return force;
}

void chillAST_ForStmt::gatherLoopIndeces(std::vector<chillAST_VarDecl *> &indeces) {
  vector<chillAST_VarDecl *> decls;
  getInit()->gatherVarLHSUsage(decls);
  getCond()->gatherVarLHSUsage(decls);
  getInc()->gatherVarLHSUsage(decls);
  // note: NOT GOING INTO BODY OF THE LOOP

  int numdecls = decls.size();
  for (int i = 0; i < decls.size(); i++)
    indeces.push_back(decls[i]);

  // Don't forget to keep heading upwards!
  if (parent)
    parent->gatherLoopIndeces(indeces);
}


void chillAST_ForStmt::gatherLoopVars(std::vector<std::string> &loopvars) {
  vector<chillAST_VarDecl *> decls;
  getInit()->gatherVarLHSUsage(decls);
  getCond()->gatherVarLHSUsage(decls);
  getInc()->gatherVarLHSUsage(decls);
  // note: NOT GOING INTO BODY OF THE LOOP
  for (int i = 0; i < decls.size(); i++) loopvars.push_back(strdup(decls[i]->varname));
}


void chillAST_ForStmt::loseLoopWithLoopVar(char *var) {

  //fprintf(stderr, "\nchillAST_ForStmt::loseLoopWithLoopVar(  %s )\n", var );

  // now recurse (could do first, I suppose)
  // if you DON'T do this first, you may have already replaced yourself with this loop body
  // the body will no longer have this forstmt as parent, it will have the forstmt's parent as its parent
  //fprintf(stderr, "forstmt 0x%x, recursing loseLoop to body 0x%x of type %s with parent 0x%x of type %s\n", this, body,  body->getTypeString(), body->parent, body->parent->getTypeString());
  getBody()->loseLoopWithLoopVar(var);




  // if *I* am a loop to be replaced, tell my parent to replace me with my loop body

  std::vector<std::string> loopvars;
  gatherLoopVars(loopvars);

  if (loopvars.size() != 1) {
    fprintf(stderr, "uhoh, loop has more than a single loop var and trying to loseLoopWithLoopVar()\n");
    print(0, stderr);
    fprintf(stderr, "\nvariables are:\n");
    for (int i = 0; i < loopvars.size(); i++) {
      fprintf(stderr, "%s\n", loopvars[i].c_str());
    }

    exit(-1);
  }

  //fprintf(stderr, "my loop var %s, looking for %s\n", loopvars[0].c_str(), var );
  if (!strcmp(var, loopvars[0].c_str())) {
    //fprintf(stderr, "OK, trying to lose myself!    for (");
    //init->print(0, stderr);
    //fprintf(stderr, "; ");
    //cond->print(0, stderr);
    //fprintf(stderr, "; ");
    //incr->print(0, stderr);
    //fprintf(stderr, ")\n" );

    if (!parent) {
      fprintf(stderr, "chillAST_ForStmt::loseLoopWithLoopVar()  I have no parent!\n");
      exit(-1);
    }

    vector<chillAST_VarDecl *> decls;
    getInit()->gatherVarLHSUsage(decls);   // this can fail if init is outside the loop
    getCond()->gatherVarLHSUsage(decls);
    getInc()->gatherVarLHSUsage(decls);
    if (decls.size() > 1) {
      fprintf(stderr, "chill_ast.cc multiple loop variables confuses me\n");
      exit(-1);
    }
    chillAST_Node *newstmt = getBody();

    // ACTUALLY, if I am being replaced, and my loop conditional is a min (Ternary), then wrap my loop body in an if statement
    if (getCond()->isBinaryOperator()) { // what else could it be?
      chillAST_BinaryOperator *BO = (chillAST_BinaryOperator *) getCond();
      if (BO->getRHS()->isTernaryOperator()) {

        chillAST_TernaryOperator *TO = (chillAST_TernaryOperator *) BO->getRHS();
        chillAST_BinaryOperator *C = (chillAST_BinaryOperator *) TO->getCond();

        //fprintf(stderr, "loop condition RHS  is ternary\nCondition RHS");
        C->print();
        chillAST_Node *l = C->getLHS();
        if (l->isParenExpr()) l = ((chillAST_ParenExpr *) l)->getSubExpr();
        chillAST_Node *r = C->getRHS();
        if (r->isParenExpr()) r = ((chillAST_ParenExpr *) r)->getSubExpr();

        //fprintf(stderr, "lhs is %s     rhs is %s\n", l->getTypeString(), r->getTypeString());

        chillAST_Node *ifcondrhs = NULL;
        if (!(l->isConstant())) ifcondrhs = l;
        else if (!(r->isConstant())) ifcondrhs = r;
        else {
          // should never happen. 2 constants. infinite loop
          fprintf(stderr, "chill_ast.cc INIFNITE LOOP?\n");
          this->print(0, stderr);
          fprintf(stderr, "\n\n");
          exit(-1);
        }

        // wrap the loop body in an if
        chillAST_DeclRefExpr *DRE = new chillAST_DeclRefExpr(decls[0]);
        chillAST_BinaryOperator *ifcond = new chillAST_BinaryOperator(DRE, "<=", ifcondrhs);
        chillAST_IfStmt *ifstmt = new chillAST_IfStmt(ifcond, getBody(), NULL);

        newstmt = ifstmt;
      }
    }

    //fprintf(stderr, "forstmt 0x%x has parent 0x%x  of type %s\n", this, parent, parent->getTypeString());
    //fprintf(stderr, "forstmt will be replaced by\n");
    //newstmt->print(0,stderr); fprintf(stderr, "\n\n");

    parent->replaceChild(this, newstmt);
  }


}

chillAST_BinaryOperator::chillAST_BinaryOperator() {
  children.push_back(NULL);
  children.push_back(NULL);
}

chillAST_BinaryOperator::chillAST_BinaryOperator(chillAST_Node *l, const char *oper, chillAST_Node *r)
    : chillAST_BinaryOperator() {
  CHILL_DEBUG_PRINT("( l  %s  r )\n", oper);

  setLHS(l);
  setRHS(r);

  if (oper) op = strdup(oper);

  // if this writes to lhs and lhs type has an 'imwrittento' concept, set that up
  if (isAssignmentOp()) {
    if (l && l->isArraySubscriptExpr()) {
      ((chillAST_ArraySubscriptExpr *) l)->imwrittento = true;
    }
  }
  if (isAugmentedAssignmentOp()) {  // +=  etc
    if (l && l->isArraySubscriptExpr()) {
      ((chillAST_ArraySubscriptExpr *) l)->imreadfrom = true; // note will ALSO have imwrittento true
    }
  }

  isFromSourceFile = true; // default
  filename = NULL;
}


int chillAST_BinaryOperator::evalAsInt() {
  // very limited. allow +-*/ and integer literals ...
  if (isAssignmentOp()) return getRHS()->evalAsInt();  // ?? ignores/loses lhs info

  if (!strcmp("+", op)) {
    //fprintf(stderr, "chillAST_BinaryOperator::evalAsInt()   %d + %d\n", lhs->evalAsInt(), rhs->evalAsInt());
    return getLHS()->evalAsInt() + getRHS()->evalAsInt();
  }
  if (!strcmp("-", op)) return getLHS()->evalAsInt() - getRHS()->evalAsInt();
  if (!strcmp("*", op)) return getLHS()->evalAsInt() * getRHS()->evalAsInt();
  if (!strcmp("/", op)) return getLHS()->evalAsInt() / getRHS()->evalAsInt();

  fprintf(stderr, "chillAST_BinaryOperator::evalAsInt() unhandled op '%s'\n", op);
  exit(-1);
}

chillAST_IntegerLiteral *chillAST_BinaryOperator::evalAsIntegerLiteral() {
  return new chillAST_IntegerLiteral(evalAsInt()); // ??
}

class chillAST_Node *chillAST_BinaryOperator::constantFold() {
  chillAST_Node::constantFold();

  chillAST_Node *returnval = this;

  if (getLHS()->isConstant() && getRHS()->isConstant()) {
    if (!strcmp(op, "+") || !strcmp(op, "-") || !strcmp(op, "*")) {
      if (getLHS()->isIntegerLiteral() && getRHS()->isIntegerLiteral()) {
        chillAST_IntegerLiteral *l = (chillAST_IntegerLiteral *) getLHS();
        chillAST_IntegerLiteral *r = (chillAST_IntegerLiteral *) getRHS();
        chillAST_IntegerLiteral *I;

        if (!strcmp(op, "+")) I = new chillAST_IntegerLiteral(l->value + r->value);
        if (!strcmp(op, "-")) I = new chillAST_IntegerLiteral(l->value - r->value);
        if (!strcmp(op, "*")) I = new chillAST_IntegerLiteral(l->value * r->value);

        returnval = I;
      } else {

        // usually don't want to do this for floats or doubles
        // could probably check for special cases like 0.0/30.0  or X/X  or X/1.0
        double lval, rval;
        int prec = 1;
        if (getLHS()->isIntegerLiteral())
          lval = ((chillAST_IntegerLiteral *)getLHS())->value;
        else {
          lval = ((chillAST_FloatingLiteral *)getLHS())->value;
          prec = max(prec, ((chillAST_FloatingLiteral *)getLHS())->getPrecision());
        }

        if (getRHS()->isIntegerLiteral())
          rval = ((chillAST_IntegerLiteral *)getRHS())->value;
        else {
          rval = ((chillAST_FloatingLiteral *) getRHS())->value;
          prec = max(prec, ((chillAST_FloatingLiteral *) getRHS())->getPrecision());
        }

        chillAST_FloatingLiteral *F;
        // NOT SAFE
        // if (streq(op, "+")) F = new chillAST_FloatingLiteral(lval + rval, parent);
        // if (streq(op, "-")) F = new chillAST_FloatingLiteral(lval - rval, parent);
        if (streq(op, "*")) F = new chillAST_FloatingLiteral(lval * rval, prec, NULL);

        returnval = F;

      }
    }
    //else fprintf(stderr, "can't fold op '%s' yet\n", op);
  }

  //fprintf(stderr, "returning "); returnval->print(0,stderr); fprintf(stderr, "\n");
  return returnval;
}


class chillAST_Node *chillAST_BinaryOperator::clone() {
  //fprintf(stderr, "chillAST_BinaryOperator::clone() "); print(); printf("\n"); fflush(stdout);

  chillAST_Node *l = getLHS()->clone();
  chillAST_Node *r = getRHS()->clone();
  chillAST_BinaryOperator *bo = new chillAST_BinaryOperator(l, op, r);
  l->setParent(bo);
  r->setParent(bo);
  bo->isFromSourceFile = isFromSourceFile;
  if (filename) bo->filename = strdup(filename);
  return bo;
}

void chillAST_BinaryOperator::gatherArrayRefs(std::vector<chillAST_ArraySubscriptExpr *> &refs, bool w) {
  getLHS()->gatherArrayRefs(refs, isAssignmentOp());
  getRHS()->gatherArrayRefs(refs, 0);
}

void chillAST_BinaryOperator::gatherScalarRefs(std::vector<chillAST_DeclRefExpr *> &refs, bool writtento) {
  getLHS()->gatherScalarRefs(refs, isAssignmentOp());
  getRHS()->gatherScalarRefs(refs, 0);
}

void chillAST_BinaryOperator::gatherVarLHSUsage(vector<chillAST_VarDecl *> &decls) {
  getLHS()->gatherVarUsage(decls);
}

chillAST_TernaryOperator::chillAST_TernaryOperator() {
  op = strdup("?");
  children.push_back(NULL);
  children.push_back(NULL);
  children.push_back(NULL);
}

chillAST_TernaryOperator::chillAST_TernaryOperator(const char *oper, chillAST_Node *c, chillAST_Node *l,
                                                   chillAST_Node *r) : chillAST_TernaryOperator() {
  if (op) op = strdup(oper);
  setCond(c);
  setLHS(l);
  setRHS(r);
}

void chillAST_TernaryOperator::gatherVarLHSUsage(vector<chillAST_VarDecl *> &decls) {
  // this makes no sense for ternary ??
}

class chillAST_Node *chillAST_TernaryOperator::constantFold() {
  chillAST_Node::constantFold();

  chillAST_Node *returnval = this;

  if (getCond()->isConstant()) {
    // TODO
  }

  return returnval;
}

class chillAST_Node *chillAST_TernaryOperator::clone() {
  chillAST_Node *c = getCond()->clone();
  chillAST_Node *l = getLHS()->clone();
  chillAST_Node *r = getRHS()->clone();
  chillAST_TernaryOperator *to = new chillAST_TernaryOperator(op, c, l, r);
  to->isFromSourceFile = isFromSourceFile;
  filename = NULL;
  return to;
}

chillAST_ArraySubscriptExpr::chillAST_ArraySubscriptExpr() {
  children.push_back(NULL); // Base
  children.push_back(NULL); // Index
  basedecl = NULL;
}

chillAST_ArraySubscriptExpr::chillAST_ArraySubscriptExpr(chillAST_Node *bas, chillAST_Node *indx, bool writtento,
                                                         void *unique):chillAST_ArraySubscriptExpr() {
  imwrittento = writtento; // ??
  imreadfrom = false; // ??
  if (bas) {
    if (bas->isImplicitCastExpr()) setBase(((chillAST_ImplicitCastExpr *) bas)->getSubExpr()); // probably wrong
    else setBase(bas);
    basedecl = multibase();
  }
  if (indx) {
    if (indx->isImplicitCastExpr()) setIndex(((chillAST_ImplicitCastExpr *) indx)->getSubExpr()); // probably wrong
    else setIndex(indx);
  }
  uniquePtr = unique;
}

chillAST_ArraySubscriptExpr::chillAST_ArraySubscriptExpr(chillAST_VarDecl *v, std::vector<chillAST_Node *> indeces):chillAST_ArraySubscriptExpr() {
  int numindeces = indeces.size();
  for (int i = 0; i < numindeces; i++) {
    fprintf(stderr, "ASE index %d  ", i);
    indeces[i]->print(0, stderr);
    fprintf(stderr, "\n");
  }
  chillAST_DeclRefExpr *DRE = new chillAST_DeclRefExpr(v->vartype, v->varname, v);
  basedecl = v; // ??

  chillAST_ArraySubscriptExpr *rent = this; // parent for subnodes

  // these are on the top level ASE that we're creating here
  setBase(DRE);
  setIndex(indeces[numindeces - 1]);

  for (int i = numindeces - 2; i >= 0; i--) {

    chillAST_ArraySubscriptExpr *ASE = new chillAST_ArraySubscriptExpr(DRE, indeces[i], rent, 0);
    rent->setBase(ASE);
  }

  imwrittento = false;
  imreadfrom = false;
  isFromSourceFile = true;
  filename = NULL;
}

void chillAST_ArraySubscriptExpr::gatherIndeces(std::vector<chillAST_Node *> &ind) {
  if (getBase()->isArraySubscriptExpr()) ((chillAST_ArraySubscriptExpr*)getBase())->gatherIndeces(ind);
  ind.push_back(getIndex());
}

chillAST_VarDecl *chillAST_ArraySubscriptExpr::multibase() {
  return getBase()->multibase();
}


chillAST_Node *chillAST_ArraySubscriptExpr::getIndex(int dim) {
  CHILL_DEBUG_PRINT("chillAST_ArraySubscriptExpr::getIndex( %d )\n", dim);
  chillAST_Node *b = getBase();
  int depth = 0;
  std::vector<chillAST_Node *> ind;
  chillAST_Node *curindex = getIndex();
  for (;;) {
    if (b->getType() == CHILLAST_NODE_IMPLICITCASTEXPR)
      b = ((chillAST_ImplicitCastExpr *) b)->getSubExpr();
    else if (b->getType() == CHILLAST_NODE_ARRAYSUBSCRIPTEXPR) {
      ind.push_back(curindex);
      curindex = ((chillAST_ArraySubscriptExpr *) b)->getIndex();
      b = ((chillAST_ArraySubscriptExpr *) b)->getBase();
      depth++;
    } else {
      ind.push_back(curindex);
      break;
    }
  }
  return ind[depth - dim];
}

class chillAST_Node *chillAST_ArraySubscriptExpr::clone() {
  chillAST_Node *b = getBase()->clone();
  chillAST_Node *i = getIndex()->clone();
  chillAST_ArraySubscriptExpr *ASE = new chillAST_ArraySubscriptExpr(b, i, imwrittento, uniquePtr);
  ASE->setParent(parent);
  ASE->imreadfrom = false; // don't know this yet
  ASE->isFromSourceFile = isFromSourceFile;
  if (filename) ASE->filename = strdup(filename);
  return ASE;
}

void chillAST_ArraySubscriptExpr::gatherArrayRefs(std::vector<chillAST_ArraySubscriptExpr *> &refs, bool writtento) {
  if (!imwrittento) imwrittento = writtento;   // may be both written and not for += TODO Purpose unclear
  getIndex()->gatherArrayRefs(refs, 0); // recurse first
  refs.push_back(this);
}

void chillAST_ArraySubscriptExpr::gatherScalarRefs(std::vector<chillAST_DeclRefExpr *> &refs, bool writtento) {
  chillAST_Node::gatherScalarRefs(refs, false);
}

bool chillAST_ArraySubscriptExpr::operator!=(const chillAST_ArraySubscriptExpr &other) {
  bool opposite = *this == other;
  return !opposite;
}


bool chillAST_ArraySubscriptExpr::operator==(const chillAST_ArraySubscriptExpr &other) {
  return this->uniquePtr == other.uniquePtr;
}

chillAST_MemberExpr::chillAST_MemberExpr() {
  children.push_back(NULL);
}
chillAST_MemberExpr::chillAST_MemberExpr(chillAST_Node *bas, const char *mem, void *unique,
                                         CHILLAST_MEMBER_EXP_TYPE t):chillAST_MemberExpr() {
  setBase(bas);
  if (mem) member = strdup(mem);
  else
    member = NULL;
  uniquePtr = unique;
  exptype = t;

  return;
}

// TODO member can be another member expression, Right?

class chillAST_Node *chillAST_MemberExpr::clone() {
  chillAST_Node *b = getBase()->clone();
  char *m = strdup(member);
  chillAST_MemberExpr *ME = new chillAST_MemberExpr(b, m, uniquePtr);
  ME->isFromSourceFile = isFromSourceFile;
  if (filename) ME->filename = strdup(filename);
  return ME;
}

bool chillAST_MemberExpr::operator!=(const chillAST_MemberExpr &other) {
  bool opposite = *this == other;
  return !opposite;
}

bool chillAST_MemberExpr::operator==(const chillAST_MemberExpr &other) {
  return this->uniquePtr == other.uniquePtr;
}

chillAST_Node *chillAST_MemberExpr::multibase2() {  /*fprintf(stderr, "ME MB2\n" );*/ return (chillAST_Node *) this; }

chillAST_VarDecl *chillAST_MemberExpr::getUnderlyingVarDecl() {
  CHILL_ERROR("chillAST_MemberExpr:getUnderlyingVarDecl()\n");
  print();
  exit(-1);
  // find the member with the correct name

}


chillAST_VarDecl *chillAST_MemberExpr::multibase() {
  chillAST_VarDecl *vd = getBase()->multibase(); // ??

  chillAST_RecordDecl *rd = vd->getStructDef();
  if (!rd) {
    fprintf(stderr, "chillAST_MemberExpr::multibase() vardecl is not a struct??\n");
    fprintf(stderr, "vd ");
    vd->print();
    fprintf(stderr, "vd ");
    vd->dump();
    exit(-1);
  }
  // OK, we have the recorddecl that defines the structure
  // now find the member with the correct name
  chillAST_VarDecl *sub = rd->findSubpart(member);
  if (!sub) {
    fprintf(stderr, "can't find member %s in \n", member);
    rd->print();
  }
  return sub;
}

chillAST_DeclRefExpr::chillAST_DeclRefExpr(const char *vartype, const char *varname, chillAST_Node *d) {
  if (vartype) declarationType = strdup(vartype);
  else declarationType = strdup("UNKNOWN");
  if (varname) declarationName = strdup(varname);
  else declarationName = strdup("NONE");
  decl = d;
}

chillAST_DeclRefExpr::chillAST_DeclRefExpr(chillAST_VarDecl *vd) {
  declarationType = strdup(vd->vartype);
  declarationName = strdup(vd->varname);
  decl = vd;
}


chillAST_DeclRefExpr::chillAST_DeclRefExpr(chillAST_FunctionDecl *fd) {
  declarationType = strdup(fd->returnType);
  declarationName = strdup(fd->functionName);
  decl = fd;
}

class chillAST_Node *chillAST_DeclRefExpr::clone() {
  chillAST_DeclRefExpr *DRE = new chillAST_DeclRefExpr(declarationType, declarationName, decl);
  DRE->setParent(parent);
  DRE->isFromSourceFile = isFromSourceFile;
  if (filename) DRE->filename = strdup(filename);
  return DRE;
}


void chillAST_DeclRefExpr::gatherVarDeclsMore(vector<chillAST_VarDecl *> &decls) {
  decl->gatherVarDeclsMore(decls);
}

void chillAST_DeclRefExpr::gatherDeclRefExprs(vector<chillAST_DeclRefExpr *> &refs) {
  refs.push_back(this);
}

void chillAST_DeclRefExpr::gatherScalarRefs(std::vector<chillAST_DeclRefExpr *> &refs, bool writtento) {
  refs.push_back(this);
}

void chillAST_DeclRefExpr::gatherVarUsage(vector<chillAST_VarDecl *> &decls) {
  for (int i = 0; i < decls.size(); i++) {
    if (decls[i] == decl) {
      return;
    }
    if (streq(declarationName, decls[i]->varname)) {
      if (streq(declarationType, decls[i]->vartype)) {
        CHILL_ERROR("Deprecated\n");
        return;
      }
    }
  }
  chillAST_VarDecl *vd = getVarDecl();  // null for functiondecl
  if (vd) decls.push_back(vd);
}


void chillAST_DeclRefExpr::replaceVarDecls(chillAST_VarDecl *olddecl, chillAST_VarDecl *newdecl) {
  if (decl == olddecl) {
    decl = newdecl;
    declarationType = strdup(newdecl->vartype);
    declarationName = strdup(newdecl->varname);
  } else {
    CHILL_ERROR("Deprecated\n");
    if (!strcmp(olddecl->varname, declarationName)) {
      decl = newdecl;
      declarationType = strdup(newdecl->vartype);
      declarationName = strdup(newdecl->varname);
    }
  }
}

chillAST_VarDecl *chillAST_ImplicitCastExpr::multibase() {
  return getSubExpr()->multibase();
}


chillAST_VarDecl *chillAST_DeclRefExpr::multibase() {
  // presumably, this is being called because this DRE is the base of an ArraySubscriptExpr
  return getVarDecl();
}


void chillAST_VarDecl::gatherVarDecls(vector<chillAST_VarDecl *> &decls) {
  for (int i = 0; i < decls.size(); i++) {
    if (decls[i] == this) {
      return;
    }
    if (streq(decls[i]->varname, varname)) {
      if (streq(decls[i]->vartype, vartype)) {
        CHILL_ERROR("Wrong\n");
        return;
      }
    }
  }
  decls.push_back(this);
}


void chillAST_VarDecl::gatherScalarVarDecls(vector<chillAST_VarDecl *> &decls) {
  if (numdimensions != 0) return; // not a scalar

  for (int i = 0; i < decls.size(); i++) {
    if (decls[i] == this) {
      return;
    }
    if (streq(decls[i]->varname, varname)) {      // wrong. scoping.  TODO
      if (streq(decls[i]->vartype, vartype)) {
        CHILL_ERROR("Wrong\n");
        return;
      }
    }
  }
  decls.push_back(this);
}


void chillAST_VarDecl::gatherArrayVarDecls(vector<chillAST_VarDecl *> &decls) {
  if (numdimensions == 0) return; // not an array

  for (int i = 0; i < decls.size(); i++) {
    if (decls[i] == this) {
      return;
    }

    if (streq(decls[i]->varname, varname)) {      // wrong. scoping.  TODO
      if (streq(decls[i]->vartype, vartype)) {
        CHILL_ERROR("Wrong\n");
        return;
      }
    }
  }
  decls.push_back(this);
}


chillAST_Node *chillAST_VarDecl::clone() {
  chillAST_VarDecl *vd = new chillAST_VarDecl(vartype, strdup(varname), arraypart,
                                              NULL);  // NULL so we don't add the variable AGAIN to the (presumably) function

  vd->typedefinition = typedefinition;
  vd->vardef = vardef; // perhaps should not do this     TODO
  //vd->isStruct = (vardef != NULL); // ??

  vd->underlyingtype = strdup(underlyingtype);

  vd->arraysizes = NULL;
  vd->knownArraySizes = knownArraySizes;
  vd->numdimensions = numdimensions;
  vd->arraypointerpart = NULL;

  if (arraypart != NULL && NULL != arraysizes) {  // !strcmp(arraypart, "")) {
    vd->numdimensions = numdimensions;

    if (arraysizes) {
      vd->arraysizes = (int *) malloc(sizeof(int *) * numdimensions);
      for (int i = 0; i < numdimensions; i++)
        vd->arraysizes[i] = arraysizes[i];
    }
  }

  if (arraypointerpart)
    vd->arraypointerpart = strdup(arraypointerpart);

  vd->isStruct = this->isStruct;
  vd->knownArraySizes = this->knownArraySizes;
  vd->isFromSourceFile = isFromSourceFile;
  if (filename) vd->filename = strdup(filename);
  return vd;
}


void chillAST_VarDecl::splitarraypart() {
  if (arraypart) CHILL_DEBUG_PRINT("%s\n", arraypart);

  // split arraypart into  (leading??) asterisks and known sizes [1][2][3]
  if (!arraypart ||  // NULL
      (arraypart && (*arraypart == '\0'))) { // or empty string

    // parts are both empty string
    if (arraypointerpart) free(arraypointerpart);
    arraypointerpart = strdup("");
    if (arraysetpart) free(arraysetpart);
    arraysetpart = strdup("");
    return;
  }

  // arraypart exists and is not empty
  int asteriskcount = 0;
  int fixedcount = 0;
  for (int i = 0; i < strlen(arraypart); i++) {
    if (arraypart[i] == '*') {
      if (fixedcount) {
        CHILL_ERROR("illegal vardecl arraypart: '%s'\n", arraypart);
        exit(-1);
        exit(-1);
      }
      asteriskcount++;
    } else { // remainder is fixed?
      fixedcount++;
      // check for brackets and digits only?   TODO
    }
  }
  arraypointerpart = (char *) calloc(asteriskcount + 1, sizeof(char));
  arraysetpart = (char *) calloc(fixedcount + 1, sizeof(char));
  char *ptr = arraypart;
  for (int i = 0; i < asteriskcount; i++) arraypointerpart[i] = *ptr++;
  for (int i = 0; i < fixedcount; i++) arraysetpart[i] = *ptr++;
}


chillAST_IntegerLiteral::chillAST_IntegerLiteral(int val) {
  value = val;
  isFromSourceFile = true; // default
  filename = NULL;
}

class chillAST_Node *chillAST_IntegerLiteral::clone() {

  chillAST_IntegerLiteral *IL = new chillAST_IntegerLiteral(value);
  IL->isFromSourceFile = isFromSourceFile;
  if (filename) IL->filename = strdup(filename);
  return IL;

}

chillAST_FloatingLiteral::chillAST_FloatingLiteral(double val, int precis, const char *printthis) {
  value = val;
  precision = precis;
  allthedigits = NULL;
  if (printthis)
    allthedigits = strdup(printthis);
  isFromSourceFile = true;
  filename = NULL;
}


chillAST_FloatingLiteral::chillAST_FloatingLiteral(chillAST_FloatingLiteral *old) {
  value = old->value;
  allthedigits = NULL;
  if (old->allthedigits) allthedigits = strdup(old->allthedigits);
  precision = old->precision;
  isFromSourceFile = true; // default
  filename = NULL;
}

chillAST_Node *chillAST_FloatingLiteral::clone() {
  chillAST_FloatingLiteral *newone = new chillAST_FloatingLiteral(this);
  newone->isFromSourceFile = isFromSourceFile;
  if (filename) newone->filename = strdup(filename);
  return newone;
}

chillAST_UnaryOperator::chillAST_UnaryOperator() {
  children.push_back(NULL);
}

chillAST_UnaryOperator::chillAST_UnaryOperator(const char *oper, bool pre, chillAST_Node *sub)
    : chillAST_UnaryOperator() {
  op = strdup(oper);
  prefix = pre;
  setSubExpr(sub);
  isFromSourceFile = true; // default
  filename = NULL;
}

void chillAST_UnaryOperator::gatherArrayRefs(std::vector<chillAST_ArraySubscriptExpr *> &refs, bool w) {
  getSubExpr()->gatherArrayRefs(refs, isAssignmentOp()); //
}

void chillAST_UnaryOperator::gatherVarLHSUsage(vector<chillAST_VarDecl *> &decls) {
  if (isAssignmentOp()) {
    getSubExpr()->gatherVarUsage(decls); // do all unary modify the subexpr? (no, - )
  }
}


chillAST_Node *chillAST_UnaryOperator::constantFold() {
  chillAST_Node::constantFold();

  chillAST_Node *returnval = this;
  if (getSubExpr()->isConstant()) {
    if (streq(op, "-")) {
      if (getSubExpr()->isIntegerLiteral()) {
        int intval = ((chillAST_IntegerLiteral *) getSubExpr())->value;
        chillAST_IntegerLiteral *I = new chillAST_IntegerLiteral(-intval);
        I->setParent(parent);
        returnval = I;
      } else {
        chillAST_FloatingLiteral *FL = (chillAST_FloatingLiteral *) getSubExpr();
        chillAST_FloatingLiteral *F = new chillAST_FloatingLiteral(FL); // clone
        F->parent = FL->parent;

        F->value = -F->value;
        returnval = F;
      }
    } else CHILL_DEBUG_PRINT("can't fold op '%s' yet\n", op);
  }
  return returnval;
}


class chillAST_Node *chillAST_UnaryOperator::clone() {
  chillAST_UnaryOperator *UO = new chillAST_UnaryOperator(op, prefix, getSubExpr()->clone());
  UO->setParent(parent);
  UO->isFromSourceFile = isFromSourceFile;
  if (filename) UO->filename = strdup(filename);
  return UO;
}

int chillAST_UnaryOperator::evalAsInt() {
  if (!strcmp("+", op)) return getSubExpr()->evalAsInt();
  if (!strcmp("-", op)) return -getSubExpr()->evalAsInt();
  if (!strcmp("++", op)) return 1 + getSubExpr()->evalAsInt();
  if (!strcmp("--", op)) return getSubExpr()->evalAsInt() - 1;

  fprintf(stderr, "chillAST_UnaryOperator::evalAsInt() unhandled op '%s'\n", op);
  exit(-1);
}

chillAST_ImplicitCastExpr::chillAST_ImplicitCastExpr() {
  children.push_back(NULL);
}

chillAST_Node* chillAST_ImplicitCastExpr::constantFold() {
  chillAST_Node::constantFold();
  return getSubExpr();
}

chillAST_ImplicitCastExpr::chillAST_ImplicitCastExpr(chillAST_Node *sub) : chillAST_ImplicitCastExpr() {
  setSubExpr(sub);
}

class chillAST_Node *chillAST_ImplicitCastExpr::clone() {
  chillAST_ImplicitCastExpr *ICE = new chillAST_ImplicitCastExpr(getSubExpr()->clone());
  ICE->isFromSourceFile = isFromSourceFile;
  if (filename) ICE->filename = strdup(filename);
  return ICE;
}

chillAST_CStyleCastExpr::chillAST_CStyleCastExpr() {
  children.push_back(NULL);
}

chillAST_CStyleCastExpr::chillAST_CStyleCastExpr(const char *to, chillAST_Node *sub) : chillAST_CStyleCastExpr() {
  towhat = strdup(to);
  setSubExpr(sub);
}

chillAST_Node * chillAST_CStyleCastExpr::constantFold() {
  chillAST_Node::constantFold();
  if (getSubExpr()->isConstant()) {
    double val;
    if (getSubExpr() -> isIntegerLiteral()) {
      val = getSubExpr()->evalAsInt();
    } else if (getSubExpr() -> isFloatingLiteral()) {
      val = ((chillAST_FloatingLiteral*)getSubExpr())->value;
    } else return this;
    if (!strcmp(towhat,"float"))
      return new chillAST_FloatingLiteral(val,1,NULL);
    if (!strcmp(towhat, "double"))
      return new chillAST_FloatingLiteral(val,2,NULL);
  }
  return this;
}

class chillAST_Node *chillAST_CStyleCastExpr::clone() {
  chillAST_CStyleCastExpr *CSCE = new chillAST_CStyleCastExpr(towhat, getSubExpr()->clone());
  CSCE->setParent(getParent());
  CSCE->isFromSourceFile = isFromSourceFile;
  if (filename) CSCE->filename = strdup(filename);
  return CSCE;
}

chillAST_CStyleAddressOf::chillAST_CStyleAddressOf() {
  children.push_back(NULL);
}

chillAST_CStyleAddressOf::chillAST_CStyleAddressOf(chillAST_Node *sub) : chillAST_CStyleAddressOf() {
  setSubExpr(sub);
}

class chillAST_Node *chillAST_CStyleAddressOf::clone() {
  chillAST_CStyleAddressOf *CSAO = new chillAST_CStyleAddressOf(getSubExpr()->clone());
  CSAO->setParent(getParent());
  CSAO->isFromSourceFile = isFromSourceFile;
  if (filename) CSAO->filename = strdup(filename);
  return CSAO;
}

chillAST_Malloc::chillAST_Malloc() {
  children.push_back(NULL);
}

chillAST_Malloc::chillAST_Malloc(chillAST_Node *numthings) : chillAST_Malloc() {
  setSize(numthings);
  isFromSourceFile = true; // default
  filename = NULL;
};

chillAST_Node *chillAST_Malloc::clone() {
  chillAST_Malloc *M = new chillAST_Malloc(getSize()->clone()); // the general version
  M->setParent(getParent());
  M->isFromSourceFile = isFromSourceFile;
  if (filename) M->filename = strdup(filename);
  return M;
};

chillAST_CudaMalloc::chillAST_CudaMalloc() {
  children.push_back(NULL);
  children.push_back(NULL);
}

chillAST_CudaMalloc::chillAST_CudaMalloc(chillAST_Node *devmemptr, chillAST_Node *size) : chillAST_CudaMalloc() {
  setDevPtr(devmemptr);
  setSize(size);
};

class chillAST_Node *chillAST_CudaMalloc::clone() {
  chillAST_CudaMalloc *CM = new chillAST_CudaMalloc(getDevPtr()->clone(), getSize()->clone());
  CM->setParent(getParent());
  CM->isFromSourceFile = isFromSourceFile;
  if (filename) CM->filename = strdup(filename);
  return CM;
}

void chillAST_CudaMalloc::gatherArrayRefs(std::vector<chillAST_ArraySubscriptExpr *> &refs, bool w) {
  chillAST_Node::gatherArrayRefs(refs, false);
}

void chillAST_CudaMalloc::gatherScalarRefs(std::vector<chillAST_DeclRefExpr *> &refs, bool writtento) {
  chillAST_Node::gatherScalarRefs(refs, false);
}

chillAST_CudaFree::chillAST_CudaFree() {
  children.push_back(NULL);
}

chillAST_CudaFree::chillAST_CudaFree(chillAST_Node *var) : chillAST_CudaFree() {
  setParent(var);
};

class chillAST_Node *chillAST_CudaFree::clone() {
  chillAST_CudaFree *CF = new chillAST_CudaFree(getPointer()->clone());
  CF->setParent(getParent());
  CF->isFromSourceFile = isFromSourceFile;
  if (filename) CF->filename = strdup(filename);
  return CF;
}

chillAST_CudaMemcpy::chillAST_CudaMemcpy() {
  addChild(NULL);
  addChild(NULL);
  addChild(NULL);
}

chillAST_CudaMemcpy::chillAST_CudaMemcpy(chillAST_Node *d, chillAST_Node *s, chillAST_Node *siz, char *kind) {
  setDest(d);
  setSrc(s);
  setSize(siz);
  cudaMemcpyKind = kind;
};

class chillAST_Node *chillAST_CudaMemcpy::clone() {
  chillAST_CudaMemcpy *CMCPY = new chillAST_CudaMemcpy(getDest()->clone(),
                                                       getSrc()->clone(), getSize()->clone(),
                                                       strdup(cudaMemcpyKind));
  CMCPY->setParent(getParent());
  CMCPY->isFromSourceFile = isFromSourceFile;
  if (filename) CMCPY->filename = strdup(filename);
  return CMCPY;
}

void chillAST_CudaMemcpy::gatherArrayRefs(std::vector<chillAST_ArraySubscriptExpr *> &refs, bool w) {
  chillAST_Node::gatherArrayRefs(refs, false);
}

void chillAST_CudaMemcpy::gatherScalarRefs(std::vector<chillAST_DeclRefExpr *> &refs, bool writtento) {
  chillAST_Node::gatherScalarRefs(refs, false);
}

chillAST_CudaSyncthreads::chillAST_CudaSyncthreads() {
}

chillAST_ReturnStmt::chillAST_ReturnStmt() {
  children.push_back(NULL);
}

chillAST_ReturnStmt::chillAST_ReturnStmt(chillAST_Node *retval) : chillAST_ReturnStmt() {
  setRetVal(retval);
}

class chillAST_Node *chillAST_ReturnStmt::clone() {
  chillAST_Node *val = NULL;
  if (getRetVal()) val = getRetVal()->clone();
  chillAST_ReturnStmt *RS = new chillAST_ReturnStmt(val);
  RS->setParent(getParent());
  RS->isFromSourceFile = isFromSourceFile;
  if (filename) RS->filename = strdup(filename);
  return RS;
}

chillAST_CallExpr::chillAST_CallExpr() {
  children.push_back(NULL);
}

chillAST_CallExpr::chillAST_CallExpr(chillAST_Node *c) : chillAST_CallExpr() {
  setCallee(c);
  grid = block = NULL;
}

chillAST_Node *chillAST_CallExpr::clone() {
  chillAST_CallExpr *CE = new chillAST_CallExpr(getCallee()->clone());
  for (int i = 1; i < getNumChildren(); i++) CE->addArg(getChild(i)->clone());
  CE->isFromSourceFile = isFromSourceFile;
  if (filename) CE->filename = strdup(filename);
  return CE;
}


chillAST_VarDecl::chillAST_VarDecl() {
  vartype = underlyingtype = varname = arraypart = arraypointerpart = arraysetpart = NULL;
  typedefinition = NULL;
  init = NULL;
  numdimensions = 0;
  arraysizes = NULL;
  parent = NULL;
  metacomment = NULL;
  vardef = NULL;
  isStruct = false;
  isAParameter = false;
  byreference = false;
  isABuiltin = false;
  isRestrict = isDevice = isShared = false;
  knownArraySizes = false;
};


chillAST_VarDecl::chillAST_VarDecl(const char *t, const char *n, const char *a) {
  vartype = strdup(t);
  typedefinition = NULL;

  underlyingtype = parseUnderlyingType(vartype);
  varname = strdup(n);
  arraypointerpart = arraysetpart = NULL;
  if (a) arraypart = strdup(a);
  else arraypart = strdup("");
  splitarraypart();

  init = NULL;
  numdimensions = 0;
  arraysizes = NULL;
  uniquePtr = NULL;

  knownArraySizes = false;
  for (int i = 0; i < strlen(a); i++) {
    if (a[i] == '[') {
      numdimensions++;
      knownArraySizes = true;
    }
    if (!knownArraySizes && a[i] == '*') numdimensions++;
  }

  vardef = NULL;
  isStruct = false;

  //insideAStruct = false;
  isAParameter = false;
  byreference = false;
  isABuiltin = false;
  isRestrict = isDevice = isShared = false; // fprintf(stderr, "RDS = false\n");
};


chillAST_VarDecl::chillAST_VarDecl(chillAST_RecordDecl *astruct, const char *nam, const char *array) {
  // define a variable whose type is a struct!
  const char *type = astruct->getName();

  vartype = strdup(type);

  // these always go together  ??
  vardef = astruct;// pointer to the thing that says what is inside the struct
  isStruct = true;  // ?? wrong if it's a union  ?? TODO

  underlyingtype = parseUnderlyingType(vartype);
  varname = strdup(nam);
  arraypart = strdup(array);
  arraypointerpart = arraysetpart = NULL;
  splitarraypart();

  init = NULL;
  numdimensions = 0;
  arraysizes = NULL;
  uniquePtr = NULL;

  knownArraySizes = false;
  for (int i = 0; i < strlen(array); i++) {
    if (array[i] == '[') {
      numdimensions++;
      knownArraySizes = true;
    }
    if (!knownArraySizes && array[i] == '*') numdimensions++;
  }

  isAParameter = false;
  fprintf(stderr, "%s is NOT a parameter\n", nam);
  byreference = false;
  isABuiltin = false;
  isRestrict = isDevice = isShared = false; // fprintf(stderr, "RDS = false\n");
  typedefinition = NULL;
};


chillAST_VarDecl::chillAST_VarDecl(chillAST_TypedefDecl *tdd, const char *n, const char *a) {
  const char *type = tdd->getStructName();
  typedefinition = tdd;
  vartype = strdup(type);
  underlyingtype = parseUnderlyingType(vartype);
  varname = strdup(n);
  arraypart = strdup(a);
  arraypointerpart = arraysetpart = NULL;
  splitarraypart();

  init = NULL;
  numdimensions = 0;
  arraysizes = NULL;
  uniquePtr = NULL;
  knownArraySizes = false;
  for (int i = 0; i < strlen(a); i++) {
    if (a[i] == '[') {
      numdimensions++;
      knownArraySizes = true;
    }
    if (!knownArraySizes && a[i] == '*') numdimensions++;
  }
  isStruct = tdd->isAStruct();
  vardef = NULL;
  isAParameter = false;
  byreference = false;
  isABuiltin = false;
  isRestrict = isDevice = isShared = false;
};


chillAST_VarDecl::chillAST_VarDecl(const char *t, const char *n, const char *a, void *ptr) {
  CHILL_DEBUG_PRINT("chillAST_VarDecl::chillAST_VarDecl( type %s, name %s, arraypart '%s' ) %p\n", t, n, a, this);
  vartype = strdup(t);
  typedefinition = NULL;
  underlyingtype = parseUnderlyingType(vartype);
  varname = strdup(n);
  vardef = NULL;  // not a struct
  isStruct = false;
  isAParameter = false;
  if (a) arraypart = strdup(a);
  else arraypart = strdup(""); // should catch this earlier
  arraypointerpart = arraysetpart = NULL;
  splitarraypart();
  init = NULL;
  numdimensions = 0;
  arraysizes = NULL;
  uniquePtr = ptr;
  knownArraySizes = false;

  for (int i = 0; i < strlen(a); i++) {
    if (a[i] == '[') {
      numdimensions++;
      knownArraySizes = true;
    }
    if (!knownArraySizes && a[i] == '*') numdimensions++; // fails for  a[4000 * 4]
  }
  // this is from ir_clang.cc ConvertVarDecl(), that got executed AFTER the vardecl was constructed. dumb
  int numdim = 0;
  //knownArraySizes = true;
  //if (index(vartype, '*')) knownArraySizes = false;  // float *a;   for example
  //if (index(arraypart, '*'))  knownArraySizes = false;

  // note: vartype here, arraypart in next code..    is that right?
  if (index(vartype, '*')) {
    for (int i = 0; i < strlen(vartype); i++) if (vartype[i] == '*') numdim++;
    numdimensions = numdim;
  }

  if (index(arraypart, '[')) {  // JUST [12][34][56]  no asterisks
    char *dupe = strdup(arraypart);

    int len = strlen(arraypart);
    for (int i = 0; i < len; i++) if (dupe[i] == '[') numdim++;

    numdimensions = numdim;
    int *as = (int *) malloc(sizeof(int *) * numdim);
    if (!as) {
      CHILL_ERROR("can't malloc array sizes in ConvertVarDecl()\n");
      exit(-1);
    }
    arraysizes = as; // 'as' changed later!


    char *ptr = dupe;
    while (ptr = index(ptr, '[')) {                   // this fails for float a[4000*4]
      ptr++;
      char *leak = strdup(ptr);
      char *close = index(leak, ']');
      if (close) *close = '\0';

      int l = strlen(leak);
      bool justdigits = true;
      bool justmath = true;
      for (int i = 0; i < l; i++) {
        char c = leak[i];
        if (!isdigit(c)) justdigits = false;
        if (!(isdigit(c) ||
              isblank(c) ||
              ((c == '+') || (c == '*') || (c == '*') || (c == '*')) || // math
              ((c == '(') || (c == ')')))
            ) {
          justmath = false;
        }
      }
      if (justdigits) {
        int dim;
        sscanf(ptr, "%d", &dim);
        *as++ = dim;
      } else {
        if (justmath) fprintf(stderr, "JUST MATH\n");
        fprintf(stderr, "need to evaluate %s, faking with hardcoded 16000\n", leak);
        *as++ = 16000; // temp TODO DFL
      }
      free(leak);
      ptr = index(ptr, ']');
    }
    free(dupe);
  }

  byreference = false;
  isABuiltin = false;
  isRestrict = isDevice = isShared = false;
  // currently this is bad, because a struct does not have a symbol table, so the
  // members of a struct are passed up to the func or sourcefile.
};

chillAST_RecordDecl *chillAST_VarDecl::getStructDef() {
  if (vardef) return vardef;
  if (typedefinition) return typedefinition->getStructDef();
  return NULL;
}


chillAST_CompoundStmt::chillAST_CompoundStmt() {
  parent = NULL;
  symbolTable = new chillAST_SymbolTable;
  typedefTable = new chillAST_TypedefTable;
};

void chillAST_CompoundStmt::replaceChild(chillAST_Node *old, chillAST_Node *newchild) {
  vector<chillAST_Node *> dupe = children;
  int numdupe = dupe.size();
  int any = 0;

  for (int i = 0; i < numdupe; i++) {
    if (dupe[i] == old) {
      children[i] = newchild;
      newchild->setParent(this);
      any = 1;
    }
  }

  if (!any) {
    CHILL_ERROR("chillAST_CompoundStmt::replaceChild(), could not find old\n");
    exit(-1);
  }
}

chillAST_Node *chillAST_CompoundStmt::clone() {
  chillAST_CompoundStmt *cs = new chillAST_CompoundStmt();
  for (int i = 0; i < children.size(); i++) cs->addChild(children[i]->clone());
  cs->setParent(getParent());
  cs->isFromSourceFile = isFromSourceFile;
  if (filename) cs->filename = strdup(filename);
  return cs;
}

void chillAST_CompoundStmt::gatherArrayRefs(std::vector<chillAST_ArraySubscriptExpr *> &refs, bool writtento) {
  chillAST_Node::gatherArrayRefs(refs,false);
}

void chillAST_CompoundStmt::gatherScalarRefs(std::vector<chillAST_DeclRefExpr *> &refs, bool writtento) {
  chillAST_Node::gatherScalarRefs(refs,false);
}

bool chillAST_CompoundStmt::findLoopIndexesToReplace(chillAST_SymbolTable *symtab, bool forcesync) {

  // see how many elements we currently have
  int sofar = children.size();

  // make big enough to add a sync after each statement. wasteful. TODO
  // this prevents inserts happening at the forstmt::addSync() from causing a
  // reallocation, which screwsup the loop below here
  children.reserve(2 * sofar);

  bool force = false;
  for (int i = 0; i < children.size(); i++) {  // children.size() to see it gain each time
    if (children.size() > sofar)
      sofar = children.size();

    bool thisforces = children[i]->findLoopIndexesToReplace(symtab, force);
    force = force || thisforces; // once set, always
  }
  return false;

/*
  vector<chillAST_Node*> childrencopy;
  for (int i=0; i<children.size(); i++) childrencopy.push_back( children[i] );
  bool force = false;

  char *origtypes[64];
  int origsize = children.size();
  for (int i=0; i<children.size(); i++) {
    fprintf(stderr, "ORIGINAL compound child %d of type %s\n", i, children[i]->getTypeString() );
    origtypes[i] = strdup( children[i]->getTypeString() );
    fprintf(stderr, "ORIGINAL compound child %d of type %s\n", i, children[i]->getTypeString() );
  }

  for (int i=0; i<childrencopy.size(); i++) {
    fprintf(stderr, "compound child %d of type %s force %d\n", i, childrencopy[i]->getTypeString(), force );
    force = force || childrencopy[i]->findLoopIndexesToReplace( symtab, force ); // once set, always
  }

  fprintf(stderr, "\n");
  for (int i=0; i<origsize; i++) {
    fprintf(stderr, "BEFORE compound child %d/%d of type %s\n",  i, origsize, origtypes[i]);
  }
  for (int i=0; i<children.size(); i++) {
    fprintf(stderr, "AFTER  compound child %d/%d of type %s\n", i, children.size(), children[i]->getTypeString() );
  }

  return false;
*/
}

chillAST_ParenExpr::chillAST_ParenExpr() {
  children.push_back(NULL);
}

chillAST_ParenExpr::chillAST_ParenExpr(chillAST_Node *sub):chillAST_ParenExpr() {
  setSubExpr(sub);
}

chillAST_Node *chillAST_ParenExpr::clone() {
  chillAST_ParenExpr *PE = new chillAST_ParenExpr(getSubExpr()->clone());
  PE->setParent(getParent());
  PE->isFromSourceFile = isFromSourceFile;
  if (filename) PE->filename = strdup(filename);
  return PE;
}

chillAST_Sizeof::chillAST_Sizeof(char *athing) {
  thing = strdup(athing); // memory leak
}

chillAST_Node *chillAST_Sizeof::clone() {
  chillAST_Sizeof *SO = new chillAST_Sizeof(thing);
  SO->setParent(getParent());
  SO->isFromSourceFile = isFromSourceFile;
  if (filename) SO->filename = strdup(filename);
  return SO;
}

chillAST_IfStmt::chillAST_IfStmt() {
  children.push_back(NULL);
  children.push_back(NULL);
  children.push_back(NULL);
}

chillAST_IfStmt::chillAST_IfStmt(chillAST_Node *c, chillAST_Node *t, chillAST_Node *e) : chillAST_IfStmt() {
  setCond(c);
  setThen(t);
  setElse(e);
}

void chillAST_IfStmt::gatherArrayRefs(std::vector<chillAST_ArraySubscriptExpr *> &refs, bool writtento) {
  chillAST_Node::gatherArrayRefs(refs, false);
}

void chillAST_IfStmt::gatherScalarRefs(std::vector<chillAST_DeclRefExpr *> &refs, bool writtento) {
  chillAST_Node::gatherScalarRefs(refs, false);
}

chillAST_Node *chillAST_IfStmt::clone() {
  chillAST_Node *c, *t, *e;
  c = t = e = NULL;
  if (getCond()) c = getCond()->clone(); // has to be one, right?
  if (getThen()) t = getThen()->clone();
  if (getElse()) e = getElse()->clone();

  chillAST_IfStmt *IS = new chillAST_IfStmt(c, t, e);
  IS->setParent(getParent());
  IS->isFromSourceFile = isFromSourceFile;
  if (filename) IS->filename = strdup(filename);
  return IS;
}

bool chillAST_IfStmt::findLoopIndexesToReplace(chillAST_SymbolTable *symtab, bool forcesync) {
  getThen()->findLoopIndexesToReplace(symtab);
  getElse()->findLoopIndexesToReplace(symtab);
  return false; // ??
}

chillAST_Node *minmaxTernary(const char * op, chillAST_Node *left, chillAST_Node *right) {

  chillAST_Node *lp1 = left->clone();
  chillAST_Node *rp1 = right->clone();
  chillAST_BinaryOperator *cond = new chillAST_BinaryOperator(lp1, op, rp1);
  chillAST_Node *lp2 = left->clone();
  chillAST_Node *rp2 = right->clone();

  return new chillAST_TernaryOperator("?", cond, lp2, rp2);
}

// look for function declaration with a given name, in the tree with root "node"
void findFunctionDeclRecursive(chillAST_Node *node, const char *procname, vector<chillAST_FunctionDecl *> &funcs) {
  if (node->isFunctionDecl()) {
    char *name = ((chillAST_FunctionDecl *) node)->functionName; // compare name with desired name
    if (!strcmp(name, procname)) {
      funcs.push_back((chillAST_FunctionDecl *) node);  // this is it
      return;
    }
  }
  // this is where the children can be used effectively.
  // we don't really care what kind of node we're at. We just check the node itself
  // and then its children is needed.
  int numc = node->children.size();
  for (int i = 0; i < numc; i++) {
    if (node->isSourceFile())
      if (node->children[i]->isFunctionDecl())
        chillAST_FunctionDecl *fd = (chillAST_FunctionDecl *) node->children[i];
    findFunctionDeclRecursive(node->children[i], procname, funcs);
  }
  return;
}

chillAST_SymbolTable *addSymbolToTable(chillAST_SymbolTable *st, chillAST_VarDecl *vd) // definition
{
  chillAST_SymbolTable *s = st;
  if (!s) s = new chillAST_SymbolTable;
  int tablesize = s->size();
  for (int i = 0; i < tablesize; i++) {
    if ((*s)[i] == vd) {
      return s; // already there
    }
  }
  for (int i = 0; i < tablesize; i++) {
    if (!strcmp((*s)[i]->varname, vd->varname)) {
      return s; // already there
    }
  }
  s->push_back(vd); // add it
  return s;
}

chillAST_TypedefTable *addTypedefToTable(chillAST_TypedefTable *tdt, chillAST_TypedefDecl *td) {

  chillAST_TypedefTable *t = tdt;
  if (!t) t = new chillAST_TypedefTable;

  int tablesize = t->size();

  for (int i = 0; i < tablesize; i++) {
    if ((*t)[i] == td) return t; // already there
  }
  t->push_back(td); // add it
  return t;
}


chillAST_NoOp::chillAST_NoOp() {
}; // so we have SOMETHING for NoOp in the cc file ???


chillAST_Preprocessing::chillAST_Preprocessing() {
  position = CHILLAST_PREPROCESSING_POSITIONUNKNOWN;
  pptype = CHILLAST_PREPROCESSING_TYPEUNKNOWN;
  blurb = strdup("");  // never use null. ignore the leak ??
}


chillAST_Preprocessing::chillAST_Preprocessing(CHILLAST_PREPROCESSING_POSITION pos,
                                               CHILLAST_PREPROCESSING_TYPE t,
                                               char *text) {
  position = pos;
  pptype = t;
  blurb = strdup(text);
}

