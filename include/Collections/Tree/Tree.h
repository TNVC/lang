#pragma once

#include <stddef.h>
#include <stdio.h>
#include <stdarg.h>
#include "Tokens.h"
#include <math.h>

namespace db {

  enum class type_t {
    STATEMENT,
    NAME,
    NUMBER,
    STRING,
  };

  typedef char  *name_t;
  typedef double number_t;
  typedef char  *string_t;

  const int MAX_NAME_SIZE = 32;

  union treeValue_t {
    statement_t statement;
    name_t      name;
    number_t    number;
    string_t    string;
  };

  struct TreeNode {
    type_t       type;
    treeValue_t  value;
    PositionInfo position;
    TreeNode   *parent;
    TreeNode   *left;
    TreeNode   *right;
  };

  struct Tree {
    TreeNode *root;
    size_t size;
  };

  typedef TreeNode *Token;
  typedef Tree    Grammar;

  enum TreeError {
    TREE_NULLPTR = 0x01 << 0,
  };

  const int TREE_ERRORS_COUNT = 1;

  const double DOUBLE_ERROR = 1e-10;

  inline bool compareNumber(number_t first, number_t second)
  {
    return fabs(first - second) < DOUBLE_ERROR;
  }

  inline bool compareValues(const treeValue_t &first, const treeValue_t &second)
  {
    return (first.statement == second.statement) &&
      (first.name == second.name) &&
      (compareNumber(first.number, second.number));
  }

  inline bool validateValue(const treeValue_t &value, type_t type)
  {
    if (type == type_t::NAME)
      return value.name;
    return true;
  }

  TreeNode *createNode(treeValue_t value, type_t type, int *error = nullptr);

  TreeNode *createNode(treeValue_t value, type_t type, TreeNode *parent, int leftChild = true, int *error = nullptr);

  TreeNode *createNode(treeValue_t value, type_t type, TreeNode *left, TreeNode *right, int *error = nullptr);

  TreeNode *createNode(const TreeNode *original, int *error = nullptr);

  TreeNode *setParent(TreeNode *child, TreeNode *parent, int leftChild = true, int *error = nullptr);

  TreeNode *setChildren(TreeNode *parent, TreeNode *leftChildren, TreeNode *rightChildren, int *error = nullptr);

  void removeNode(TreeNode *node, int *error = nullptr);


  unsigned validateTree(const Tree *tree);

  void createTree(Tree *tree, int *error = nullptr);

  void destroyTree(Tree *tree, int *error = nullptr);

  void addElement(TreeNode *node, treeValue_t value, int leftChild, int *error = nullptr);

  const TreeNode *findElement(const Tree *tree, treeValue_t value, type_t type, int isList = false, int *error = nullptr);

#define dumpTree(TREE, ERROR, FILE)                                 \
  do_dumpTree(TREE, ERROR, FILE, __FILE__, __func__, __LINE__, "")

#define dumpTreeWithMessage(TREE, ERROR, FILE, MESSAGE, ...)            \
  do_dumpTree(TREE, ERROR, FILE, __FILE__, __func__, __LINE__, MESSAGE __VA_OPT__(,) __VA_ARGS__)

  void do_dumpTree(
                   const Tree *tree,
                   unsigned error,
                   FILE *file,
                   const char* fileName,
                   const char* functionName,
                   int line,
                   const char *message,
                   ...
                   );

  void saveTree(const Tree *tree, FILE *file, int *error = nullptr);

  void loadTree(Tree *tree, FILE *file, int *error = nullptr);
  void loadTree(
                Tree *tree,
                const char *fileName,
                int *error = nullptr
                );
}
