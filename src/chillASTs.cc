


#include <chilldebug.h>
#include <stack>
#include "chillAST.h"
#include "printer/dump.h"
#include "printer/cfamily.h"

using namespace std;

int chillAST_Node::chill_scalar_counter = 0;
int chillAST_Node::chill_array_counter = 1;


const char *ChillAST_Node_Names[] = {
    "Unknown AST node type",
    "SourceFile",
    "TypedefDecl",
    "VarDecl",
    //  "ParmVarDecl",  not used any more
    "FunctionDecl",
    "RecordDecl",
    "MacroDefinition",
    "CompoundStmt",
    "ForStmt",
    "TernaryOperator",
    "BinaryOperator",
    "UnaryOperator",
    "ArraySubscriptExpr",
    "MemberExpr",
    "DeclRefExpr",
    "IntegerLiteral",
    "FloatingLiteral",
    "ImplicitCastExpr", // not sure we need this
    "ReturnStmt",
    "CallExpr",
    "DeclStmt",
    "ParenExpr",
    "CStyleCastExpr",
    "CStyleAddressOf",
    "IfStmt",
    "SizeOf",
    "Malloc",
    "Free",
    "NoOp",
// CUDA specific
    "CudaMalloc",
    "CudaFree",
    "CudaMemcpy",
    "CudaKernelCall",
    "CudaSyncthreads",
    "fake1",
    "fake2",
    "fake3"
};

bool streq(const char *a, const char *b) {
  return !strcmp(a, b);
}

//! Parse to the most basic type
char *parseUnderlyingType(const char *sometype) {
  int len = strlen(sometype);
  CHILL_DEBUG_PRINT("parseUnderlyingType( %s )\n", sometype);
  char *underlying = strdup(sometype);
  char *p;

  p = &underlying[len - 1];

  while (p > underlying)
    if (*p == ' ' || *p == '*')
      --p;
    else if (*p == ']')
      while (*p != '[') --p;
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
    fprintf(stderr, "false alarm. %s is a variable, but doesn't have subparts\n", varname);
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

//! remove UL from numbers, MODIFIES the argument!
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


//! remove __restrict__ , MODIFIES the argument!
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

  FILE *fp = fopen(fn, "w");
  if (!fp) {
    fprintf(stderr, "can't open file '%s' for writing\n", fn);
    exit(-1);
  }
  fprintf(fp, "\n\n");
  print(0, fp);

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
  //fprintf(stderr, "chillAST_TypedefDecl::findSubpart( %s )\n", name);
  //fprintf(stderr, "typedef %s  %s\n", structname, newtype);

  if (rd) { // we have a record decl look there
    chillAST_VarDecl *sub = rd->findSubpart(name);
    //fprintf(stderr, "rd found subpart %p\n", sub);
    return sub;
  }

  // can this ever happen now ???
  int nsub = subparts.size();
  //fprintf(stderr, "%d subparts\n", nsub);
  for (int i = 0; i < nsub; i++) {
    if (!strcmp(name, subparts[i]->varname)) return subparts[i];
  }
  //fprintf(stderr, "subpart not found\n");


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
  //fprintf(stderr, "chillAST_RecordDecl::findSubpart( %s )\n", nam);
  int nsub = subparts.size();
  //fprintf(stderr, "%d subparts\n", nsub);
  for (int i = 0; i < nsub; i++) {
    //fprintf(stderr, "comparing to '%s' to '%s'\n", nam, subparts[i]->varname);
    if (!strcmp(nam, subparts[i]->varname)) return subparts[i];
  }
  fprintf(stderr, "chillAST_RecordDecl::findSubpart() couldn't find member NAMED %s in ", nam);
  print();

  return NULL;
}


chillAST_VarDecl *chillAST_RecordDecl::findSubpartByType(const char *typ) {
  //fprintf(stderr, "chillAST_RecordDecl::findSubpart( %s )\n", nam);
  int nsub = subparts.size();
  //fprintf(stderr, "%d subparts\n", nsub);
  for (int i = 0; i < nsub; i++) {
    //fprintf(stderr, "comparing '%s' to '%s'\n", typ, subparts[i]->vartype);
    if (!strcmp(typ, subparts[i]->vartype)) return subparts[i];
  }
  //fprintf(stderr, "chillAST_RecordDecl::findSubpart() couldn't find member of TYPE %s in ", typ); print(); printf("\n\n"); fflush(stdout);

  return NULL;
}

chillAST_SymbolTable *chillAST_RecordDecl::addVariableToSymbolTable(chillAST_VarDecl *vd) {
  // for now, just bail. or do we want the struct to have an actual symbol table?
  //fprintf(stderr, "chillAST_RecordDecl::addVariableToSymbolTable() ignoring struct member %s vardecl\n", vd->varname);
  return NULL; // damn, I hope nothing uses this!
}

void chillAST_RecordDecl::printStructure(int indent, FILE *fp) {
  //fprintf(stderr, "chillAST_RecordDecl::printStructure()\n");
  chillindent(indent, fp);
  if (isStruct) {
    fprintf(fp, "struct { ", name);
    for (int i = 0; i < subparts.size(); i++) {
      subparts[i]->print(0, fp); // ?? TODO indent level
      fprintf(fp, "; ");
    }
    fprintf(fp, "} ");
  } else {
    fprintf(fp, "/* UNKNOWN RECORDDECL printStructure() */  ");
    exit(-1);
  }
  fflush(fp);
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
  body = new chillAST_CompoundStmt();
  returnType = strdup(rt);
  this->setFunctionCPU();
  uniquePtr = unique; // a quick way to check equivalence. DO NOT ACCESS THROUGH THIS
};

void chillAST_FunctionDecl::setBody(chillAST_Node *bod) {
  if (bod->isCompoundStmt()) body = (chillAST_CompoundStmt *) bod;
  else {
    CHILL_ERROR("Should always be a compound statements");
    body = new chillAST_CompoundStmt();
    body->addChild(bod);
  }
  bod->setParent(this);  // well, ...
}

void chillAST_FunctionDecl::insertChild(int i, chillAST_Node *node) {
  fprintf(stderr, "chillAST_FunctionDecl::insertChild()  ");
  node->print(0, stderr);
  fprintf(stderr, "\n\n");
  body->insertChild(i, node);

  if (node->isVarDecl()) {
    chillAST_VarDecl *vd = ((chillAST_VarDecl *) node);
    fprintf(stderr, "functiondecl %s inserting a VarDecl named %s\n", functionName, vd->varname);
    chillAST_SymbolTable *st = getSymbolTable();
    if (!st)
      fprintf(stderr, "symbol table is NULL!\n");
    else
      fprintf(stderr, "%d entries in the symbol table\n", st->size());
    fprintf(stderr, "\n\n");
  }
}

void chillAST_FunctionDecl::addChild(chillAST_Node *node) {
  CHILL_DEBUG_BEGIN
    node->print(0, stderr);
    fprintf(stderr, "\n\n");
  CHILL_DEBUG_END
  if (node->isVarDecl()) {
    chillAST_VarDecl *vd = ((chillAST_VarDecl *) node);
    CHILL_DEBUG_PRINT("functiondecl %s adding a VarDecl named %s\n", functionName, vd->varname);
  }

  body->addChild(node);
  node->setParent(this); // this, or body??
}

void chillAST_FunctionDecl::gatherVarDecls(vector<chillAST_VarDecl *> &decls) {
  //fprintf(stderr, "chillAST_FunctionDecl::gatherVarDecls()\n");
  //if (0 < children.size()) fprintf(stderr, "functiondecl has %d children\n", children.size());
  //fprintf(stderr, "functiondecl has %d parameters\n", numParameters());
  for (int i = 0; i < getParameters()->size(); i++) (*getParameters())[i]->gatherVarDecls(decls);
  //fprintf(stderr, "after parms, %d decls\n", decls.size());
  for (int i = 0; i < children.size(); i++) children[i]->gatherVarDecls(decls);
  //fprintf(stderr, "after children, %d decls\n", decls.size());
  body->gatherVarDecls(decls);  // todo, figure out if functiondecl has actual children
  //fprintf(stderr, "after body, %d decls\n", decls.size());
  //for (int d=0; d<decls.size(); d++) {
  //  decls[d]->print(0,stderr); fprintf(stderr, "\n");
  //}
}


void chillAST_FunctionDecl::gatherScalarVarDecls(vector<chillAST_VarDecl *> &decls) {
  //if (0 < children.size()) fprintf(stderr, "functiondecl has %d children\n", children.size());

  for (int i = 0; i < getParameters()->size(); i++) (*getSymbolTable())[i]->gatherScalarVarDecls(decls);
  for (int i = 0; i < children.size(); i++) children[i]->gatherScalarVarDecls(decls);
  body->gatherScalarVarDecls(decls);  // todo, figure out if functiondecl has actual children
}


void chillAST_FunctionDecl::gatherArrayVarDecls(vector<chillAST_VarDecl *> &decls) {
  //if (0 < children.size()) fprintf(stderr, "functiondecl has %d children\n", children.size());

  for (int i = 0; i < getParameters()->size(); i++) (*getSymbolTable())[i]->gatherArrayVarDecls(decls);
  for (int i = 0; i < children.size(); i++) children[i]->gatherArrayVarDecls(decls);
  body->gatherArrayVarDecls(decls);  // todo, figure out if functiondecl has actual children
}


chillAST_VarDecl *chillAST_FunctionDecl::findArrayDecl(const char *name) {
  //fprintf(stderr, "chillAST_FunctionDecl::findArrayDecl( %s )\n", name );
  chillAST_VarDecl *p = getVariableDeclaration(name);
  //if (p) fprintf(stderr, "function %s has parameter named %s\n", functionName, name );
  if (p && p->isArray()) return p;

  chillAST_VarDecl *v = body->getVariableDeclaration(name);
  //if (v) fprintf(stderr, "function %s has symbol table variable named %s\n", functionName, name );
  if (v && v->isArray()) return v;

  // declared variables that may not be in symbol table but probably should be
  vector<chillAST_VarDecl *> decls;
  gatherArrayVarDecls(decls);
  for (int i = 0; i < decls.size(); i++) {
    chillAST_VarDecl *vd = decls[i];
    if (0 == strcmp(vd->varname, name) && vd->isArray()) return vd;
  }

  //fprintf(stderr, "can't find array named %s in function %s \n", name, functionName);
  return NULL;
}


void chillAST_FunctionDecl::gatherVarUsage(vector<chillAST_VarDecl *> &decls) {
  for (int i = 0; i < children.size(); i++) children[i]->gatherVarUsage(decls);
  body->gatherVarUsage(decls);  // todo, figure out if functiondecl has actual children
}


void chillAST_FunctionDecl::gatherDeclRefExprs(vector<chillAST_DeclRefExpr *> &refs) {
  for (int i = 0; i < children.size(); i++) children[i]->gatherDeclRefExprs(refs);
  body->gatherDeclRefExprs(refs);  // todo, figure out if functiondecl has actual children
}


