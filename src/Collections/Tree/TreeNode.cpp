#include "Tree.h"

#include <stdlib.h>
#include <string.h>
#include "Assert.h"

#pragma GCC diagnostic ignored "-Wunused-parameter"

#define ERROR(...)                              \
  do                                            \
    {                                           \
      if (error)                                \
        *error = -1;                            \
                                                \
      return __VA_ARGS__;                       \
    } while (0)

db::TreeNode *db::createNode(treeValue_t value, type_t type, int *error)
{
  db::TreeNode *node = (db::TreeNode *)calloc(1, sizeof(db::TreeNode));

  if (!node)
    ERROR(nullptr);

  node->type = type;

  if (type == db::type_t::NAME || type == db::type_t::STRING)
    {
      char *buffer = (char *)calloc(MAX_NAME_SIZE, sizeof(char));
      if (!buffer)
        {
          free(node);
          ERROR(nullptr);
        }

      node->value.name = buffer;

      strncpy(buffer, value.name, MAX_NAME_SIZE);
    }
  else
    node->value = value;

  return node;
}

db::TreeNode *db::createNode(db::treeValue_t value, db::type_t type, db::TreeNode *parent, int leftChild, int *error)
{
  assert(parent);

  db::TreeNode *node = createNode(value, type, error);

  if (!node)
    ERROR(nullptr);

  return setParent(node, parent, leftChild, error);
}

db::TreeNode *db::createNode(db::treeValue_t value, db::type_t type, db::TreeNode *left, db::TreeNode *right, int *error)
{
  int errorCode = 0;

  db::TreeNode *node = createNode(value, type, &errorCode);

  if (errorCode)
    ERROR(nullptr);

  node->left  = left;
  node->right = right;

  if (left ) left ->parent = node;
  if (right) right->parent = node;

  return node;
}

db::TreeNode *db::createNode(const db::TreeNode *original, int *error)
{
  if (!original)
    ERROR(nullptr);

  int errorCode = 0;

  db::TreeNode *node =
    createNode(
               original->value,
               original->type,
               original->left ? db::createNode(original->left) : nullptr,
               original->right ? db::createNode(original->right) : nullptr,
               &errorCode
               );

  if (errorCode)
    ERROR(nullptr);

  return node;
}

db::TreeNode *db::setParent(db::TreeNode *child, db::TreeNode *parent, int leftChild, int *error)
{
  assert(child);
  assert(parent);

  child->parent = parent;

  if (leftChild)
    parent->left  = child;
  else
    parent->right = child;

  return child;
}

db::TreeNode *db::setChildren(db::TreeNode *parent, db::TreeNode *leftChildren, db::TreeNode *rightChildren, int *error)
{
  assert(parent);

  parent->left  =  leftChildren;
  parent->right = rightChildren;

  if ( leftChildren) db::setParent( leftChildren, parent, 1, error);
  if (rightChildren) db::setParent(rightChildren, parent, 0, error);

  return parent;
}

void db::removeNode(db::TreeNode *node, int *error)
{
  assert(node);

  if (node->left)
    removeNode(node->left);

  if (node->right)
    removeNode(node->right);

  if (node->type == db::type_t::NAME)
      free(node->value.name);

  if (node->type == db::type_t::STRING)
    free(node->value.string);

  free(node);
}
