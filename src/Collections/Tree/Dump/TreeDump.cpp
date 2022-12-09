#include "Tree.h"
#include "TreeDump.h"

#include <stdio.h>

#include "SystemLike.h"

#pragma GCC diagnostic ignored "-Wsign-compare"

#define SEPARATOR "=========================================================="

const char *const ERROR_MESSAGE[] =
  {
    "Pointer to tree is nullptr",
  };

static void printCallInfo(
                          const char *fileName,
                          const char *functionName,
                          int line,
                          FILE *file
                          );

static void printError(unsigned error, FILE *file);

static void printFields(const db::Tree *tree, FILE *file);

static void printData(const db::Tree *tree, FILE *file);

void db::do_dumpTree(
                 const db::Tree *tree,
                 unsigned error,
                 FILE *file,
                 const char *fileName,
                 const char *functionName,
                 int line,
                 const char *message,
                 ...
                 )
{
  if (!file)
    file = stdout;

  fprintf(file, "<h2>");

  va_list args = {};

  va_start(args, message);

  vfprintf(file, message, args);

  va_end(args);

  fprintf(file, "</h2>\n");

  fprintf(file, "Tree[%p] ", (const void *)tree);

  if (isPointerCorrect(tree))
      printCallInfo(fileName, functionName, line, file);

  printError(error, file);

  if (isPointerCorrect(tree))
    {
      printFields(tree, file);

      printData(tree, file);
    }

  fputc('\n', file);
}


static void printCallInfo(
                          const char *fileName,
                          const char *functionName,
                          int line,
                          FILE *file
                          )
{
  fprintf(
          file,
          "%s at %s(%d):\n",
          isPointerCorrect(functionName) ? functionName : "null",
          isPointerCorrect(fileName)     ? fileName     : "null",
          line
          );
}

static void printError(unsigned error, FILE *file)
{
  if (!error)
    {
      fprintf(file, "No error\n");

      return;
    }

  fprintf(file, "%s\n", SEPARATOR);

  fprintf(file, "<font color = red />Erors:\n");

  for (int i = 0; i < db::TREE_ERRORS_COUNT; ++i)
    if ((error >> i) & 0x01)
      fprintf(file, "%s,\n", ERROR_MESSAGE[i]);

  fprintf(file, "<font color = black />%s\n", SEPARATOR);
}

static void printFields(const db::Tree *tree, FILE *file)
{
  fprintf(file, "%s\n", SEPARATOR);

  fprintf(file, "Node *root = %p;\n", (const void *)tree->root);

  fprintf(file, "size_t size = %zu;\n", tree->size);

  fprintf(file, "%s\n", SEPARATOR);
}

static void printData(const db::Tree *tree, FILE *file)
{
  const char *image = createImage(tree, true);

  fprintf(file, "<hr>\n");

  fprintf(file, "<!%s>", SEPARATOR);

  fprintf(file, "<image src=../%s />", image);

  fprintf(file, "<hr>\n");

  fprintf(file, "<!%s>", SEPARATOR);
}