void chillAST_FunctionDecl::cleanUpVarDecls() {
  //fprintf(stderr, "\ncleanUpVarDecls() for function %s\n", functionName);
  vector<chillAST_VarDecl *> used;
  vector<chillAST_VarDecl *> defined;
  vector<chillAST_VarDecl *> deletethese;

  gatherVarUsage(used);
  gatherVarDecls(defined);

  //fprintf(stderr, "\nvars used: \n");
  //for ( int i=0; i< used.size(); i++) {
  //used[i]->print(0, stderr);  fprintf(stderr, "\n");
  //}
  //fprintf(stderr, "\n");
  //fprintf(stderr, "\nvars defined: \n");
  //for ( int i=0; i< defined.size(); i++) {
  //  defined[i]->print(0, stderr);  fprintf(stderr, "\n");
  //}
  //fprintf(stderr, "\n");

  for (int j = 0; j < defined.size(); j++) {
    //fprintf(stderr, "j %d  defined %s\n", j, defined[j]->varname);
    bool definedandused = false;
    for (int i = 0; i < used.size(); i++) {
      if (used[i] == defined[j]) {
        //fprintf(stderr, "i %d used %s\n", i, used[i]->varname);
        //fprintf(stderr, "\n");
        definedandused = true;
        break;
      }
    }

    if (!definedandused) {
      if (defined[j]->isParmVarDecl()) {
        //fprintf(stderr, "we'd remove %s except that it's a parameter. Maybe someday\n", defined[j]->varname);
      } else {
        //fprintf(stderr, "we can probably remove the definition of %s\n", defined[j]->varname);
        deletethese.push_back(defined[j]);
      }
    }
  }


  //fprintf(stderr, "deleting %d vardecls\n", deletethese.size());
  for (int i = 0; i < deletethese.size(); i++) {
    //fprintf(stderr, "deleting varDecl %s\n",  deletethese[i]->varname);
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
  if (body) body->findLoopIndexesToReplace(symtab, false);
  return false;
}


chillAST_Node *chillAST_FunctionDecl::constantFold() {
  if (body) body = (chillAST_CompoundStmt *) body->constantFold();
  return this;
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
  fprintf(stderr, "%s chillAST_MacroDefinition::setBody( 0x%x )\n", macroName, bod);
  body = bod;
  fprintf(stderr, "body is:\n");
  body->print(0, stderr);
  fprintf(stderr, "\n\n");
  bod->setParent(this);  // well, ...
}

void chillAST_MacroDefinition::insertChild(int i, chillAST_Node *node) {
  body->insertChild(i, node);
}

void chillAST_MacroDefinition::addChild(chillAST_Node *node) {
  body->addChild(node);
  node->setParent(this); // this, or body??
}

chillAST_ForStmt::chillAST_ForStmt() {
  init = cond = incr = NULL;
  body = new chillAST_CompoundStmt();

  conditionoperator = IR_COND_UNKNOWN;
  symbolTable = new chillAST_SymbolTable();
}


chillAST_ForStmt::chillAST_ForStmt(chillAST_Node *ini, chillAST_Node *con, chillAST_Node *inc, chillAST_Node *bod) {
  init = ini;
  cond = con;
  incr = inc;
  body = bod;
  init->setParent(this);
  cond->setParent(this);
  incr->setParent(this);
  symbolTable = new chillAST_SymbolTable();

  //fprintf(stderr, "chillAST_ForStmt::chillAST_ForStmt() bod %p\n", bod);

  if (body) body->setParent(this);  // not sure this should be legal

  if (!cond->isBinaryOperator()) {
    CHILL_ERROR("ForStmt conditional is of type %s. Expecting a BinaryOperator\n", cond->getTypeString());
    exit(-1);
  }
  chillAST_BinaryOperator *bo = (chillAST_BinaryOperator *) cond;
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


bool chillAST_ForStmt::lowerBound(int &l) { // l is an output (passed as reference)

  // above, cond must be a binaryoperator ... ???
  if (conditionoperator == IR_COND_LT ||
      conditionoperator == IR_COND_LE) {

    // lower bound is rhs of init
    if (!init->isBinaryOperator()) {
      fprintf(stderr, "chillAST_ForStmt::lowerBound() init is not a chillAST_BinaryOperator\n");
      exit(-1);
    }

    chillAST_BinaryOperator *bo = (chillAST_BinaryOperator *) init;
    if (!init->isAssignmentOp()) {
      fprintf(stderr, "chillAST_ForStmt::lowerBound() init is not an assignment chillAST_BinaryOperator\n");
      exit(-1);
    }

    //fprintf(stderr, "rhs "); bo->rhs->print(0,stderr);  fprintf(stderr, "   ");
    l = bo->rhs->evalAsInt(); // float could be legal I suppose
    //fprintf(stderr, "   %d\n", l);
    return true;
  } else if (conditionoperator == IR_COND_GT ||
             conditionoperator == IR_COND_GE) {  // decrementing
    // lower bound is rhs of cond (not init)
    chillAST_BinaryOperator *bo = (chillAST_BinaryOperator *) cond;
    l = bo->rhs->evalAsInt(); // float could be legal I suppose
    return true;
  }

  // some case we don't handle ??
  fprintf(stderr, "chillAST_ForStmt::lowerBound() can't find lower bound of ");
  print(0, stderr);
  fprintf(stderr, "\n\n");
  return false;      // or exit ???
}


bool chillAST_ForStmt::upperBound(int &u) { // u is an output (passed as reference)

  // above, cond must be a binaryoperator ... ???
  if (conditionoperator == IR_COND_GT ||
      conditionoperator == IR_COND_GE) {  // decrementing

    // upper bound is rhs of init
    if (!init->isBinaryOperator()) {
      fprintf(stderr, "chillAST_ForStmt::upperBound() init is not a chillAST_BinaryOperator\n");
      exit(-1);
    }

    chillAST_BinaryOperator *bo = (chillAST_BinaryOperator *) init;
    if (!init->isAssignmentOp()) {
      fprintf(stderr, "chillAST_ForStmt::upperBound() init is not an assignment chillAST_BinaryOperator\n");
      exit(-1);
    }

    u = bo->rhs->evalAsInt(); // float could be legal I suppose
    return true;
  } else if (conditionoperator == IR_COND_LT ||
             conditionoperator == IR_COND_LE) {
    //fprintf(stderr, "upper bound is rhs of cond   ");
    // upper bound is rhs of cond (not init)
    chillAST_BinaryOperator *bo = (chillAST_BinaryOperator *) cond;
    //bo->rhs->print(0,stderr);
    u = bo->rhs->evalAsInt(); // float could be legal I suppose

    if (conditionoperator == IR_COND_LT) u -= 1;

    //fprintf(stderr, "    %d\n", u);
    return true;
  }

  // some case we don't handle ??
  fprintf(stderr, "chillAST_ForStmt::upperBound() can't find upper bound of ");
  print(0, stderr);
  fprintf(stderr, "\n\n");
  return false;      // or exit ???
}


void chillAST_ForStmt::printControl(int in, FILE *fp) {
  chillindent(in, fp);
  fprintf(fp, "for (");
  init->print(0, fp);
  fprintf(fp, "; ");
  cond->print(0, fp);
  fprintf(fp, "; ");
  incr->print(0, fp);
  fprintf(fp, ")");
  fflush(fp);
}

chillAST_Node *chillAST_ForStmt::constantFold() {
  init = init->constantFold();
  cond = cond->constantFold();
  incr = incr->constantFold();
  body = body->constantFold();
  return this;
}

chillAST_Node *chillAST_ForStmt::clone() {
  chillAST_ForStmt *fs = new chillAST_ForStmt(init->clone(), cond->clone(), incr->clone(), body->clone());
  fs->isFromSourceFile = isFromSourceFile;
  if (filename) fs->filename = strdup(filename);
  fs->setParent(getParent());
  return fs;
}

void chillAST_ForStmt::gatherVarDecls(vector<chillAST_VarDecl *> &decls) {
  //fprintf(stderr, "chillAST_ForStmt::gatherVarDecls()\n");
  //fprintf(stderr, "chillAST_ForStmt::gatherVarDecls()  before %d\n", decls.size());
  // TODO clear a loop_var_decls variable and then walk it ?
  init->gatherVarDecls(decls);
  cond->gatherVarDecls(decls);
  incr->gatherVarDecls(decls);
  body->gatherVarDecls(decls);
  //fprintf(stderr, "after %d\n", decls.size());
}

void chillAST_ForStmt::gatherScalarVarDecls(vector<chillAST_VarDecl *> &decls) {
  //fprintf(stderr, "chillAST_ForStmt::gatherScalarVarDecls()  before %d\n", decls.size());
  init->gatherScalarVarDecls(decls);
  cond->gatherScalarVarDecls(decls);
  incr->gatherScalarVarDecls(decls);
  body->gatherScalarVarDecls(decls);
}

void chillAST_ForStmt::gatherArrayVarDecls(vector<chillAST_VarDecl *> &decls) {
  //fprintf(stderr, "chillAST_ForStmt::gatherArrayVarDecls()  before %d\n", decls.size());
  init->gatherArrayVarDecls(decls);
  cond->gatherArrayVarDecls(decls);
  incr->gatherArrayVarDecls(decls);
  body->gatherArrayVarDecls(decls);
}

void chillAST_ForStmt::gatherArrayRefs(std::vector<chillAST_ArraySubscriptExpr *> &refs, bool writtento) {
  init->gatherArrayRefs(refs, 0);  // 0 ??
  cond->gatherArrayRefs(refs, 0);  // 0 ??
  incr->gatherArrayRefs(refs, 0);  // 0 ??
  body->gatherArrayRefs(refs, 0);  // 0 ??
}

void chillAST_ForStmt::gatherScalarRefs(std::vector<chillAST_DeclRefExpr *> &refs, bool writtento) {
  init->gatherScalarRefs(refs, 0);  // 0 ??
  cond->gatherScalarRefs(refs, 0);  // 0 ??
  incr->gatherScalarRefs(refs, 0);  // 0 ??
  body->gatherScalarRefs(refs, 0);  // 0 ??
}

void chillAST_ForStmt::gatherDeclRefExprs(vector<chillAST_DeclRefExpr *> &refs) {
  init->gatherDeclRefExprs(refs);
  cond->gatherDeclRefExprs(refs);
  incr->gatherDeclRefExprs(refs);
  body->gatherDeclRefExprs(refs);
}


void chillAST_ForStmt::gatherVarUsage(vector<chillAST_VarDecl *> &decls) {
  init->gatherVarUsage(decls);
  cond->gatherVarUsage(decls);
  incr->gatherVarUsage(decls);
  body->gatherVarUsage(decls);
}

void chillAST_ForStmt::gatherStatements(std::vector<chillAST_Node *> &statements) {

  // for completeness, should do all 4. Maybe someday
  //init->gatherStatements( statements );
  //cond->gatherStatements( statements );
  //incr->gatherStatements( statements );
  body->gatherStatements(statements);
}


void chillAST_ForStmt::addSyncs() {
  //fprintf(stderr, "\nchillAST_ForStmt::addSyncs()  ");
  //fprintf(stderr, "for (");
  //init->print(0, stderr);
  //fprintf(stderr, "; ");
  //cond->print(0, stderr);
  //fprintf(stderr, "; ");
  //incr->print(0, stderr);
  //fprintf(stderr, ")\n");

  if (!parent) {
    fprintf(stderr, "uhoh, chillAST_ForStmt::addSyncs() ForStmt has no parent!\n");
    fprintf(stderr, "for (");
    init->print(0, stderr);
    fprintf(stderr, "; ");
    cond->print(0, stderr);
    fprintf(stderr, "; ");
    incr->print(0, stderr);
    fprintf(stderr, ")\n");

    return; // exit?
  }

  if (parent->isCompoundStmt()) {
    //fprintf(stderr, "ForStmt parent is CompoundStmt 0x%x\n", parent);
    vector<chillAST_Node *> *chillin = parent->getChildren();
    int numc = chillin->size();
    //fprintf(stderr, "ForStmt parent is CompoundStmt 0x%x with %d children\n", parent, numc);
    for (int i = 0; i < numc; i++) {
      if (this == parent->getChild(i)) {
        //fprintf(stderr, "forstmt 0x%x is child %d of %d\n", this, i, numc);
        chillAST_CudaSyncthreads *ST = new chillAST_CudaSyncthreads();
        parent->insertChild(i + 1, ST);  // corrupts something ...
        //fprintf(stderr, "Create a call to __syncthreads() 2\n");
        //parent->addChild(ST);  // wrong, but safer   still kills
      }
    }

    chillin = parent->getChildren();
    int nowc = chillin->size();
    //fprintf(stderr, "old, new number of children = %d %d\n", numc, nowc);

  } else {
    fprintf(stderr, "chillAST_ForStmt::addSyncs() unhandled parent type %s\n", parent->getTypeString());
    exit(-1);
  }

  //fprintf(stderr, "leaving addSyncs()\n");
}


void chillAST_ForStmt::removeSyncComment() {
  //fprintf(stderr, "chillAST_ForStmt::removeSyncComment()\n");
  if (metacomment && strstr(metacomment, "~cuda~") && strstr(metacomment, "preferredIdx: ")) {
    char *ptr = strlen("preferredIdx: ") + strstr(metacomment, "preferredIdx: ");
    *ptr = '\0';
  }
}


bool chillAST_ForStmt::findLoopIndexesToReplace(chillAST_SymbolTable *symtab, bool forcesync) {
  CHILL_DEBUG_PRINT("\nchillAST_ForStmt::findLoopIndexesToReplace( force = %d )\n", forcesync);
  //if (metacomment) fprintf(stderr, "metacomment '%s'\n", metacomment);

  bool force = forcesync;
  bool didasync = false;
  if (forcesync) {
    //fprintf(stderr, "calling addSyncs() because PREVIOUS ForStmt in a block had preferredIdx\n");
    addSyncs();
    didasync = true;
  }

  //fprintf(stderr, "chillAST_ForStmt::findLoopIndexesToReplace()\n");
  if (metacomment && strstr(metacomment, "~cuda~") && strstr(metacomment, "preferredIdx: ")) {
    //fprintf(stderr, "metacomment '%s'\n", metacomment);

    char *copy = strdup(metacomment);
    char *ptr = strstr(copy, "preferredIdx: ");
    char *vname = ptr + strlen("preferredIdx: ");
    char *space = strstr(vname, " "); // TODO index()
    if (space) {
      //fprintf(stderr, "vname = '%s'\n", vname);
      force = true;
    }

    if ((!didasync) && force) {
      //fprintf(stderr, "calling addSyncs() because ForStmt metacomment had preferredIdx '%s'\n", vname);
      addSyncs();
      removeSyncComment();
      didasync = true;
    }

    if (space) *space = '\0'; // if this is multiple words, grab the first one
    //fprintf(stderr, "vname = '%s'\n", vname);

    //fprintf(stderr, "\nfor (");
    //init->print(0, stderr);
    //fprintf(stderr, "; ");
    //cond->print(0, stderr);
    //fprintf(stderr, "; ");
    //incr->print(0, stderr);
    //fprintf(stderr, ")    %s\n", metacomment );
    //fprintf(stderr, "prefer '%s'\n", vname );

    vector<chillAST_VarDecl *> decls;
    init->gatherVarLHSUsage(decls);
    //cond->gatherVarUsage( decls );
    //incr->gatherVarUsage( decls );
    //fprintf(stderr, "forstmt has %d vardecls in init, cond, inc\n", decls.size());

    if (1 != decls.size()) {
      fprintf(stderr, "uhoh, preferred index in for statement, but multiple variables used\n");
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
    fprintf(stderr, "RIGHT NOW, change all the references that this loop wants swapped out \n");

    chillAST_VarDecl *newguy = findVariableDecleration(vname); // recursive
    if (!newguy) {
      fprintf(stderr, "there was no variable named %s anywhere I could find\n", vname);
    }

    // wrong - this only looks at variables defined in the forstmt, not
    // in parents of the forstmt
    //int numsym = symtab->size();
    //fprintf(stderr, "%d symbols\n", numsym);
    //for (int i=0; i<numsym; i++) {
    //  fprintf(stderr, "sym %d is '%s'\n", i, (*symtab)[i]->varname);
    //  if (!strcmp(vname,  (*symtab)[i]->varname)) {
    //    newguy = (*symtab)[i];
    // }
    //}
    if (!newguy) {
      fprintf(stderr, "chillAST_ForStmt::findLoopIndexesToReplace() there is no defined variable %s\n", vname);

      // make one ??  seems like this should never happen
      newguy = new chillAST_VarDecl(olddecl->vartype, vname, ""/*?*/, NULL);
      // insert actual declaration in code location?   how?

      // find parent of the ForStmt?
      // find parent^n of the ForStmt that is not a Forstmt?
      // find parent^n of the Forstmt that is a FunctionDecl?
      chillAST_Node *contain = findContainingNonLoop();
      if (contain == NULL) {
        fprintf(stderr, "nothing but loops all the way up?\n");
        exit(0);
      }
      fprintf(stderr, "containing non-loop is a %s\n", contain->getTypeString());

      contain->print(0, stderr);
      contain->insertChild(0, newguy); // TODO ugly order
      contain->addVariableToScope(newguy); // adds to first enclosing symbolTable

      if (!symbolTableFindName(contain->getSymbolTable(), vname)) {
        fprintf(stderr, "container doesn't have a var names %s afterwards???\n", vname);
        exit(-1);
      }
    }


    // swap out old for new in init, cond, incr, body
    if (newguy) {
      fprintf(stderr, "\nwill replace %s with %s in init, cond, incr\n", olddecl->varname, newguy->varname);
      fprintf(stderr, "was: for (");
      init->print(0, stderr);
      fprintf(stderr, "; ");
      cond->print(0, stderr);
      fprintf(stderr, "; ");
      incr->print(0, stderr);
      fprintf(stderr, ")\n");


      init->replaceVarDecls(olddecl, newguy);
      cond->replaceVarDecls(olddecl, newguy);
      incr->replaceVarDecls(olddecl, newguy);

      fprintf(stderr, " is: for (");
      init->print(0, stderr);
      fprintf(stderr, "; ");
      cond->print(0, stderr);
      fprintf(stderr, "; ");
      incr->print(0, stderr);
      fprintf(stderr, ")\n\n");

      fprintf(stderr, "recursing to ForStmt body of type %s\n", body->getTypeString());
      body->replaceVarDecls(olddecl, newguy);

      fprintf(stderr, "\nafter recursing to body, this loop is   (there should be no %s)\n", olddecl->varname);
      print(0, stderr);
      fprintf(stderr, "\n");

    }

    //if (!space) // there was only one preferred
    //fprintf(stderr, "removing metacomment\n");
    metacomment = NULL; // memleak

  }

  // check for more loops.  We may have already swapped variables out in body (right above here)
  body->findLoopIndexesToReplace(symtab, false);

  return force;
}

void chillAST_ForStmt::replaceChild(chillAST_Node *old, chillAST_Node *newchild) {
  //fprintf(stderr, "chillAST_ForStmt::replaceChild()  REALLY CALLING BODY->ReplaceCHILD\n");
  body->replaceChild(old, newchild);
}


void chillAST_ForStmt::replaceVarDecls(chillAST_VarDecl *olddecl, chillAST_VarDecl *newdecl) {
  // logic problem  if my loop var is olddecl!

  //fprintf(stderr, "chillAST_ForStmt::replaceVarDecls( old %s,  new %s )\n", olddecl->varname, newdecl->varname);

  // this is called for inner loops!
  init->replaceVarDecls(olddecl, newdecl);
  cond->replaceVarDecls(olddecl, newdecl);
  incr->replaceVarDecls(olddecl, newdecl);
  body->replaceVarDecls(olddecl, newdecl);
}


void chillAST_ForStmt::gatherLoopIndeces(std::vector<chillAST_VarDecl *> &indeces) {
  //fprintf(stderr, "chillAST_ForStmt::gatherLoopIndeces()\nloop is:\n"); print(0,stderr);

  vector<chillAST_VarDecl *> decls;
  init->gatherVarLHSUsage(decls);
  cond->gatherVarLHSUsage(decls);
  incr->gatherVarLHSUsage(decls);
  // note: NOT GOING INTO BODY OF THE LOOP

  int numdecls = decls.size();
  //fprintf(stderr, "gatherLoopIndeces(), %d lhs vardecls for this ForStmt\n", numdecls);

  for (int i = 0; i < decls.size(); i++) {
    //fprintf(stderr, "%s %p\n", decls[i]->varname, decls[i] );
    indeces.push_back(decls[i]);
  }

  // Don't forget to keep heading upwards!
  if (parent) {
    //fprintf(stderr, "loop %p has parent of type %s\n", this, parent->getTypeString());
    parent->gatherLoopIndeces(indeces);
  }
  //else fprintf(stderr, "this loop has no parent???\n");

}


void chillAST_ForStmt::gatherLoopVars(std::vector<std::string> &loopvars) {
  //fprintf(stderr, "gathering loop vars for loop   for (");
  //init->print(0, stderr);
  //fprintf(stderr, "; ");
  //cond->print(0, stderr);
  //fprintf(stderr, "; ");
  //incr->print(0, stderr);
  //fprintf(stderr, ")\n" );

  //init->dump(0, stderr);


  vector<chillAST_VarDecl *> decls;
  init->gatherVarLHSUsage(decls);
  cond->gatherVarLHSUsage(decls);
  incr->gatherVarLHSUsage(decls);
  // note: NOT GOING INTO BODY OF THE LOOP

  for (int i = 0; i < decls.size(); i++) loopvars.push_back(strdup(decls[i]->varname));

}


void chillAST_ForStmt::loseLoopWithLoopVar(char *var) {

  //fprintf(stderr, "\nchillAST_ForStmt::loseLoopWithLoopVar(  %s )\n", var );

  // now recurse (could do first, I suppose)
  // if you DON'T do this first, you may have already replaced yourself with this loop body
  // the body will no longer have this forstmt as parent, it will have the forstmt's parent as its parent
  //fprintf(stderr, "forstmt 0x%x, recursing loseLoop to body 0x%x of type %s with parent 0x%x of type %s\n", this, body,  body->getTypeString(), body->parent, body->parent->getTypeString());
  body->loseLoopWithLoopVar(var);




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
    init->gatherVarLHSUsage(decls);   // this can fail if init is outside the loop
    cond->gatherVarLHSUsage(decls);
    incr->gatherVarLHSUsage(decls);
    if (decls.size() > 1) {
      fprintf(stderr, "chill_ast.cc multiple loop variables confuses me\n");
      exit(-1);
    }
    chillAST_Node *newstmt = body;

    // ACTUALLY, if I am being replaced, and my loop conditional is a min (Ternary), then wrap my loop body in an if statement
    if (cond->isBinaryOperator()) { // what else could it be?
      chillAST_BinaryOperator *BO = (chillAST_BinaryOperator *) cond;
      if (BO->rhs->isTernaryOperator()) {

        chillAST_TernaryOperator *TO = (chillAST_TernaryOperator *) BO->rhs;
        chillAST_BinaryOperator *C = (chillAST_BinaryOperator *) TO->condition;

        //fprintf(stderr, "loop condition RHS  is ternary\nCondition RHS");
        C->print();
        chillAST_Node *l = C->lhs;
        if (l->isParenExpr()) l = ((chillAST_ParenExpr *) l)->subexpr;
        chillAST_Node *r = C->rhs;
        if (r->isParenExpr()) r = ((chillAST_ParenExpr *) r)->subexpr;

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
        chillAST_IfStmt *ifstmt = new chillAST_IfStmt(ifcond, body, NULL);

        newstmt = ifstmt;
      }
    }

    //fprintf(stderr, "forstmt 0x%x has parent 0x%x  of type %s\n", this, parent, parent->getTypeString());
    //fprintf(stderr, "forstmt will be replaced by\n");
    //newstmt->print(0,stderr); fprintf(stderr, "\n\n");

    parent->replaceChild(this, newstmt);
  }


}


chillAST_BinaryOperator::chillAST_BinaryOperator(chillAST_Node *l, const char *oper, chillAST_Node *r) {
  //fprintf(stderr, "chillAST_BinaryOperator::chillAST_BinaryOperator( l %p  %s  r %p,   parent %p)  this %p\n", l, oper, r, par, this);
  CHILL_DEBUG_PRINT("( l  %s  r )\n", oper);

  //if (l && r ) {
  //  fprintf(stderr, "("); l->print(0,stderr); fprintf(stderr, ") %s (", oper); r->print(0,stderr); fprintf(stderr, ")\n\n");
  //}

  lhs = l;
  rhs = r;

  if (lhs) lhs->setParent(this);
  if (rhs) rhs->setParent(this);  // may only have part of the lhs and rhs when binop is created
  if (oper) op = strdup(oper);

  // if this writes to lhs and lhs type has an 'imwrittento' concept, set that up
  if (isAssignmentOp()) {
    if (lhs && lhs->isArraySubscriptExpr()) {
      ((chillAST_ArraySubscriptExpr *) lhs)->imwrittento = true;
      //fprintf(stderr, "chillAST_BinaryOperator, op '=', lhs is an array reference  LVALUE\n");
    }
  }
  if (isAugmentedAssignmentOp()) {  // +=  etc
    //fprintf(stderr, "isAugmentedAssignmentOp()  "); print(); fflush(stdout);
    if (lhs && lhs->isArraySubscriptExpr()) {
      //fprintf(stderr, "lhs is also read from  ");  lhs->print(); fflush(stdout);
      ((chillAST_ArraySubscriptExpr *) lhs)->imreadfrom = true; // note will ALSO have imwrittento true
    }
  }

  isFromSourceFile = true; // default
  filename = NULL;
}


int chillAST_BinaryOperator::evalAsInt() {
  // very limited. allow +-*/ and integer literals ...
  if (isAssignmentOp()) return rhs->evalAsInt();  // ?? ignores/loses lhs info

  if (!strcmp("+", op)) {
    //fprintf(stderr, "chillAST_BinaryOperator::evalAsInt()   %d + %d\n", lhs->evalAsInt(), rhs->evalAsInt());
    return lhs->evalAsInt() + rhs->evalAsInt();
  }
  if (!strcmp("-", op)) return lhs->evalAsInt() - rhs->evalAsInt();
  if (!strcmp("*", op)) return lhs->evalAsInt() * rhs->evalAsInt();
  if (!strcmp("/", op)) return lhs->evalAsInt() / rhs->evalAsInt();

  fprintf(stderr, "chillAST_BinaryOperator::evalAsInt() unhandled op '%s'\n", op);
  exit(-1);
}

chillAST_IntegerLiteral *chillAST_BinaryOperator::evalAsIntegerLiteral() {
  return new chillAST_IntegerLiteral(evalAsInt()); // ??
}

class chillAST_Node *chillAST_BinaryOperator::constantFold() {
  //fprintf(stderr, "\nchillAST_BinaryOperator::constantFold()  ");
  //print(0,stderr); fprintf(stderr, "\n");

  lhs = lhs->constantFold();
  rhs = rhs->constantFold();

  chillAST_Node *returnval = this;

  if (lhs->isConstant() && rhs->isConstant()) {
    //fprintf(stderr, "binop folding constants\n"); print(0,stderr); fprintf(stderr, "\n");

    if (!strcmp(op, "+") || !strcmp(op, "-") || !strcmp(op, "*")) {
      if (lhs->isIntegerLiteral() && rhs->isIntegerLiteral()) {
        chillAST_IntegerLiteral *l = (chillAST_IntegerLiteral *) lhs;
        chillAST_IntegerLiteral *r = (chillAST_IntegerLiteral *) rhs;
        chillAST_IntegerLiteral *I;

        if (!strcmp(op, "+")) I = new chillAST_IntegerLiteral(l->value + r->value);
        if (!strcmp(op, "-")) I = new chillAST_IntegerLiteral(l->value - r->value);
        if (!strcmp(op, "*")) I = new chillAST_IntegerLiteral(l->value * r->value);

        returnval = I;
        //fprintf(stderr, "%d %s %d becomes %d\n", l->value,op, r->value, I->value);
      } else { // at least one is a float

        // usually don't want to do this for floats or doubles
        // could probably check for special cases like 0.0/30.0  or X/X  or X/1.0
#ifdef FOLDFLOATS
        float lval, rval;
        if (lhs->isIntegerLiteral()) {
          lval = (float) ((chillAST_IntegerLiteral *)lhs)->value;
        }
        else {
          lval = ((chillAST_FloatingLiteral *)lhs)->value;
        }

        if (rhs->isIntegerLiteral()) {
          rval = (float) ((chillAST_IntegerLiteral *)rhs)->value;
        }
        else {
          rval = ((chillAST_FloatingLiteral *)rhs)->value;
        }

        chillAST_FloatingLiteral *F;
        if (streq(op, "+")) F = new chillAST_FloatingLiteral(lval + rval, parent);
        if (streq(op, "-")) F = new chillAST_FloatingLiteral(lval - rval, parent);
        if (streq(op, "*")) F = new chillAST_FloatingLiteral(lval * rval, parent);

        returnval = F;
#endif

      }
    }
    //else fprintf(stderr, "can't fold op '%s' yet\n", op);
  }

  //fprintf(stderr, "returning "); returnval->print(0,stderr); fprintf(stderr, "\n");
  return returnval;
}


class chillAST_Node *chillAST_BinaryOperator::clone() {
  //fprintf(stderr, "chillAST_BinaryOperator::clone() "); print(); printf("\n"); fflush(stdout);

  chillAST_Node *l = lhs->clone();
  chillAST_Node *r = rhs->clone();
  chillAST_BinaryOperator *bo = new chillAST_BinaryOperator(l, op, r);
  l->setParent(bo);
  r->setParent(bo);
  bo->isFromSourceFile = isFromSourceFile;
  if (filename) bo->filename = strdup(filename);
  return bo;
}

void chillAST_BinaryOperator::gatherArrayRefs(std::vector<chillAST_ArraySubscriptExpr *> &refs, bool w) {
  //fprintf(stderr, "chillAST_BinaryOperator::gatherArrayRefs()\n");
  //print(); fflush(stdout); fprintf(stderr, "\n");
  //if (isAugmentedAssignmentOp()) {
  //  fprintf(stderr, "%s  is augmented assignment\n", op);
  //}

  //if (isAssignmentOp()) {
  //  fprintf(stderr, "%s  is assignment\n", op);
  //}

  //if (isAugmentedAssignmentOp()) { // lhs is ALSO on the RHS, NOT as a write
  //  if (lhs->isArraySubscriptExpr()) { // probably some case where this fails
  //    ((chillAST_ArraySubscriptExpr *) lhs)->imreadfrom = true;
  //    //lhs->&gatherArrayRefs( refs, 0 );
  //  }
  //}

  //fprintf(stderr, "in chillAST_BinaryOperator::gatherArrayRefs(), %d &arrayrefs before\n", refs.size());
  lhs->gatherArrayRefs(refs, isAssignmentOp());
  //fprintf(stderr, "in chillAST_BinaryOperator::gatherArrayRefs(), %d &arrayrefs after lhs\n", refs.size());
  rhs->gatherArrayRefs(refs, 0);
  //fprintf(stderr, "in chillAST_BinaryOperator::gatherArrayRefs(), %d &refs\n", refs.size());

  //for (int i=0; i<refs.size(); i++) {
  //  fprintf(stderr, "%s\n", (*refs)[i]->basedecl->varname);
  //}

}

void chillAST_BinaryOperator::gatherScalarRefs(std::vector<chillAST_DeclRefExpr *> &refs, bool writtento) {
  lhs->gatherScalarRefs(refs, isAssignmentOp());
  rhs->gatherScalarRefs(refs, 0);
}


void chillAST_BinaryOperator::replaceChild(chillAST_Node *old, chillAST_Node *newchild) {
  //fprintf(stderr, "\nbinop::replaceChild( old 0x%x, new )    lhs 0x%x   rhd 0x%x\n", old, lhs, rhs);

  // will pointers match??
  if (lhs == old) setLHS(newchild);
  else if (rhs == old) setRHS(newchild);

  // silently ignore?
  //else {
  //  fprintf(stderr, "\nERROR chillAST_BinaryOperator::replaceChild( old 0x%x, new )    lhs 0x%x   rhd 0x%x\n", old, lhs, rhs);
  //  fprintf(stderr, "old is not a child of this BinaryOperator\n");
  //  print();
  //  dump();
  //  exit(-1);
  //}
}


void chillAST_BinaryOperator::gatherVarDecls(vector<chillAST_VarDecl *> &decls) {
  //fprintf(stderr, "chillAST_BinaryOperator::gatherVarDecls()\n");

  //fprintf(stderr, "chillAST_BinaryOperator::gatherVarDecls()  before %d\n", decls.size());
  //print(0,stderr); fprintf(stderr, "\n");
  //fprintf(stderr, "lhs is %s\n", lhs->getTypeString());
  if (lhs) lhs->gatherVarDecls(decls); // 'if' to deal with partially formed
  if (rhs) rhs->gatherVarDecls(decls);
  //fprintf(stderr, "after %d\n", decls.size());
}


void chillAST_BinaryOperator::gatherScalarVarDecls(vector<chillAST_VarDecl *> &decls) {
  //fprintf(stderr, "chillAST_BinaryOperator::gatherScalarVarDecls()  before %d\n", decls.size());
  //fprintf(stderr, "lhs is %s\n", lhs->getTypeString());
  lhs->gatherScalarVarDecls(decls);
  rhs->gatherScalarVarDecls(decls);
  //fprintf(stderr, "after %d\n", decls.size());
}


void chillAST_BinaryOperator::gatherArrayVarDecls(vector<chillAST_VarDecl *> &decls) {
  //fprintf(stderr, "chillAST_BinaryOperator::gatherArrayVarDecls()  before %d\n", decls.size());
  //fprintf(stderr, "lhs is %s\n", lhs->getTypeString());
  lhs->gatherArrayVarDecls(decls);
  rhs->gatherArrayVarDecls(decls);
  //fprintf(stderr, "after %d\n", decls.size());
}


void chillAST_BinaryOperator::gatherDeclRefExprs(vector<chillAST_DeclRefExpr *> &refs) {
  lhs->gatherDeclRefExprs(refs);
  rhs->gatherDeclRefExprs(refs);
}


void chillAST_BinaryOperator::gatherStatements(std::vector<chillAST_Node *> &statements) {

  // what's legit?
  if (isAssignmentOp()) {
    statements.push_back(this);
  }

}


void chillAST_BinaryOperator::gatherVarUsage(vector<chillAST_VarDecl *> &decls) {
  lhs->gatherVarUsage(decls);
  rhs->gatherVarUsage(decls);
}

void chillAST_BinaryOperator::gatherVarLHSUsage(vector<chillAST_VarDecl *> &decls) {
  lhs->gatherVarUsage(decls);
}

void chillAST_BinaryOperator::replaceVarDecls(chillAST_VarDecl *olddecl, chillAST_VarDecl *newdecl) {
  //if (!strcmp(op, "<=")) {
  //  fprintf(stderr, "chillAST_BinaryOperator::replaceVarDecls( old %s, new %s)\n", olddecl->varname, newdecl->varname );
  //  print(); printf("\n"); fflush(stdout);
  //  fprintf(stderr, "binaryoperator, lhs is of type %s\n", lhs->getTypeString());
  //  fprintf(stderr, "binaryoperator, rhs is of type %s\n", rhs->getTypeString());
  //}
  lhs->replaceVarDecls(olddecl, newdecl);
  rhs->replaceVarDecls(olddecl, newdecl);
  //if (!strcmp(op, "<=")) {
  //  print(); printf("\n\n"); fflush(stdout);
  //}
}

chillAST_TernaryOperator::chillAST_TernaryOperator(const char *oper, chillAST_Node *c, chillAST_Node *l,
                                                   chillAST_Node *r) {
  if (op)
    op = strdup(oper);
  else
    op = strdup("?"); // the only one so far
  condition = c;
  lhs = l;
  rhs = r;
  if (condition) condition->setParent(this);
  if (lhs) lhs->setParent(this);
  if (rhs) rhs->setParent(this);
}

void chillAST_TernaryOperator::replaceChild(chillAST_Node *old, chillAST_Node *newchild) {
  //fprintf(stderr, "\nbinop::replaceChild( old 0x%x, new )    lhs 0x%x   rhd 0x%x\n", old, lhs, rhs);

  // will pointers match??
  if (lhs == old) setLHS(newchild);
  else if (rhs == old) setRHS(newchild);
  else if (condition == old) setCond(newchild);

  // silently ignore?
  //else {
  //}
}


void chillAST_TernaryOperator::gatherVarDecls(vector<chillAST_VarDecl *> &decls) {
  condition->gatherVarDecls(decls);
  lhs->gatherVarDecls(decls);
  rhs->gatherVarDecls(decls);
}

void chillAST_TernaryOperator::gatherScalarVarDecls(vector<chillAST_VarDecl *> &decls) {
  condition->gatherScalarVarDecls(decls);
  lhs->gatherScalarVarDecls(decls);
  rhs->gatherScalarVarDecls(decls);
}


void chillAST_TernaryOperator::gatherArrayVarDecls(vector<chillAST_VarDecl *> &decls) {
  condition->gatherArrayVarDecls(decls);
  lhs->gatherArrayVarDecls(decls);
  rhs->gatherArrayVarDecls(decls);
}


void chillAST_TernaryOperator::gatherDeclRefExprs(vector<chillAST_DeclRefExpr *> &refs) {
  condition->gatherDeclRefExprs(refs);
  lhs->gatherDeclRefExprs(refs);
  rhs->gatherDeclRefExprs(refs);
}


void chillAST_TernaryOperator::gatherVarUsage(vector<chillAST_VarDecl *> &decls) {
  condition->gatherVarUsage(decls);
  lhs->gatherVarUsage(decls);
  rhs->gatherVarUsage(decls);
}

void chillAST_TernaryOperator::gatherVarLHSUsage(vector<chillAST_VarDecl *> &decls) {
  // this makes no sense for ternary ??
}

void chillAST_TernaryOperator::replaceVarDecls(chillAST_VarDecl *olddecl, chillAST_VarDecl *newdecl) {
  condition->replaceVarDecls(olddecl, newdecl);
  lhs->replaceVarDecls(olddecl, newdecl);
  rhs->replaceVarDecls(olddecl, newdecl);
}

class chillAST_Node *chillAST_TernaryOperator::constantFold() {
  condition = condition->constantFold();
  lhs = lhs->constantFold();
  rhs = rhs->constantFold();

  chillAST_Node *returnval = this;

  if (condition->isConstant()) {
    // TODO
  }

  return returnval;
}

class chillAST_Node *chillAST_TernaryOperator::clone() {
  chillAST_Node *c = condition->clone();
  chillAST_Node *l = lhs->clone();
  chillAST_Node *r = rhs->clone();
  chillAST_TernaryOperator *to = new chillAST_TernaryOperator(op, l, r, parent);
  c->setParent(to);
  l->setParent(to);
  r->setParent(to);
  to->isFromSourceFile = isFromSourceFile;
  filename = NULL;
  return to;
}

void chillAST_TernaryOperator::gatherArrayRefs(std::vector<chillAST_ArraySubscriptExpr *> &refs, bool w) {
  condition->gatherArrayRefs(refs, isAssignmentOp());
  lhs->gatherArrayRefs(refs, isAssignmentOp());
  rhs->gatherArrayRefs(refs, 0);
}

void chillAST_TernaryOperator::gatherScalarRefs(std::vector<chillAST_DeclRefExpr *> &refs, bool writtento) {
  condition->gatherScalarRefs(refs, isAssignmentOp());
  lhs->gatherScalarRefs(refs, isAssignmentOp());
  rhs->gatherScalarRefs(refs, 0);
}

chillAST_ArraySubscriptExpr::chillAST_ArraySubscriptExpr(chillAST_Node *bas, chillAST_Node *indx, bool writtento,
                                                         void *unique) {
  base = index = NULL;
  basedecl = NULL; //fprintf(stderr, "setting basedecl NULL for ASE %p\n", this);
  imwrittento = writtento; // ??
  imreadfrom = false; // ??
  parent = NULL;
  metacomment = NULL;
  if (bas) {
    if (bas->isImplicitCastExpr()) base = ((chillAST_ImplicitCastExpr *) bas)->subexpr; // probably wrong
    else base = bas;
    base->setParent(this);
    basedecl = multibase();
  }
  if (indx) {
    if (indx->isImplicitCastExpr()) index = ((chillAST_ImplicitCastExpr *) indx)->subexpr; // probably wrong
    else index = indx;
    index->setParent(this);
  }
  uniquePtr = unique;
}

chillAST_ArraySubscriptExpr::chillAST_ArraySubscriptExpr(chillAST_VarDecl *v, std::vector<chillAST_Node *> indeces) {
  //fprintf(stderr, "\nchillAST_ArraySubscriptExpr::chillAST_ArraySubscriptExpr() 4\n");
  //fprintf(stderr,"chillAST_ArraySubscriptExpr( chillAST_VarDecl *v, std::vector<int> indeces)\n");
  //if (parent == NULL) {
  //  fprintf(stderr, "dammit.  ASE %p has no parent\n", this);
  //}


  int numindeces = indeces.size();
  for (int i = 0; i < numindeces; i++) {
    fprintf(stderr, "ASE index %d  ", i);
    indeces[i]->print(0, stderr);
    fprintf(stderr, "\n");
    //  printf("[");
    //  indeces[i]->print();
    //  printf("]");
  }
  //fflush(stdout);
  //fprintf(stderr, "\n");

  chillAST_DeclRefExpr *DRE = new chillAST_DeclRefExpr(v->vartype, v->varname, v);
  basedecl = v; // ??
  //fprintf(stderr, "%p  ASE 3 basedecl = %p   ", this, basedecl);
  //fprintf(stderr, "of type %s\n", basedecl->getTypeString());
  //basedecl->print(); printf("\n");
  //basedecl->dump(); printf("\n"); fflush(stdout);
  //fprintf(stderr, "basedecl varname %s\n", basedecl->varname);

  chillAST_ArraySubscriptExpr *rent = this; // parent for subnodes

  // these are on the top level ASE that we're creating here
  base = (chillAST_Node *) DRE;
  index = indeces[numindeces - 1];

  base->setParent(this);
  index->setParent(this);

  for (int i = numindeces - 2; i >= 0; i--) {

    chillAST_ArraySubscriptExpr *ASE = new chillAST_ArraySubscriptExpr(DRE, indeces[i], rent, 0);
    rent->base = ASE; //
    rent = ASE;
  }

  imwrittento = false;
  imreadfrom = false;
  //fprintf(stderr, "ASE is "); print(); printf("\n\n"); fflush(stdout);
  isFromSourceFile = true; // default
  filename = NULL;

  //fprintf(stderr, "\nASE %p   parent %p  ", this, parent); print(0,stderr); fprintf(stderr, "\n\n");
}


chillAST_Node *chillAST_Node::getEnclosingStatement(int level) {  // TODO do for subclasses?

  //fprintf(stderr, "chillAST_Node::getEnclosingStatement( level %d ) node type %s\n", level, getTypeString());
  //print(); printf("\n"); fflush(stdout);

  // so far, user will ONLY call this directly on an array subscript expression
  if (isArraySubscriptExpr()) return parent->getEnclosingStatement(level + 1);

  if (level != 0) {
    if (isBinaryOperator() ||
        isUnaryOperator() ||
        isTernaryOperator() ||
        isReturnStmt() ||
        isCallExpr()
        )
      return this;


    // things that are not endpoints. recurse through parent
    if (isMemberExpr()) return parent->getEnclosingStatement(level + 1);
    if (isImplicitCastExpr()) return parent->getEnclosingStatement(level + 1);
    if (isSizeof()) return parent->getEnclosingStatement(level + 1);
    if (isCStyleCastExpr()) return parent->getEnclosingStatement(level + 1);
    return NULL;
  }

  CHILL_ERROR("level %d type %s, returning NULL\n", level, getTypeString());
  exit(-1);

  return NULL;
}


void chillAST_ArraySubscriptExpr::gatherIndeces(std::vector<chillAST_Node *> &ind) {
  if (base->isArraySubscriptExpr()) ((chillAST_ArraySubscriptExpr *) base)->gatherIndeces(ind);
  ind.push_back(index);
}

chillAST_VarDecl *chillAST_ArraySubscriptExpr::multibase() {
  //this should probably be a chillAST_Node function instead of having all these ifs
  return base->multibase();
}


chillAST_Node *chillAST_ArraySubscriptExpr::getIndex(int dim) {
  CHILL_DEBUG_PRINT("chillAST_ArraySubscriptExpr::getIndex( %d )\n", dim);

  chillAST_Node *b = base;

  int depth = 0;
  std::vector<chillAST_Node *> ind;
  chillAST_Node *curindex = index;
  for (;;) {
    if (b->getType() == CHILLAST_NODE_IMPLICITCASTEXPR)
      b = ((chillAST_ImplicitCastExpr *) b)->subexpr;
    else if (b->getType() == CHILLAST_NODE_ARRAYSUBSCRIPTEXPR) {
      ind.push_back(curindex);
      curindex = ((chillAST_ArraySubscriptExpr *) b)->index;
      b = ((chillAST_ArraySubscriptExpr *) b)->base;
      depth++;
    } else {
      ind.push_back(curindex);
      break;
    }
  }
  return ind[depth - dim];
}


class chillAST_Node *chillAST_ArraySubscriptExpr::constantFold() {
  base = base->constantFold();
  index = index->constantFold();
  return this;
}

class chillAST_Node *chillAST_ArraySubscriptExpr::clone() {
  if (base->isDeclRefExpr()) {
    chillAST_VarDecl *vd = (chillAST_VarDecl *) (((chillAST_DeclRefExpr *) base)->decl);
  }
  chillAST_Node *b = base->clone();

  chillAST_Node *i = index->clone();

  chillAST_ArraySubscriptExpr *ASE = new chillAST_ArraySubscriptExpr(b, i, imwrittento, uniquePtr);
  ASE->setParent(parent);

  ASE->imreadfrom = false; // don't know this yet

  ASE->isFromSourceFile = isFromSourceFile;
  if (filename) ASE->filename = strdup(filename);
  return ASE;
}

void chillAST_ArraySubscriptExpr::gatherArrayRefs(std::vector<chillAST_ArraySubscriptExpr *> &refs, bool writtento) {
  //fprintf(stderr, "chillAST_ArraySubscriptExpr::gatherArrayRefs setting imwrittento %d for ", writtento);
//fprintf(stderr, "%s ", base->getTypeString());
//base->print(); printf("\n"); fflush(stdout);

  //fprintf(stderr, "found an array subscript. &refs 0x%x   ", refs);
  if (!imwrittento) imwrittento = writtento;   // may be both written and not for +=
  fflush(stdout);

  //fprintf(stderr, "recursing on index ");  index->print(0,stderr); fprintf(stderr, "\n");
  index->gatherArrayRefs(refs, 0); // recurse first
  //fprintf(stderr, "adding this "); print(0,stderr); fprintf(stderr, "\n");
  //fprintf(stderr, "refs[%d] = 0x%x  = ", refs.size(), this); print(); fflush(stdout);
  refs.push_back(this);

  //fprintf(stderr, " size now %d\n", refs.size());

}

void chillAST_ArraySubscriptExpr::gatherScalarRefs(std::vector<chillAST_DeclRefExpr *> &refs, bool writtento) {
  index->gatherScalarRefs(refs, 0);
}

void chillAST_ArraySubscriptExpr::gatherVarDecls(vector<chillAST_VarDecl *> &decls) {
  //fprintf(stderr, "chillAST_ArraySubscriptExpr::gatherVarDecls()\n");

  base->gatherVarDecls(decls);
  index->gatherVarDecls(decls);
}


void chillAST_ArraySubscriptExpr::gatherScalarVarDecls(vector<chillAST_VarDecl *> &decls) {
  //fprintf(stderr, "chillAST_ArraySubscriptExpr::gatherScalarVarDecls()\n");
  //fprintf(stderr, "base %s   index %s\n", base->getTypeString(), index->getTypeString());
  base->gatherScalarVarDecls(decls);
  index->gatherScalarVarDecls(decls);
}


void chillAST_ArraySubscriptExpr::gatherArrayVarDecls(vector<chillAST_VarDecl *> &decls) {
  //fprintf(stderr, "chillAST_ArraySubscriptExpr::gatherArrayVarDecls()\n");
  //fprintf(stderr, "base %s   index %s\n", base->getTypeString(), index->getTypeString());
  base->gatherArrayVarDecls(decls);
  index->gatherArrayVarDecls(decls);
}


void chillAST_ArraySubscriptExpr::gatherDeclRefExprs(vector<chillAST_DeclRefExpr *> &refs) {
  base->gatherDeclRefExprs(refs);
  index->gatherDeclRefExprs(refs);
}


void chillAST_ArraySubscriptExpr::gatherVarUsage(vector<chillAST_VarDecl *> &decls) {
  base->gatherVarUsage(decls);
  index->gatherVarUsage(decls);
}


void chillAST_ArraySubscriptExpr::replaceVarDecls(chillAST_VarDecl *olddecl, chillAST_VarDecl *newdecl) {
  base->replaceVarDecls(olddecl, newdecl);
  index->replaceVarDecls(olddecl, newdecl);
}


void chillAST_ArraySubscriptExpr::replaceChild(chillAST_Node *old, chillAST_Node *newchild) {

  if (old == index) {
    index = newchild;
    index->parent = this;
    return;
  }

  if (old == base) {
    base = newchild;
    base->parent = this;
    return;
  }

  CHILL_DEBUG_BEGIN
    fprintf(stderr, "chillAST_ArraySubscriptExpr::replaceChild() old is not base or index\n");
    print(0, stderr);
    fprintf(stderr, "\nchild: ");
    if (!old) fprintf(stderr, "oldchild NULL!\n");
    old->print(0, stderr);
    fprintf(stderr, "\nnew: ");
    newchild->print(0, stderr);
    fprintf(stderr, "\n");
  CHILL_DEBUG_END
  exit(-1); // make easier for gdb
};


bool chillAST_ArraySubscriptExpr::operator!=(const chillAST_ArraySubscriptExpr &other) {
  bool opposite = *this == other;
  return !opposite;
}


bool chillAST_ArraySubscriptExpr::operator==(const chillAST_ArraySubscriptExpr &other) {
  return this->uniquePtr == other.uniquePtr;
}


chillAST_MemberExpr::chillAST_MemberExpr(chillAST_Node *bas, const char *mem, void *unique,
                                         CHILLAST_MEMBER_EXP_TYPE t) {
  base = bas;
  if (bas)
    base->setParent(this);
  if (mem) member = strdup(mem);
  else
    member = NULL;
  uniquePtr = unique;
  exptype = t;

  return;  // ignore tests below ?? TODO ??


  // base needs to RESOLVE to a decl ref expr but may not BE one
  //   A.b . c   lhs is a binop or memberexpr

  if (bas->isBinaryOperator()) {
    //fprintf(stderr, "checking binop to see if it resolved to a declrefexpr\n");
    // cheat for now or just remove the check below
    return;
  }

  if (!(bas->isDeclRefExpr() || bas->isArraySubscriptExpr())) {
    fprintf(stderr, "chillAST_MemberExpr::chillAST_MemberExpr(), base is of type %s\n", bas->getTypeString());
    fprintf(stderr, "chillAST_MemberExpr::chillAST_MemberExpr(), base is not DeclRefExpr\n");

    base->print();
    printf(".%s\n", mem);
    fflush(stdout);
    exit(-1);
  }
}

// TODO member can be another member expression, Right?

class chillAST_Node *chillAST_MemberExpr::constantFold() {
  base = base->constantFold();
  //member = member->constantFold();
  return this;
}

class chillAST_Node *chillAST_MemberExpr::clone() {
  chillAST_Node *b = base->clone();
  char *m = strdup(member); // ??
  chillAST_MemberExpr *ME = new chillAST_MemberExpr(b, m, uniquePtr /* ?? */ );
  ME->isFromSourceFile = isFromSourceFile;
  if (filename) ME->filename = strdup(filename);
  return ME;
}

void chillAST_MemberExpr::gatherArrayRefs(std::vector<chillAST_ArraySubscriptExpr *> &refs, bool writtento) {
  fprintf(stderr, "chillAST_MemberExpr::gatherArrayRefs()   ");
  print(0, stderr);
  fprintf(stderr, "\n");
  fprintf(stderr, "base of of type %s\n", base->getTypeString());
  base->gatherArrayRefs(refs, writtento); //

}

void chillAST_MemberExpr::gatherScalarRefs(std::vector<chillAST_DeclRefExpr *> &refs, bool writtento) {
  base->gatherScalarRefs(refs, writtento);
}

void chillAST_MemberExpr::gatherVarDecls(vector<chillAST_VarDecl *> &decls) {
  base->gatherVarDecls(decls);
}

void chillAST_MemberExpr::gatherScalarVarDecls(vector<chillAST_VarDecl *> &decls) {
  base->gatherScalarVarDecls(decls);
}


void chillAST_MemberExpr::gatherArrayVarDecls(vector<chillAST_VarDecl *> &decls) {
  base->gatherArrayVarDecls(decls);
}


void chillAST_MemberExpr::gatherDeclRefExprs(vector<chillAST_DeclRefExpr *> &refs) {
  base->gatherDeclRefExprs(refs);
}


void chillAST_MemberExpr::gatherVarUsage(vector<chillAST_VarDecl *> &decls) {
  base->gatherVarUsage(decls);
}


void chillAST_MemberExpr::replaceVarDecls(chillAST_VarDecl *olddecl, chillAST_VarDecl *newdecl) {
  base->replaceVarDecls(olddecl, newdecl);
}

bool chillAST_MemberExpr::operator!=(const chillAST_MemberExpr &other) {
  bool opposite = *this == other;
  return !opposite;
}

bool chillAST_MemberExpr::operator==(const chillAST_MemberExpr &other) {
  return this->uniquePtr == other.uniquePtr;
}


void chillAST_MemberExpr::replaceChild(chillAST_Node *old, chillAST_Node *newchild) {
  //printf("\nMemberExpr::replaceChild(  )\n");
  //printf("old: ");
  //old->print();
  //printf("\nnew: ");
  //newchild->print();
  //printf("\n"); fflush(stdout);

  // will pointers match??
  if (base == old) {
    //fprintf(stderr, "old matches base of MemberExpr\n");
    base = newchild;
  } else {
    base->replaceChild(old, newchild);
  }
}

chillAST_Node *chillAST_MemberExpr::multibase2() {  /*fprintf(stderr, "ME MB2\n" );*/ return (chillAST_Node *) this; }

chillAST_VarDecl *chillAST_MemberExpr::getUnderlyingVarDecl() {
  fprintf(stderr, "chillAST_MemberExpr:getUnderlyingVarDecl()\n");
  print();
  exit(-1);
  // find the member with the correct name

}


chillAST_VarDecl *chillAST_MemberExpr::multibase() {
  //c.i[c.count]    we want i member of c
  //fprintf(stderr, "ME MB\n" );

  //fprintf(stderr, "chillAST_MemberExpr::multibase()\n");
  //print(); printf("\n"); fflush(stdout);
  //fprintf(stderr, "MemberExpr base is type %s,  member %s\n", base->getTypeString(), member);

  //chillAST_VarDecl *vd = base->getUnderlyingVarDecl(); // this is the only thing that ever calls this ???
  chillAST_VarDecl *vd = base->multibase(); // ??


  //fprintf(stderr, "vd "); vd->print(); printf("\n"); fflush(stdout);

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
  //fprintf(stderr, "sub %s:\n", member);
  if (!sub) {
    fprintf(stderr, "can't find member %s in \n", member);
    rd->print();
  }
  //sub->print(); printf("\n");  fflush(stdout);
  //sub->dump() ; printf("\n");  fflush(stdout);

  return sub;
  //find vardecl of member in def of base


}

chillAST_DeclRefExpr::chillAST_DeclRefExpr(const char *vartype, const char *varname, chillAST_Node *d) {
  //fprintf(stderr, "DRE::DRE2 0x%x   %s %s  0x%x\n", this, vartype, varname, d );
  if (vartype) declarationType = strdup(vartype);
  else declarationType = strdup("UNKNOWN");
  if (varname) declarationName = strdup(varname);
  else declarationName = strdup("NONE");
  decl = d;
}

chillAST_DeclRefExpr::chillAST_DeclRefExpr(chillAST_VarDecl *vd) { // variable def
  //fprintf(stderr, "DRE::DRE3 (VD)  0x%x   %s %s  0x%x\n", this, vd->vartype, vd->varname, vd );

  declarationType = strdup(vd->vartype);
  declarationName = strdup(vd->varname);
  decl = vd;
  isFromSourceFile = true; // default
  filename = NULL;
}


chillAST_DeclRefExpr::chillAST_DeclRefExpr(chillAST_FunctionDecl *fd) { // function def
  declarationType = strdup(fd->returnType);
  declarationName = strdup(fd->functionName);
  decl = fd;
  isFromSourceFile = true; // default
  filename = NULL;
}


chillAST_DeclRefExpr *buildDeclRefExpr(chillAST_VarDecl *vd) {
  chillAST_DeclRefExpr *dre = new chillAST_DeclRefExpr(vd);
  return dre;
}

class chillAST_Node *chillAST_DeclRefExpr::constantFold() {  // can never do anything?
  return this;
}

class chillAST_Node *chillAST_DeclRefExpr::clone() {
  //fprintf(stderr, "chillAST_DeclRefExpr::clone()\n");
  chillAST_DeclRefExpr *DRE = new chillAST_DeclRefExpr(declarationType, declarationName, decl);
  DRE->setParent(parent);
  DRE->isFromSourceFile = isFromSourceFile;
  if (filename) DRE->filename = strdup(filename);
  return DRE;
}


void chillAST_DeclRefExpr::gatherVarDeclsMore(vector<chillAST_VarDecl *> &decls) {
  //fprintf(stderr, "chillAST_DeclRefExpr::gatherVarDeclsMore()\n");
  decl->gatherVarDeclsMore(decls);
}


void chillAST_DeclRefExpr::gatherScalarVarDecls(vector<chillAST_VarDecl *> &decls) {
  //fprintf(stderr, "chillAST_DeclRefExpr::gatherScalarVarDecls()\n");
  decl->gatherScalarVarDecls(decls);
  //fprintf(stderr, "now %d scalar vardecls\n", decls.size());
}


void chillAST_DeclRefExpr::gatherArrayVarDecls(vector<chillAST_VarDecl *> &decls) {
  //fprintf(stderr, "chillAST_DeclRefExpr::gatherArrayVarDecls()\n");
  decl->gatherArrayVarDecls(decls);
  //fprintf(stderr, "now %d Array vardecls\n", decls.size());
}


void chillAST_DeclRefExpr::gatherDeclRefExprs(vector<chillAST_DeclRefExpr *> &refs) {
  refs.push_back(this);
}

void chillAST_DeclRefExpr::gatherScalarRefs(std::vector<chillAST_DeclRefExpr *> &refs, bool writtento) {
  refs.push_back(this);
}

void chillAST_DeclRefExpr::gatherVarUsage(vector<chillAST_VarDecl *> &decls) {
  //fprintf(stderr, "chillAST_DeclRefExpr::gatherVarUsage()\n");
  for (int i = 0; i < decls.size(); i++) {
    if (decls[i] == decl) {
      //fprintf(stderr, "decl was already there\n");
      return;
    }
    if (streq(declarationName, decls[i]->varname)) {
      if (streq(declarationType, decls[i]->vartype)) {
        //fprintf(stderr, "decl was already there\n");
        return;
      }
    }
  }
  chillAST_VarDecl *vd = getVarDecl();  // null for functiondecl
  if (vd) decls.push_back(vd);

}


void chillAST_DeclRefExpr::replaceVarDecls(chillAST_VarDecl *olddecl, chillAST_VarDecl *newdecl) {
  //fprintf(stderr, "chillAST_DeclRefExpr::replaceVarDecls()\n");
  if (decl == olddecl) {
    //fprintf(stderr, "replacing old %s with %s\n", olddecl->varname, newdecl->varname);
    //fprintf(stderr, "DRE was "); print();
    decl = newdecl;
    declarationType = strdup(newdecl->vartype);
    declarationName = strdup(newdecl->varname);
    //fprintf(stderr, "\nDRE  is "); print(); fprintf(stderr, "\n\n");
  } else {
    if (!strcmp(olddecl->varname, declarationName)) {
      //fprintf(stderr, "uhoh, chillAST_DeclRefExpr::replaceVarDecls()\n");
      decl = newdecl;
      declarationType = strdup(newdecl->vartype);
      declarationName = strdup(newdecl->varname);
    }
  }
}

chillAST_VarDecl *chillAST_ImplicitCastExpr::multibase() {
  return subexpr->multibase();
}


chillAST_VarDecl *chillAST_DeclRefExpr::multibase() {
  // presumably, this is being called because this DRE is the base of an ArraySubscriptExpr
  return getVarDecl();
}


void chillAST_VarDecl::gatherVarDecls(vector<chillAST_VarDecl *> &decls) {
  //fprintf(stderr, "chillAST_VarDecl::gatherVarDecls()\n");
  for (int i = 0; i < decls.size(); i++) {
    if (decls[i] == this) {
      //fprintf(stderr, "decl was already there\n");
      return;
    }
    if (streq(decls[i]->varname, varname)) {
      if (streq(decls[i]->vartype, vartype)) {
        //fprintf(stderr, "VarDecl (direct) decl was already there\n");
        return;
      }
    }
  }
  decls.push_back(this);
}


void chillAST_VarDecl::gatherScalarVarDecls(vector<chillAST_VarDecl *> &decls) {
  //fprintf(stderr, "chillAST_VarDecl::gatherScalarVarDecls(), %s numdimensions %d\n", varname, numdimensions);

  if (numdimensions != 0) return; // not a scalar

  for (int i = 0; i < decls.size(); i++) {
    if (decls[i] == this) {
      //fprintf(stderr, "decl was already there\n");
      return;
    }

    if (streq(decls[i]->varname, varname)) {      // wrong. scoping.  TODO
      if (streq(decls[i]->vartype, vartype)) {
        //fprintf(stderr, "VarDecl (direct) decl was already there\n");
        return;
      }
    }
  }
  //fprintf(stderr, "adding vardecl for %s to decls\n", varname);
  decls.push_back(this);
}


void chillAST_VarDecl::gatherArrayVarDecls(vector<chillAST_VarDecl *> &decls) {
  //fprintf(stderr, "chillAST_VarDecl::gatherScalarVarDecls(), %s numdimensions %d\n", varname, numdimensions);

  if (numdimensions == 0) return; // not an array

  for (int i = 0; i < decls.size(); i++) {
    if (decls[i] == this) {
      //fprintf(stderr, "decl was already there\n");
      return;
    }

    if (streq(decls[i]->varname, varname)) {      // wrong. scoping.  TODO
      if (streq(decls[i]->vartype, vartype)) {
        //fprintf(stderr, "VarDecl (direct) decl was already there\n");
        return;
      }
    }
  }
  //fprintf(stderr, "adding vardecl for %s to decls\n", varname);
  decls.push_back(this);
}


chillAST_Node *chillAST_VarDecl::constantFold() { return this; }

chillAST_Node *chillAST_VarDecl::clone() {
  fprintf(stderr, "\nchillAST_VarDecl::clone()  cloning vardecl for %s\n", varname);
  if (isAParameter) fprintf(stderr, "old vardecl IS a parameter\n");
  //else  fprintf(stderr, "old vardecl IS NOT a parameter\n");

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
    //fprintf(stderr, "in chillAST_VarDecl::clone(), cloning the array info\n");
    //fprintf(stderr, "numdimensions %d     arraysizes 0x%x\n", numdimensions, arraysizes) ;
    vd->numdimensions = numdimensions;

    if (arraysizes) {
      vd->arraysizes = (int *) malloc(sizeof(int *) * numdimensions);
      for (int i = 0; i < numdimensions; i++) {
        //fprintf(stderr, "i %d\n", i);
        vd->arraysizes[i] = arraysizes[i];
      }
    }
  }

  if (arraypointerpart) {
    //fprintf(stderr, "copying arraypointerpart\n");
    vd->arraypointerpart = strdup(arraypointerpart);
  }

  vd->isStruct = this->isStruct;
  //vd->insideAStruct =  this->insideAStruct;

  //if (vd->isStruct)  fprintf(stderr, "vardecl::clone()  %s is a struct\n", varname);
  //else fprintf(stderr, "vardecl::clone()  %s is NOT a struct\n", varname);


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

class chillAST_Node *chillAST_IntegerLiteral::constantFold() { return this; } // can never do anything


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

chillAST_Node *chillAST_FloatingLiteral::constantFold() { return this; }; // NOOP

chillAST_Node *chillAST_FloatingLiteral::clone() {
  //fprintf(stderr, "chillAST_FloatingLiteral::clone()  ");
  //fprintf(stderr, "allthedigits %p \n", allthedigits);
  chillAST_FloatingLiteral *newone = new chillAST_FloatingLiteral(this);

  newone->isFromSourceFile = isFromSourceFile;
  if (filename) newone->filename = strdup(filename);
  //print(); printf("  "); newone->print(); printf("\n"); fflush(stdout);
  return newone;
}

chillAST_UnaryOperator::chillAST_UnaryOperator(const char *oper, bool pre, chillAST_Node *sub) {
  op = strdup(oper);
  prefix = pre;
  subexpr = sub;
  subexpr->setParent(this);
  isFromSourceFile = true; // default
  filename = NULL;
}

void chillAST_UnaryOperator::gatherArrayRefs(std::vector<chillAST_ArraySubscriptExpr *> &refs, bool w) {
  subexpr->gatherArrayRefs(refs, isAssignmentOp()); //
}

void chillAST_UnaryOperator::gatherVarLHSUsage(vector<chillAST_VarDecl *> &decls) {
  if ((!strcmp("++", op)) || (!strcmp("--", op))) {
    subexpr->gatherVarUsage(decls); // do all unary modify the subexpr? (no, - )
  }
}


chillAST_Node *chillAST_UnaryOperator::constantFold() {
  //fprintf(stderr, "chillAST_UnaryOperator::constantFold() ");
  //print(); fprintf(stderr, "\n");

  subexpr = subexpr->constantFold();
  chillAST_Node *returnval = this;
  if (subexpr->isConstant()) {
    //fprintf(stderr, "unary op folding constants\n");
    //print(0,stderr); fprintf(stderr, "\n");

    if (streq(op, "-")) {
      if (subexpr->isIntegerLiteral()) {
        int intval = ((chillAST_IntegerLiteral *) subexpr)->value;
        chillAST_IntegerLiteral *I = new chillAST_IntegerLiteral(-intval);
        I->setParent(parent);
        returnval = I;
        //fprintf(stderr, "integer -%d becomes %d\n", intval, I->value);
      } else {
        chillAST_FloatingLiteral *FL = (chillAST_FloatingLiteral *) subexpr;
        chillAST_FloatingLiteral *F = new chillAST_FloatingLiteral(FL); // clone
        F->parent = FL->parent;

        F->value = -F->value;
        F->print();
        fprintf(stderr, "\n");

        returnval = F;
      }
    } else fprintf(stderr, "can't fold op '%s' yet\n", op);
  }
  return returnval;
}


class chillAST_Node *chillAST_UnaryOperator::clone() {
  chillAST_UnaryOperator *UO = new chillAST_UnaryOperator(op, prefix, subexpr->clone());
  UO->setParent(parent);
  UO->isFromSourceFile = isFromSourceFile;
  if (filename) UO->filename = strdup(filename);
  return UO;
}


void chillAST_UnaryOperator::gatherVarDecls(vector<chillAST_VarDecl *> &decls) {
  subexpr->gatherVarDecls(decls);
}


void chillAST_UnaryOperator::gatherScalarVarDecls(vector<chillAST_VarDecl *> &decls) {
  subexpr->gatherScalarVarDecls(decls);
}


void chillAST_UnaryOperator::gatherArrayVarDecls(vector<chillAST_VarDecl *> &decls) {
  subexpr->gatherArrayVarDecls(decls);
}


void chillAST_UnaryOperator::gatherDeclRefExprs(vector<chillAST_DeclRefExpr *> &refs) {
  subexpr->gatherDeclRefExprs(refs);
}


void chillAST_UnaryOperator::gatherVarUsage(vector<chillAST_VarDecl *> &decls) {
  subexpr->gatherVarUsage(decls);
}

void chillAST_UnaryOperator::replaceVarDecls(chillAST_VarDecl *olddecl, chillAST_VarDecl *newdecl) {
  subexpr->replaceVarDecls(olddecl, newdecl);
}


int chillAST_UnaryOperator::evalAsInt() {
  if (!strcmp("+", op)) return subexpr->evalAsInt();
  if (!strcmp("-", op)) return -subexpr->evalAsInt();
  if (!strcmp("++", op)) return 1 + subexpr->evalAsInt();
  if (!strcmp("--", op)) return subexpr->evalAsInt() - 1;

  fprintf(stderr, "chillAST_UnaryOperator::evalAsInt() unhandled op '%s'\n", op);
  exit(-1);

}

chillAST_ImplicitCastExpr::chillAST_ImplicitCastExpr(chillAST_Node *sub) {
  subexpr = sub;
  subexpr->setParent(this);
  //fprintf(stderr, "ImplicitCastExpr 0x%x  has subexpr 0x%x", this, subexpr);
  //fprintf(stderr, " of type %s\n", subexpr->getTypeString());
}

void chillAST_ImplicitCastExpr::replaceChild(chillAST_Node *old, chillAST_Node *newchild) {
  if (subexpr == old) { // should be the case for this to get called
    subexpr = newchild;
    subexpr->setParent(this);
    //old->parent = NULL;
    return;
  }

  fprintf(stderr, "chillAST_ImplicitCastExpr::replaceChild() called with bad 'old'\n");
  exit(-1);  // ??
}

class chillAST_Node *chillAST_ImplicitCastExpr::constantFold() {
  chillAST_Node *child = subexpr->constantFold();
  child->setParent(parent); // remove myself !! probably a bad idea. TODO
  return child;
}


class chillAST_Node *chillAST_ImplicitCastExpr::clone() {
  chillAST_ImplicitCastExpr *ICE = new chillAST_ImplicitCastExpr(subexpr->clone());
  ICE->setParent(getParent());
  ICE->isFromSourceFile = isFromSourceFile;
  if (filename) ICE->filename = strdup(filename);
  return ICE;
}


void chillAST_ImplicitCastExpr::gatherArrayRefs(std::vector<chillAST_ArraySubscriptExpr *> &refs, bool w) {
  subexpr->gatherArrayRefs(refs, w);
}

void chillAST_ImplicitCastExpr::gatherScalarRefs(std::vector<chillAST_DeclRefExpr *> &refs, bool writtento) {
  subexpr->gatherScalarRefs(refs, writtento);
}

void chillAST_ImplicitCastExpr::gatherVarDecls(vector<chillAST_VarDecl *> &decls) {
  subexpr->gatherVarDecls(decls);
}


void chillAST_ImplicitCastExpr::gatherScalarVarDecls(vector<chillAST_VarDecl *> &decls) {
  subexpr->gatherScalarVarDecls(decls);
}


void chillAST_ImplicitCastExpr::gatherArrayVarDecls(vector<chillAST_VarDecl *> &decls) {
  subexpr->gatherArrayVarDecls(decls);
}


void chillAST_ImplicitCastExpr::gatherDeclRefExprs(vector<chillAST_DeclRefExpr *> &refs) {
  subexpr->gatherDeclRefExprs(refs);
}


void chillAST_ImplicitCastExpr::gatherVarUsage(vector<chillAST_VarDecl *> &decls) {
  subexpr->gatherVarUsage(decls);
}


chillAST_CStyleCastExpr::chillAST_CStyleCastExpr(const char *to, chillAST_Node *sub) {

  //fprintf(stderr, "chillAST_CStyleCastExpr::chillAST_CStyleCastExpr( %s, ...)\n", to);
  towhat = strdup(to);
  subexpr = sub;
  if (subexpr) subexpr->setParent(this);
  //fprintf(stderr, "chillAST_CStyleCastExpr (%s)   sub 0x%x\n", towhat, sub );
}

void chillAST_CStyleCastExpr::replaceChild(chillAST_Node *old, chillAST_Node *newchild) {
  if (subexpr == old) { // should be the case for this to get called
    subexpr = newchild;
    subexpr->setParent(this);
    //old->parent = NULL;
    return;
  }

  fprintf(stderr, "chillAST_CStyleCastExpr::replaceChild() called with bad 'old'\n");
  exit(-1);  // ??
}

void chillAST_CStyleCastExpr::replaceVarDecls(chillAST_VarDecl *olddecl, chillAST_VarDecl *newdecl) {
  subexpr->replaceVarDecls(olddecl, newdecl);
}

class chillAST_Node *chillAST_CStyleCastExpr::constantFold() {
  subexpr = subexpr->constantFold();
  return this;
}


class chillAST_Node *chillAST_CStyleCastExpr::clone() {
  chillAST_CStyleCastExpr *CSCE = new chillAST_CStyleCastExpr(towhat, subexpr->clone());
  CSCE->setParent(getParent());
  CSCE->isFromSourceFile = isFromSourceFile;
  if (filename) CSCE->filename = strdup(filename);
  return CSCE;
}

void chillAST_CStyleCastExpr::gatherArrayRefs(std::vector<chillAST_ArraySubscriptExpr *> &refs, bool w) {
  subexpr->gatherArrayRefs(refs, w);
}

void chillAST_CStyleCastExpr::gatherScalarRefs(std::vector<chillAST_DeclRefExpr *> &refs, bool writtento) {
  subexpr->gatherScalarRefs(refs, writtento);
}


void chillAST_CStyleCastExpr::gatherVarDecls(vector<chillAST_VarDecl *> &decls) {
  subexpr->gatherVarDecls(decls);
}


void chillAST_CStyleCastExpr::gatherScalarVarDecls(vector<chillAST_VarDecl *> &decls) {
  subexpr->gatherScalarVarDecls(decls);
}


void chillAST_CStyleCastExpr::gatherArrayVarDecls(vector<chillAST_VarDecl *> &decls) {
  subexpr->gatherArrayVarDecls(decls);
}


void chillAST_CStyleCastExpr::gatherDeclRefExprs(vector<chillAST_DeclRefExpr *> &refs) {
  subexpr->gatherDeclRefExprs(refs);
}


void chillAST_CStyleCastExpr::gatherVarUsage(vector<chillAST_VarDecl *> &decls) {
  subexpr->gatherVarUsage(decls);
}


chillAST_CStyleAddressOf::chillAST_CStyleAddressOf(chillAST_Node *sub) {
  subexpr = sub;
  subexpr->setParent(this);
  //fprintf(stderr, "chillAST_CStyleCastExpr (%s)   sub 0x%x\n", towhat, sub );
}

class chillAST_Node *chillAST_CStyleAddressOf::constantFold() {
  subexpr = subexpr->constantFold();
  return this;
}

class chillAST_Node *chillAST_CStyleAddressOf::clone() {
  chillAST_CStyleAddressOf *CSAO = new chillAST_CStyleAddressOf(subexpr->clone());
  CSAO->setParent(getParent());
  CSAO->isFromSourceFile = isFromSourceFile;
  if (filename) CSAO->filename = strdup(filename);
  return CSAO;
}

void chillAST_CStyleAddressOf::gatherArrayRefs(std::vector<chillAST_ArraySubscriptExpr *> &refs, bool w) {
  subexpr->gatherArrayRefs(refs, w);
}

void chillAST_CStyleAddressOf::gatherScalarRefs(std::vector<chillAST_DeclRefExpr *> &refs, bool writtento) {
  subexpr->gatherScalarRefs(refs, writtento);
}

void chillAST_CStyleAddressOf::gatherVarDecls(vector<chillAST_VarDecl *> &decls) {
  subexpr->gatherVarDecls(decls);
}

void chillAST_CStyleAddressOf::gatherScalarVarDecls(vector<chillAST_VarDecl *> &decls) {
  subexpr->gatherScalarVarDecls(decls);
}


void chillAST_CStyleAddressOf::gatherArrayVarDecls(vector<chillAST_VarDecl *> &decls) {
  subexpr->gatherArrayVarDecls(decls);
}


void chillAST_CStyleAddressOf::gatherDeclRefExprs(vector<chillAST_DeclRefExpr *> &refs) {
  subexpr->gatherDeclRefExprs(refs);
}


void chillAST_CStyleAddressOf::gatherVarUsage(vector<chillAST_VarDecl *> &decls) {
  subexpr->gatherVarUsage(decls);
}


chillAST_Malloc::chillAST_Malloc(chillAST_Node *numthings) {
  sizeexpr = numthings;
  isFromSourceFile = true; // default
  filename = NULL;
};

chillAST_Node *chillAST_Malloc::constantFold() {
  sizeexpr->constantFold();
}

chillAST_Node *chillAST_Malloc::clone() {
  chillAST_Malloc *M = new chillAST_Malloc(sizeexpr); // the general version
  M->setParent(getParent());
  M->isFromSourceFile = isFromSourceFile;
  if (filename) M->filename = strdup(filename);
  return M;
};

void chillAST_Malloc::gatherArrayRefs(std::vector<chillAST_ArraySubscriptExpr *> &refs, bool writtento) {
  sizeexpr->gatherArrayRefs(refs, writtento);
};


void chillAST_Malloc::gatherScalarRefs(std::vector<chillAST_DeclRefExpr *> &refs, bool writtento) {
  sizeexpr->gatherScalarRefs(refs, writtento);
};

void chillAST_Malloc::gatherVarDecls(vector<chillAST_VarDecl *> &decls) {
  sizeexpr->gatherVarDecls(decls);
};

void chillAST_Malloc::gatherScalarVarDecls(vector<chillAST_VarDecl *> &decls) {
  sizeexpr->gatherScalarVarDecls(decls);
};

void chillAST_Malloc::gatherArrayVarDecls(vector<chillAST_VarDecl *> &decls) {
  sizeexpr->gatherArrayVarDecls(decls);
};

void chillAST_Malloc::gatherVarUsage(vector<chillAST_VarDecl *> &decls) {
  sizeexpr->gatherVarUsage(decls);
};

chillAST_CudaMalloc::chillAST_CudaMalloc(chillAST_Node *devmemptr, chillAST_Node *size) {
  devPtr = devmemptr;
  sizeinbytes = size;  // probably a multiply like   sizeof(int) * 1024
};

class chillAST_Node *chillAST_CudaMalloc::constantFold() {
  devPtr = devPtr->constantFold();
  return this;
}

class chillAST_Node *chillAST_CudaMalloc::clone() {
  chillAST_CudaMalloc *CM = new chillAST_CudaMalloc(devPtr->clone(), sizeinbytes->clone());
  CM->setParent(getParent());
  CM->isFromSourceFile = isFromSourceFile;
  if (filename) CM->filename = strdup(filename);
  return CM;
}

void chillAST_CudaMalloc::gatherArrayRefs(std::vector<chillAST_ArraySubscriptExpr *> &refs, bool w) {
  devPtr->gatherArrayRefs(refs, false);
  sizeinbytes->gatherArrayRefs(refs, false);
}

void chillAST_CudaMalloc::gatherScalarRefs(std::vector<chillAST_DeclRefExpr *> &refs, bool writtento) {
  devPtr->gatherScalarRefs(refs, false);
  sizeinbytes->gatherScalarRefs(refs, false);
}

void chillAST_CudaMalloc::gatherVarDecls(vector<chillAST_VarDecl *> &decls) {
  devPtr->gatherVarDecls(decls);
  sizeinbytes->gatherVarDecls(decls);
}


void chillAST_CudaMalloc::gatherScalarVarDecls(vector<chillAST_VarDecl *> &decls) {
  devPtr->gatherScalarVarDecls(decls);
  sizeinbytes->gatherScalarVarDecls(decls);
}


void chillAST_CudaMalloc::gatherArrayVarDecls(vector<chillAST_VarDecl *> &decls) {
  devPtr->gatherArrayVarDecls(decls);
  sizeinbytes->gatherArrayVarDecls(decls);
}


void chillAST_CudaMalloc::gatherVarUsage(vector<chillAST_VarDecl *> &decls) {
  devPtr->gatherVarUsage(decls);
  sizeinbytes->gatherVarUsage(decls);
}


chillAST_CudaFree::chillAST_CudaFree(chillAST_VarDecl *var) {
  variable = var;
};

class chillAST_Node *chillAST_CudaFree::constantFold() {
  return this;
}

class chillAST_Node *chillAST_CudaFree::clone() {
  chillAST_CudaFree *CF = new chillAST_CudaFree(variable);
  CF->setParent(getParent());
  CF->isFromSourceFile = isFromSourceFile;
  if (filename) CF->filename = strdup(filename);
  return CF;
}

void chillAST_CudaFree::gatherArrayRefs(std::vector<chillAST_ArraySubscriptExpr *> &refs, bool w) {}

void chillAST_CudaFree::gatherScalarRefs(std::vector<chillAST_DeclRefExpr *> &refs, bool writtento) {}

void chillAST_CudaFree::gatherVarDecls(vector<chillAST_VarDecl *> &decls) {
  variable->gatherVarDecls(decls);
}


void chillAST_CudaFree::gatherScalarVarDecls(vector<chillAST_VarDecl *> &decls) {
  variable->gatherScalarVarDecls(decls);
}


void chillAST_CudaFree::gatherArrayVarDecls(vector<chillAST_VarDecl *> &decls) {
  variable->gatherArrayVarDecls(decls);
}


void chillAST_CudaFree::gatherVarUsage(vector<chillAST_VarDecl *> &decls) {
  variable->gatherVarUsage(decls);
}


chillAST_CudaMemcpy::chillAST_CudaMemcpy(chillAST_VarDecl *d, chillAST_VarDecl *s, chillAST_Node *siz, char *kind) {
  dest = d;
  src = s;
  //fprintf(stderr, "chillAST_CudaMemcpy::chillAST_CudaMemcpy( dest %s, src %s, ...)\n", d->varname, s->varname );
  size = siz;
  cudaMemcpyKind = kind;
};

class chillAST_Node *chillAST_CudaMemcpy::constantFold() {
  dest = (chillAST_VarDecl *) dest->constantFold();
  src = (chillAST_VarDecl *) src->constantFold();
  size = size->constantFold();
  return this;
}

class chillAST_Node *chillAST_CudaMemcpy::clone() {
  chillAST_CudaMemcpy *CMCPY = new chillAST_CudaMemcpy((chillAST_VarDecl *) (dest->clone()),
                                                       (chillAST_VarDecl *) (src->clone()), size->clone(),
                                                       strdup(cudaMemcpyKind));
  CMCPY->setParent(getParent());
  CMCPY->isFromSourceFile = isFromSourceFile;
  if (filename) CMCPY->filename = strdup(filename);
  return CMCPY;
}

void chillAST_CudaMemcpy::gatherArrayRefs(std::vector<chillAST_ArraySubscriptExpr *> &refs, bool w) {
  dest->gatherArrayRefs(refs, false);
  src->gatherArrayRefs(refs, false);
  size->gatherArrayRefs(refs, false);
}

void chillAST_CudaMemcpy::gatherScalarRefs(std::vector<chillAST_DeclRefExpr *> &refs, bool writtento) {
  dest->gatherScalarRefs(refs, false);
  src->gatherScalarRefs(refs, false);
  size->gatherScalarRefs(refs, false);
}

void chillAST_CudaMemcpy::gatherVarDecls(vector<chillAST_VarDecl *> &decls) {
  dest->gatherVarDecls(decls);
  src->gatherVarDecls(decls);
  size->gatherVarDecls(decls);
}


void chillAST_CudaMemcpy::gatherScalarVarDecls(vector<chillAST_VarDecl *> &decls) {
  dest->gatherScalarVarDecls(decls);
  src->gatherScalarVarDecls(decls);
  size->gatherScalarVarDecls(decls);
}


void chillAST_CudaMemcpy::gatherArrayVarDecls(vector<chillAST_VarDecl *> &decls) {
  dest->gatherArrayVarDecls(decls);
  src->gatherArrayVarDecls(decls);
  size->gatherArrayVarDecls(decls);
}


void chillAST_CudaMemcpy::gatherVarUsage(vector<chillAST_VarDecl *> &decls) {
  dest->gatherVarUsage(decls);
  src->gatherVarUsage(decls);
  size->gatherVarUsage(decls);
}


chillAST_CudaSyncthreads::chillAST_CudaSyncthreads() {
}

chillAST_ReturnStmt::chillAST_ReturnStmt(chillAST_Node *retval) {
  returnvalue = retval;
  if (returnvalue) returnvalue->setParent(this);
}

class chillAST_Node *chillAST_ReturnStmt::constantFold() {
  if (returnvalue) returnvalue = returnvalue->constantFold();
  return this;
}


class chillAST_Node *chillAST_ReturnStmt::clone() {
  chillAST_Node *val = NULL;
  if (returnvalue) val = returnvalue->clone();
  chillAST_ReturnStmt *RS = new chillAST_ReturnStmt(val);
  RS->setParent(getParent());
  RS->isFromSourceFile = isFromSourceFile;
  if (filename) RS->filename = strdup(filename);
  return RS;
}


void chillAST_ReturnStmt::gatherVarDecls(vector<chillAST_VarDecl *> &decls) {
  if (returnvalue) returnvalue->gatherVarDecls(decls);
}


void chillAST_ReturnStmt::gatherScalarVarDecls(vector<chillAST_VarDecl *> &decls) {
  if (returnvalue) returnvalue->gatherScalarVarDecls(decls);
}


void chillAST_ReturnStmt::gatherArrayVarDecls(vector<chillAST_VarDecl *> &decls) {
  if (returnvalue) returnvalue->gatherArrayVarDecls(decls);
}


void chillAST_ReturnStmt::gatherDeclRefExprs(vector<chillAST_DeclRefExpr *> &refs) {
  if (returnvalue) returnvalue->gatherDeclRefExprs(refs);
}


void chillAST_ReturnStmt::gatherVarUsage(vector<chillAST_VarDecl *> &decls) {
  if (returnvalue) returnvalue->gatherVarUsage(decls);
}


chillAST_CallExpr::chillAST_CallExpr(chillAST_Node *c) { //, int numofargs, chillAST_Node **theargs ) {

  //fprintf(stderr, "chillAST_CallExpr::chillAST_CallExpr  callee type %s\n", c->getTypeString());
  callee = c;
  //callee->setParent( this ); // ??
  numargs = 0;
  grid = block = NULL;
}


void chillAST_CallExpr::addArg(chillAST_Node *a) {
  args.push_back(a);
  a->setParent(this);
  numargs += 1;
}

void chillAST_CallExpr::gatherArrayRefs(std::vector<chillAST_ArraySubscriptExpr *> &refs, bool writtento) {
  for (int i = 0; i < args.size(); i++) {
    args[i]->gatherArrayRefs(refs, writtento);
  }
}

void chillAST_CallExpr::gatherScalarRefs(std::vector<chillAST_DeclRefExpr *> &refs, bool writtento) {
  for (int i = 0; i < args.size(); i++) {
    args[i]->gatherScalarRefs(refs, writtento);
  }
}


void chillAST_CallExpr::gatherVarDecls(vector<chillAST_VarDecl *> &decls) {
  for (int i = 0; i < args.size(); i++) {
    args[i]->gatherVarDecls(decls);
  }
}


void chillAST_CallExpr::gatherScalarVarDecls(vector<chillAST_VarDecl *> &decls) {
  for (int i = 0; i < args.size(); i++) {
    args[i]->gatherScalarVarDecls(decls);
  }
}


void chillAST_CallExpr::gatherArrayVarDecls(vector<chillAST_VarDecl *> &decls) {
  for (int i = 0; i < args.size(); i++) {
    args[i]->gatherArrayVarDecls(decls);
  }
}


void chillAST_CallExpr::gatherDeclRefExprs(vector<chillAST_DeclRefExpr *> &refs) {
  for (int i = 0; i < args.size(); i++) {
    args[i]->gatherDeclRefExprs(refs);
  }
}

void chillAST_CallExpr::replaceVarDecls(chillAST_VarDecl *olddecl, chillAST_VarDecl *newdecl) {
  for (int i = 0; i < args.size(); i++) args[i]->replaceVarDecls(olddecl, newdecl);
}

void chillAST_CallExpr::gatherVarUsage(vector<chillAST_VarDecl *> &decls) {
  for (int i = 0; i < args.size(); i++) {
    args[i]->gatherVarUsage(decls);
  }
}


chillAST_Node *chillAST_CallExpr::constantFold() {
  numargs = args.size(); // wrong place for this
  for (int i = 0; i < numargs; i++) {
    args[i] = args[i]->constantFold();
  }
  return this;
}

chillAST_Node *chillAST_CallExpr::clone() {
  //fprintf(stderr, "chillAST_CallExpr::clone()\n");
  //print(0, stderr); fprintf(stderr, "\n");

  chillAST_CallExpr *CE = new chillAST_CallExpr(callee->clone());
  for (int i = 0; i < args.size(); i++) CE->addArg(args[i]->clone());
  CE->isFromSourceFile = isFromSourceFile;
  if (filename) CE->filename = strdup(filename);
  return CE;
}


chillAST_VarDecl::chillAST_VarDecl() {
  //fprintf(stderr, "0chillAST_VarDecl::chillAST_VarDecl()  %p\n", this);
  fprintf(stderr, "0chillAST_VarDecl::chillAST_VarDecl()\n");
  vartype = underlyingtype = varname = arraypart = arraypointerpart = arraysetpart = NULL;
  typedefinition = NULL;

  //fprintf(stderr, "setting underlying type NULL\n" );
  init = NULL;
  numdimensions = 0;
  arraysizes = NULL;
  parent = NULL;
  metacomment = NULL;

  vardef = NULL;
  isStruct = false;

  //insideAStruct = false;
  isAParameter = false;
  byreference = false;
  isABuiltin = false;
  isRestrict = isDevice = isShared = false; // fprintf(stderr, "RDS = false\n");
  knownArraySizes = false;
};


chillAST_VarDecl::chillAST_VarDecl(const char *t, const char *n, const char *a) {
  //fprintf(stderr, "1chillAST_VarDecl::chillAST_VarDecl( type %s, name %s, arraypart %s,  parent %p)  %p\n", t, n, a, par, this);
  fprintf(stderr, "1chillAST_VarDecl::chillAST_VarDecl( type %s, name %s, arraypart %s)\n", t, n, a);
  vartype = strdup(t);
  typedefinition = NULL;

  underlyingtype = parseUnderlyingType(vartype);
  //fprintf(stderr, "setting underlying type %s from %s\n",  underlyingtype, vartype );
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
  //fprintf(stderr, "arraypart len %d\n", strlen(a));
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

  fprintf(stderr, "3chillAST_VarDecl::chillAST_VarDecl( %s  %p struct ", nam, this);
  const char *type = astruct->getName();
  fprintf(stderr, "%s, name %s, arraypart %s parent ) %p\n", type, nam, array, this); // , par);

  vartype = strdup(type);

  // these always go together  ??
  vardef = astruct;// pointer to the thing that says what is inside the struct
  isStruct = true;  // ?? wrong if it's a union  ?? TODO

  //insideAStruct = false;
  //fprintf(stderr, "setting vardef of %s to %p\n", nam, vardef);

  underlyingtype = parseUnderlyingType(vartype);
  //fprintf(stderr, "setting underlying type %s from %s\n",  underlyingtype, vartype );
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
  fprintf(stderr, "4chillAST_VarDecl::chillAST_VarDecl( %s  typedef ", n);
  const char *type = tdd->getStructName();
  //fprintf (stderr, "%s, name %s, arraypart %s parent ) %p\n", type, n, a,this); // , par);
  typedefinition = tdd;
  vartype = strdup(type);
  underlyingtype = parseUnderlyingType(vartype);
  //fprintf(stderr, "setting underlying type %s from %s\n",  underlyingtype, vartype );
  varname = strdup(n);
  arraypart = strdup(a);
  arraypointerpart = arraysetpart = NULL;
  splitarraypart();

  init = NULL;
  numdimensions = 0;
  arraysizes = NULL;
  uniquePtr = NULL;

  knownArraySizes = false;
  //fprintf(stderr, "arraypart len %d\n", strlen(a));
  for (int i = 0; i < strlen(a); i++) {
    if (a[i] == '[') {
      numdimensions++;
      knownArraySizes = true;
    }
    if (!knownArraySizes && a[i] == '*') numdimensions++;
  }

  isStruct = tdd->isAStruct();

  //insideAStruct = false;

  vardef = NULL;


  isAParameter = false;
  byreference = false;
  isABuiltin = false;
  isRestrict = isDevice = isShared = false; // //fprintf(stderr, "RDS = false\n");
};


chillAST_VarDecl::chillAST_VarDecl(const char *t, const char *n, const char *a, void *ptr) {
  CHILL_DEBUG_PRINT("chillAST_VarDecl::chillAST_VarDecl( type %s, name %s, arraypart '%s' ) %p\n", t, n, a, this);

  vartype = strdup(t);
  typedefinition = NULL;
  underlyingtype = parseUnderlyingType(vartype);
  //fprintf(stderr, "setting underlying type %s from %s\n",  underlyingtype, vartype );
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

  //fprintf(stderr, "name arraypart len %d\n", strlen(a));
  //fprintf(stderr, "arraypart '%s'\n", arraypart);
  for (int i = 0; i < strlen(a); i++) {
    if (a[i] == '[') {
      numdimensions++;
      knownArraySizes = true;
    }
    if (!knownArraySizes && a[i] == '*') numdimensions++; // fails for  a[4000 * 4]
  }
  //if (0 == strlen(a) && numdimensions == 0) {
  //  for (int i=0; i<strlen(t); i++) {   // handle float * x
  //    if (t[i] == '[') numdimensions++;
  //    if (t[i] == '*') numdimensions++;
  //  }
  //}
  //fprintf(stderr, "2name %s numdimensions %d\n", n, numdimensions);




  // this is from ir_clang.cc ConvertVarDecl(), that got executed AFTER the vardecl was constructed. dumb
  int numdim = 0;
  //knownArraySizes = true;
  //if (index(vartype, '*')) knownArraySizes = false;  // float *a;   for example
  //if (index(arraypart, '*'))  knownArraySizes = false;

  // note: vartype here, arraypart in next code..    is that right?
  if (index(vartype, '*')) {
    for (int i = 0; i < strlen(vartype); i++) if (vartype[i] == '*') numdim++;
    //fprintf(stderr, "numd %d\n", numd);
    numdimensions = numdim;
  }

  if (index(arraypart, '[')) {  // JUST [12][34][56]  no asterisks
    char *dupe = strdup(arraypart);

    int len = strlen(arraypart);
    for (int i = 0; i < len; i++) if (dupe[i] == '[') numdim++;

    //fprintf(stderr, "numdim %d\n", numdim);

    numdimensions = numdim;
    int *as = (int *) malloc(sizeof(int *) * numdim);
    if (!as) {
      fprintf(stderr, "can't malloc array sizes in ConvertVarDecl()\n");
      exit(-1);
    }
    arraysizes = as; // 'as' changed later!


    char *ptr = dupe;
    //fprintf(stderr, "dupe '%s'\n", ptr);
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
          //fprintf(stderr, " not justmath because '%c'\n", c);
          justmath = false;
        }

      }

      //fprintf(stderr, "tmp '%s'\n", leak);
      if (justdigits) {
        int dim;
        sscanf(ptr, "%d", &dim);
        //fprintf(stderr, "dim %d\n", dim);
        *as++ = dim;
      } else {
        if (justmath) fprintf(stderr, "JUST MATH\n");
        fprintf(stderr, "need to evaluate %s, faking with hardcoded 16000\n", leak);
        *as++ = 16000; // temp TODO DFL
      }
      free(leak);

      ptr = index(ptr, ']');
      //fprintf(stderr, "bottom of loop, ptr = '%s'\n", ptr);
    }
    free(dupe);
    //for (int i=0; i<numdim; i++) {
    //  fprintf(stderr, "dimension %d = %d\n", i,  arraysizes[i]);
    //}

    //fprintf(stderr, "need to handle [] array to determine num dimensions\n");
    //exit(-1);
  }


  //insideAStruct = false;
  byreference = false;
  isABuiltin = false;
  isRestrict = isDevice = isShared = false; // fprintf(stderr, "RDS = false\n");

  //print(); printf("\n"); fflush(stdout);

  // currently this is bad, because a struct does not have a symbol table, so the
  // members of a struct are passed up to the func or sourcefile.
  CHILL_DEBUG_PRINT("LEAVING\n");
  //parent->print(); fprintf(stderr, "\n\n");


};

