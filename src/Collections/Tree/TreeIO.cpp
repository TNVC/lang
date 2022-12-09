#include "Tree.h"
#include "DSL.h"

#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include "Error.h"
#include "Assert.h"
#include "ErrorHandler.h"

#define FAIL(...)                               \
  do                                            \
    {                                           \
      if (fail) *fail = true;                   \
                                                \
      return __VA_ARGS__;                       \
    } while (0)

const int MAX_NAME_SIZE = 256;
static_assert(MAX_NAME_SIZE > 0);

const int MAX_LEXEME_SIZE = 48;
static_assert(MAX_LEXEME_SIZE >= db::MAX_NAME_SIZE);

static char *toString(const db::treeValue_t value, db::type_t type);

static void printNode(const db::TreeNode *node, FILE *file, int *error = nullptr);

void db::saveTree(const db::Tree *tree, FILE *file, int *error)
{
  if (!file) ERROR();

  if(tree->root)
    {
      printNode(tree->root, file);

      putc('\n', file);
    }
}

static void printNode(const db::TreeNode *node, FILE *file, int *error)
{
  if (!node) ERROR();
  assert(node);
  assert(file);

  fprintf(file, "(");

  if (Left ) printNode(Left , file);

  fprintf(file, "%s", toString(node->value, node->type));

  if (Right) printNode(Right, file);

  fprintf(file, ")");
}

static char *toString(const db::treeValue_t value, db::type_t type)
{
  static char buffer[MAX_LEXEME_SIZE] = "";

  memset(buffer, 0, MAX_LEXEME_SIZE);

  switch (type)
    {
    case db::type_t::STATEMENT:
      sprintf(
              buffer,
              " %s ",
              db::STATEMENT_NAMES[(int)value.statement]); break;
    case db::type_t::NAME:
      sprintf(buffer, "%s", value.name);                  break;
    case db::type_t::NUMBER:
      sprintf(buffer, "%lg", value.number);               break;
    case db::type_t::STRING:
      sprintf(buffer, "\"%s\"", value.string);            break;
    default:
      sprintf(buffer, "UNKNOWN TYPE!!");                  break;
    }

  return buffer;
}
