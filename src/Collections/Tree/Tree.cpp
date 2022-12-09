#include "Tree.h"
#include "TreeDump.h"

#include "Assert.h"

static const db::TreeNode *findNode(const db::TreeNode *node, const db::treeValue_t value, db::type_t type);

unsigned db::validateTree(const db::Tree *tree)
{
  if (!tree)
    return db::TREE_NULLPTR;

  unsigned error = 0;

  return error;
}

void db::createTree(db::Tree *tree, int *error)
{
  if (!tree)
    ERROR();

  tree->root = nullptr;
  tree->size = 0;

  CHECK_VALID(tree, error);
}

void db::destroyTree(db::Tree *tree, int *error)
{
  CHECK_VALID(tree, error);

  if (tree->root)
    db::removeNode(tree->root, error);

  tree->root = nullptr;
  tree->size = 0;
}

const db::TreeNode *db::findElement(const db::Tree *tree, db::treeValue_t value, db::type_t type, int isList, int *error)
{
  CHECK_VALID(tree, error, 0);

  if (!validateValue(value, type))
    ERROR(0);

  const db::TreeNode *result = nullptr;

  if (tree->root)
    result = findNode(tree->root, value, type);

  if (!result)
    return nullptr;
  else
    {
      if (isList)
        return (!(result->left || result->right) ?  result : nullptr);
      else
        return result;
    }
}

static const db::TreeNode *findNode(const db::TreeNode *node, const db::treeValue_t value, db::type_t type)
{
  assert(node);
  assert(validateValue(value, type));

  const db::TreeNode *result = nullptr;

  if (node->left)
    {
      result = findNode(node->left,  value, type);

      if (result)
        return result;
    }

  if (node->right)
    {
      result = findNode(node->right,  value, type);

      if (result)
        return result;
    }

  if (node->type == type)
    return (!db::compareValues(node->value, value) ? node : nullptr);

  return nullptr;
}