void chillAST_VarDecl::printName(int in, FILE *fp) {
  chillindent(in, fp);
  fprintf(fp, "%s", varname);
};

chillAST_RecordDecl *chillAST_VarDecl::getStructDef() {
  if (vardef) return vardef;
  if (typedefinition) return typedefinition->getStructDef();
  return NULL;
}


chillAST_CompoundStmt::chillAST_CompoundStmt() {
  //fprintf(stderr, "chillAST_CompoundStmt::chillAST_CompoundStmt() %p\n", this);
  parent = NULL;
  symbolTable = new chillAST_SymbolTable;
  typedefTable = new chillAST_TypedefTable;
};

void chillAST_CompoundStmt::replaceChild(chillAST_Node *old, chillAST_Node *newchild) {
  //fprintf(stderr, "chillAST_CompoundStmt::replaceChild( old %s, new %s)\n", old->getTypeString(), newchild->getTypeString() );
  vector<chillAST_Node *> dupe = children;
  int numdupe = dupe.size();
  int any = 0;

  for (int i = 0; i < numdupe; i++) {

    //fprintf(stderr, "\ni %d\n",i);
    //for (int j=0; j<numdupe; j++) {
    //  fprintf(stderr, "this 0x%x   children[%d/%d] = 0x%x type %s\n", this, j, children.size(), children[j], children[j]->getTypeString());
    //}


    if (dupe[i] == old) {
      //fprintf(stderr, "replacing child %d of %d\n", i, numdupe);
      //fprintf(stderr, "was \n"); print();
      children[i] = newchild;
      newchild->setParent(this);
      //fprintf(stderr, "is  \n");  print(); fprintf(stderr, "\n\n");
      // old->parent = NULL;
      any = 1;
    }
  }

  if (!any) {
    fprintf(stderr, "chillAST_CompoundStmt::replaceChild(), could not find old\n");
    exit(-1);
  }
}


