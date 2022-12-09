#include <stdio.h>
#include "SystemLike.h"
#include "ElementFunctions.h"

#pragma GCC diagnostic ignored "-Wunused-parameter"

int printElement(int element, FILE *filePtr)
{
  if (!isPointerCorrect(filePtr))
    return -1;

  return fprintf(filePtr, "%d", element);
}

int elementLength(int value)
{
  int charsNum = !(value > 0);

  value = value * !charsNum - value * charsNum;
  while (value > 0)
    {
      ++charsNum;

      value /= 10;
    }

  return charsNum;
}

int maxElementLength(int element)
{
  return 12;
}

int getPoison(int element)
{
  return (int)0xDED00DED;
}

int isPoison(int element)
{
  return (int)0xDED00DED == element;
}

int printElement(db::VarTable *element, FILE *filePtr)
{
  return fprintf(filePtr, "~");
}

int elementLength(db::VarTable *element)
{
  return 1;
}

int maxElementLength(db::VarTable *element)
{
  return 1;
}

db::VarTable *getPoison(db::VarTable *element)
{
  return nullptr;
}

int isPoison(db::VarTable *element)
{
  return nullptr == element;
}
