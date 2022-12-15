#pragma once

#include "StringPool.h"
#include "Tree.h"

namespace db {

  struct Variable {
    const char *name;
    int number;
    bool isConst;
    bool isGlobal;
  };

  struct VarTable {
    Variable *table;
    size_t capacity;
    size_t size;

    VarTable &operator=(const VarTable &original) = delete;
  };

  struct Function {
    const char *name;
    bool hasReturn;
    Token token;
  };

  struct FunTable {
    Function *table;
    size_t capacity;
    size_t size;

    FunTable &operator=(const FunTable &original) = delete;
  };

}