void chillAST_CompoundStmt::loseLoopWithLoopVar(char *var) {
  //fprintf(stderr, "chillAST_CompoundStmt::loseLoopWithLoopVar( %s )\n", var);

  //fprintf(stderr, "CompoundStmt 0x%x has parent 0x%x  ", this, this->parent);
  //fprintf(stderr, "%s\n", parent->getTypeString());


  //fprintf(stderr, "CompoundStmt node has %d children\n", children.size());
  //fprintf(stderr, "before doing a damned thing, \n");
  //print();
  //dump(); fflush(stdout);
  //fprintf(stderr, "\n\n");

#ifdef DAMNED
                                                                                                                          for (int j=0; j<children.size(); j++) {
    fprintf(stderr, "j %d/%d  ", j, children.size());
    fprintf(stderr, "subnode %d 0x%x  ", j, children[j] );
    fprintf(stderr, "asttype %d  ", children[j]->getType());
    fprintf(stderr, "%s    ", children[j]->getTypeString());
    if (children[j]->isForStmt()) {
      chillAST_ForStmt *FS = ((chillAST_ForStmt *)  children[j]);
      fprintf(stderr, "for (");
      FS->init->print(0, stderr);
      fprintf(stderr, "; ");
      FS->cond->print(0, stderr);
      fprintf(stderr, "; ");
      FS->incr->print(0, stderr);
      fprintf(stderr, ")  with %d statements in body 0x%x\n",  FS->body->getNumChildren(), FS->body );
    }
    else fprintf(stderr, "\n");
  }
#endif


  vector<chillAST_Node *> dupe = children; // simple enough?
  for (int i = 0; i < dupe.size(); i++) {
    //for (int j=0; j<dupe.size(); j++) {
    //  fprintf(stderr, "j %d/%d\n", j, dupe.size());
    //  fprintf(stderr, "subnode %d %s    ", j, children[j]->getTypeString());
    //  if (children[j]->isForStmt()) {
    //    chillAST_ForStmt *FS = ((chillAST_ForStmt *)  children[j]);
    //    fprintf(stderr, "for (");
    //     FS->init->print(0, stderr);
    //    fprintf(stderr, "; ");
    //    FS->cond->print(0, stderr);
    //    fprintf(stderr, "; ");
    //    FS->incr->print(0, stderr);
    //    fprintf(stderr, ")  with %d statements in body 0x%x\n",  FS->body->getNumChildren(), FS->body );
    //}
    //else fprintf(stderr, "\n");
    //}

    //fprintf(stderr, "CompoundStmt 0x%x recursing to child %d/%d\n", this, i, dupe.size());
    dupe[i]->loseLoopWithLoopVar(var);
  }
  //fprintf(stderr, "CompoundStmt node 0x%x done recursing\n", this );
}

chillAST_Node *chillAST_CompoundStmt::constantFold() {
  //fprintf(stderr, "chillAST_CompoundStmt::constantFold()\n");
  for (int i = 0; i < children.size(); i++) children[i] = children[i]->constantFold();
  return this;
}


chillAST_Node *chillAST_CompoundStmt::clone() {
  chillAST_CompoundStmt *cs = new chillAST_CompoundStmt();
  for (int i = 0; i < children.size(); i++) cs->addChild(children[i]->clone());
  cs->setParent(getParent());
  cs->isFromSourceFile = isFromSourceFile;
  if (filename) cs->filename = strdup(filename);
  return cs;
}


void chillAST_CompoundStmt::gatherVarDecls(vector<chillAST_VarDecl *> &decls) {
  //fprintf(stderr, "chillAST_CompoundStmt::gatherVarDecls()\n");
  for (int i = 0; i < children.size(); i++) children[i]->gatherVarDecls(decls);
}


void chillAST_CompoundStmt::gatherScalarVarDecls(vector<chillAST_VarDecl *> &decls) {
  for (int i = 0; i < children.size(); i++) children[i]->gatherScalarVarDecls(decls);
}


void chillAST_CompoundStmt::gatherArrayVarDecls(vector<chillAST_VarDecl *> &decls) {
  for (int i = 0; i < children.size(); i++) children[i]->gatherArrayVarDecls(decls);
}


void chillAST_CompoundStmt::gatherDeclRefExprs(vector<chillAST_DeclRefExpr *> &refs) {
  for (int i = 0; i < children.size(); i++) children[i]->gatherDeclRefExprs(refs);
}


void chillAST_CompoundStmt::gatherVarUsage(vector<chillAST_VarDecl *> &decls) {
  for (int i = 0; i < children.size(); i++) children[i]->gatherVarUsage(decls);
}


void chillAST_CompoundStmt::gatherArrayRefs(std::vector<chillAST_ArraySubscriptExpr *> &refs, bool writtento) {
  for (int i = 0; i < children.size(); i++) children[i]->gatherArrayRefs(refs, 0);
}

void chillAST_CompoundStmt::gatherScalarRefs(std::vector<chillAST_DeclRefExpr *> &refs, bool writtento) {
  for (int i = 0; i < children.size(); i++) children[i]->gatherScalarRefs(refs, 0);
}

void chillAST_CompoundStmt::gatherStatements(std::vector<chillAST_Node *> &statements) {
  for (int i = 0; i < children.size(); i++) children[i]->gatherStatements(statements);
}


void chillAST_CompoundStmt::replaceVarDecls(chillAST_VarDecl *olddecl, chillAST_VarDecl *newdecl) {
  for (int i = 0; i < children.size(); i++) children[i]->replaceVarDecls(olddecl, newdecl);
}


bool chillAST_CompoundStmt::findLoopIndexesToReplace(chillAST_SymbolTable *symtab, bool forcesync) {

  // see how many elements we currently have
  int sofar = children.size();

  // make big enough to add a sync after each statement. wasteful. TODO
  // this prevents inserts happening at the forstmt::addSync() from causing a
  // reallocation, which screwsup the loop below here
  children.reserve(2 * sofar);
  //fprintf(stderr, "sofar %d   reserved %d\n", sofar, 2*sofar);

  bool force = false;
  for (int i = 0; i < children.size(); i++) {  // children.size() to see it gain each time
    if (children.size() > sofar) {
      //fprintf(stderr, "HEY! CompoundStmt::findLoopIndexesToReplace() noticed that children increased from %d to %d\n", sofar, children.size());
      sofar = children.size();
    }

    //fprintf(stderr, "compound child %d of type %s force %d\n", i, children[i]->getTypeString(), force );
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


chillAST_ParenExpr::chillAST_ParenExpr(chillAST_Node *sub) {
  subexpr = sub;
  subexpr->setParent(this);
}

void chillAST_ParenExpr::gatherArrayRefs(std::vector<chillAST_ArraySubscriptExpr *> &refs, bool writtento) {
  subexpr->gatherArrayRefs(refs, writtento);
}

void chillAST_ParenExpr::gatherScalarRefs(std::vector<chillAST_DeclRefExpr *> &refs, bool writtento) {
  subexpr->gatherScalarRefs(refs, writtento);
}


chillAST_Node *chillAST_ParenExpr::constantFold() {
  subexpr = subexpr->constantFold();
  return this;
}


chillAST_Node *chillAST_ParenExpr::clone() {
  chillAST_ParenExpr *PE = new chillAST_ParenExpr(subexpr->clone());
  PE->setParent(getParent());
  PE->isFromSourceFile = isFromSourceFile;
  if (filename) PE->filename = strdup(filename);
  return PE;
}

void chillAST_ParenExpr::gatherVarDecls(vector<chillAST_VarDecl *> &decls) {
  subexpr->gatherVarDecls(decls);
}


void chillAST_ParenExpr::gatherScalarVarDecls(vector<chillAST_VarDecl *> &decls) {
  subexpr->gatherScalarVarDecls(decls);
}


void chillAST_ParenExpr::gatherArrayVarDecls(vector<chillAST_VarDecl *> &decls) {
  subexpr->gatherArrayVarDecls(decls);
}


void chillAST_ParenExpr::gatherDeclRefExprs(vector<chillAST_DeclRefExpr *> &refs) {
  subexpr->gatherDeclRefExprs(refs);
}

void chillAST_ParenExpr::replaceVarDecls(chillAST_VarDecl *olddecl, chillAST_VarDecl *newdecl) {
  subexpr->replaceVarDecls(olddecl, newdecl);
}

void chillAST_ParenExpr::gatherVarUsage(vector<chillAST_VarDecl *> &decls) {
  subexpr->gatherVarUsage(decls);
}


chillAST_Sizeof::chillAST_Sizeof(char *athing) {
  thing = strdup(athing); // memory leak
}

void chillAST_Sizeof::gatherArrayRefs(std::vector<chillAST_ArraySubscriptExpr *> &refs, bool writtento) {}

void chillAST_Sizeof::gatherScalarRefs(std::vector<chillAST_DeclRefExpr *> &refs, bool writtento) {}

chillAST_Node *chillAST_Sizeof::constantFold() {
  return this;
}

chillAST_Node *chillAST_Sizeof::clone() {
  chillAST_Sizeof *SO = new chillAST_Sizeof(thing);
  SO->setParent(getParent());
  SO->isFromSourceFile = isFromSourceFile;
  if (filename) SO->filename = strdup(filename);
  return SO;
}

void chillAST_Sizeof::gatherVarDecls(vector<chillAST_VarDecl *> &decls) {  // TODO
}


void chillAST_Sizeof::gatherScalarVarDecls(vector<chillAST_VarDecl *> &decls) {  // TODO
}


void chillAST_Sizeof::gatherArrayVarDecls(vector<chillAST_VarDecl *> &decls) {  // TODO
}


void chillAST_Sizeof::gatherDeclRefExprs(vector<chillAST_DeclRefExpr *> &refs) {
  // TODO
}


void chillAST_Sizeof::gatherVarUsage(vector<chillAST_VarDecl *> &decls) {
}


void insertNewDeclAtLocationOfOldIfNeeded(chillAST_VarDecl *newdecl, chillAST_VarDecl *olddecl) {
  //fprintf(stderr, "insertNewDeclAtLocationOfOldIfNeeded( new 0x%x  old 0x%x\n", newdecl, olddecl );

  if (newdecl == NULL || olddecl == NULL) {
    fprintf(stderr, "chill_ast.cc insertNewDeclAtLocationOfOldIfNeeded() NULL decl\n");
    exit(-1);
  }

  if (newdecl == olddecl) return;

  newdecl->vartype = strdup(olddecl->vartype);

  chillAST_Node *newparent = newdecl->parent;
  chillAST_Node *oldparent = olddecl->parent;
  //fprintf(stderr, "newparent 0x%x   oldparent 0x%x\n", newparent, oldparent );
  if (newparent == oldparent) return;

  if (newparent != NULL)
    //fprintf(stderr, "chill_ast.cc insertNewDeclAtLocationOfOldIfNeeded() new decl already has parent??  probably wrong\n");
    newdecl->parent = oldparent;  // will be true soon

  // find actual location of old decl and insert new one there
  //fprintf(stderr, "oldparent is of type %s\n", oldparent->getTypeString()); // better be compoundstmt ??
  vector<chillAST_Node *> *children = oldparent->getChildren();

  int numchildren = children->size();
  //fprintf(stderr, "oldparent has %d children\n", numchildren);

  if (numchildren == 0) {
    fprintf(stderr,
            "chill_ast.cc insertNewDeclAtLocationOfOldIfNeeded() impossible number of oldparent children (%d)\n",
            numchildren);
    exit(-1);
  }

  bool newalreadythere = false;
  int index = -1;
  //fprintf(stderr, "olddecl is 0x%x\n", olddecl);
  //fprintf(stderr, "I know of %d variables\n", numchildren);
  for (int i = 0; i < numchildren; i++) {
    chillAST_Node *child = oldparent->getChild(i);
    //fprintf(stderr, "child %d @ 0x%x is of type %s\n", i, child, child->getTypeString());
    if ((*children)[i] == olddecl) {
      index = i;
      //fprintf(stderr, "found old decl at index %d\n", index);
    }
    if ((*children)[i] == newdecl) {
      newalreadythere = true;
      //fprintf(stderr, "new already there @ index %d\n", i);
    }
  }
  if (index == -1) {
    fprintf(stderr, "chill_ast.cc insertNewDeclAtLocationOfOldIfNeeded() can't find old decl for %s\n",
            olddecl->varname);
    exit(-1);
  }

  if (!newalreadythere) oldparent->insertChild(index, newdecl);

}


void gatherVarDecls(vector<chillAST_Node *> &code, vector<chillAST_VarDecl *> &decls) {
  //fprintf(stderr, "gatherVarDecls()\n");

  int numcode = code.size();
  //fprintf(stderr, "%d top level statements\n", numcode);
  for (int i = 0; i < numcode; i++) {
    chillAST_Node *statement = code[i];
    statement->gatherVarDecls(decls);
  }

}


void gatherVarUsage(vector<chillAST_Node *> &code, vector<chillAST_VarDecl *> &decls) {
  //fprintf(stderr, "gatherVarUsage()\n");

  int numcode = code.size();
  //fprintf(stderr, "%d top level statements\n", numcode);
  for (int i = 0; i < numcode; i++) {
    chillAST_Node *statement = code[i];
    statement->gatherVarUsage(decls);
  }

}


chillAST_IfStmt::chillAST_IfStmt() {
  cond = thenpart = elsepart = NULL;
  isFromSourceFile = true; // default
  filename = NULL;
}

chillAST_IfStmt::chillAST_IfStmt(chillAST_Node *c, chillAST_Node *t, chillAST_Node *e) {
  cond = c;
  if (cond) cond->setParent(this);
  thenpart = t;
  if (thenpart) thenpart->setParent(this);
  elsepart = e;
  if (elsepart) elsepart->setParent(this);
}

void chillAST_IfStmt::gatherVarDecls(vector<chillAST_VarDecl *> &decls) {
  if (cond) cond->gatherVarDecls(decls);
  if (thenpart) thenpart->gatherVarDecls(decls);
  if (elsepart) elsepart->gatherVarDecls(decls);
}


void chillAST_IfStmt::gatherScalarVarDecls(vector<chillAST_VarDecl *> &decls) {
  if (cond) cond->gatherScalarVarDecls(decls);
  if (thenpart) thenpart->gatherScalarVarDecls(decls);
  if (elsepart) elsepart->gatherScalarVarDecls(decls);
}


void chillAST_IfStmt::gatherArrayVarDecls(vector<chillAST_VarDecl *> &decls) {
  if (cond) cond->gatherArrayVarDecls(decls);
  if (thenpart) thenpart->gatherArrayVarDecls(decls);
  if (elsepart) elsepart->gatherArrayVarDecls(decls);
}


void chillAST_IfStmt::gatherDeclRefExprs(vector<chillAST_DeclRefExpr *> &refs) {
  if (cond) cond->gatherDeclRefExprs(refs);
  if (thenpart) thenpart->gatherDeclRefExprs(refs);
  if (elsepart) elsepart->gatherDeclRefExprs(refs);
}


void chillAST_IfStmt::gatherVarUsage(vector<chillAST_VarDecl *> &decls) {
  if (cond) cond->gatherVarUsage(decls);
  if (thenpart) thenpart->gatherVarUsage(decls);
  if (elsepart) elsepart->gatherVarUsage(decls);
}


void chillAST_IfStmt::gatherArrayRefs(std::vector<chillAST_ArraySubscriptExpr *> &refs, bool writtento) {
  cond->gatherArrayRefs(refs, 0);  // 0 ??
  thenpart->gatherArrayRefs(refs, 0);  // 0 ??
  if (elsepart) elsepart->gatherArrayRefs(refs, 0);  // 0 ??
}

void chillAST_IfStmt::gatherScalarRefs(std::vector<chillAST_DeclRefExpr *> &refs, bool writtento) {
  cond->gatherScalarRefs(refs, 0);  // 0 ??
  thenpart->gatherScalarRefs(refs, 0);  // 0 ??
  if (elsepart) elsepart->gatherScalarRefs(refs, 0);  // 0 ??
}


chillAST_Node *chillAST_IfStmt::constantFold() {
  if (cond) cond = cond->constantFold();
  if (thenpart) thenpart = thenpart->constantFold();
  if (elsepart) elsepart = elsepart->constantFold();
  return this;
}

void chillAST_IfStmt::gatherStatements(std::vector<chillAST_Node *> &statements) {

  //print(); printf("\n"); fflush(stdout);
  thenpart->gatherStatements(statements);
  //fprintf(stderr, "ifstmt, after then, %d statements\n", statements.size());
  if (elsepart) {
    //fprintf(stderr, "there is an elsepart of type %s\n", elsepart->getTypeString());
    elsepart->gatherStatements(statements);
  }
  //fprintf(stderr, "ifstmt, after else, %d statements\n", statements.size());
}


chillAST_Node *chillAST_IfStmt::clone() {
  chillAST_Node *c, *t, *e;
  c = t = e = NULL;
  if (cond) c = cond->clone(); // has to be one, right?
  if (thenpart) t = thenpart->clone();
  if (elsepart) e = elsepart->clone();

  chillAST_IfStmt *IS = new chillAST_IfStmt(c, t, e);
  IS->setParent(getParent());
  IS->isFromSourceFile = isFromSourceFile;
  if (filename) IS->filename = strdup(filename);
  return IS;
}

bool chillAST_IfStmt::findLoopIndexesToReplace(chillAST_SymbolTable *symtab, bool forcesync) {
  thenpart->findLoopIndexesToReplace(symtab);
  elsepart->findLoopIndexesToReplace(symtab);
  return false; // ??
}


chillAST_Node *lessthanmacro(chillAST_Node *left, chillAST_Node *right) {

  chillAST_ParenExpr *lp1 = new chillAST_ParenExpr(left);
  chillAST_ParenExpr *rp1 = new chillAST_ParenExpr(right);
  chillAST_BinaryOperator *cond = new chillAST_BinaryOperator(lp1, "<", rp1);

  chillAST_ParenExpr *lp2 = new chillAST_ParenExpr(left);
  chillAST_ParenExpr *rp2 = new chillAST_ParenExpr(right);

  chillAST_TernaryOperator *t = new chillAST_TernaryOperator("?", cond, lp2, rp2);

  return t;
}


// look for function declaration with a given name, in the tree with root "node"
void findFunctionDeclRecursive(chillAST_Node *node, const char *procname, vector<chillAST_FunctionDecl *> &funcs) {
  //fprintf(stderr, "findmanually()                CHILL AST node of type %s\n", node->getTypeString());

  if (node->isFunctionDecl()) {
    char *name = ((chillAST_FunctionDecl *) node)->functionName; // compare name with desired name
    //fprintf(stderr, "node name 0x%x  ", name);
    //fprintf(stderr, "%s     procname ", name);
    //fprintf(stderr, "0x%x  ", procname);
    //fprintf(stderr, "%s\n", procname);
    if (!strcmp(name, procname)) {
      //fprintf(stderr, "found procedure %s\n", procname );
      funcs.push_back((chillAST_FunctionDecl *) node);  // this is it
      // quit recursing. probably not correct in some horrible case
      return;
    }
    //else fprintf(stderr, "this is not the function we're looking for\n");
  }


  // this is where the children can be used effectively.
  // we don't really care what kind of node we're at. We just check the node itself
  // and then its children is needed.

  int numc = node->children.size();
  fprintf(stderr, "(top)node has %d children\n", numc);

  for (int i = 0; i < numc; i++) {
    if (node->isSourceFile()) {
      fprintf(stderr, "node of type %s is recursing to child %d of type %s\n", node->getTypeString(), i,
              node->children[i]->getTypeString());
      if (node->children[i]->isFunctionDecl()) {
        chillAST_FunctionDecl *fd = (chillAST_FunctionDecl *) node->children[i];
        fprintf(stderr, "child %d is functiondecl %s\n", i, fd->functionName);
      }
    }
    findFunctionDeclRecursive(node->children[i], procname, funcs);

  }
  return;
}


chillAST_FunctionDecl *findFunctionDecl(chillAST_Node *node, const char *procname) {
  vector<chillAST_FunctionDecl *> functions;
  findFunctionDeclRecursive(node, procname, functions);

  if (functions.size() == 0) {
    fprintf(stderr, "could not find function named '%s'\n", procname);
    exit(-1);
  }

  if (functions.size() > 1) {
    fprintf(stderr, "oddly, found %d functions named '%s'\n", functions.size(), procname);
    fprintf(stderr, "I am unsure what to do\n");

    for (int f = 0; f < functions.size(); f++) {
      fprintf(stderr, "function %d  %p   %s\n", f, functions[f], functions[f]->functionName);
    }
    exit(-1);
  }

  //fprintf(stderr, "found the procedure named %s\n", procname);
  return functions[0];
}


chillAST_SymbolTable *addSymbolToTable(chillAST_SymbolTable *st, chillAST_VarDecl *vd) // definition
{
  chillAST_SymbolTable *s = st;
  if (!s) s = new chillAST_SymbolTable;

  int tablesize = s->size();

  for (int i = 0; i < tablesize; i++) {
    if ((*s)[i] == vd) {
      //fprintf(stderr, "the exact same symbol, not just the same name, was already there\n");
      return s; // already there
    }
  }

  for (int i = 0; i < tablesize; i++) {
    //fprintf(stderr, "name %s vs name %s\n", (*s)[i]->varname, vd->varname);
    if (!strcmp((*s)[i]->varname, vd->varname)) {
      //fprintf(stderr, "symbol with the same name was already there\n");
      return s; // already there
    }
  }

  //fprintf(stderr, "adding %s %s to a symbol table that didn't already have it\n", vd->vartype, vd->varname);

  //printf("before:\n");
  //printSymbolTable( s ); fflush(stdout);

  s->push_back(vd); // add it

  //printf("after:\n");
  //printSymbolTable( s ); fflush(stdout);
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

