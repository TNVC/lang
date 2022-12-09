#include "GarbageCollector.h"

#include <stdlib.h>

/// Activate GB
/// @return Index to last push element in SequenceForFree
static int initCollector();

/// Free() all elements in SequenceForFree
static void freeResources();

/// Array with pointer which will free() in freeResources()
static void *SequenceForFree[MAX_LENGTH] = {};

/// Last push element in SequenceForFree
static int LastElementIndex = initCollector();

static int initCollector()
{
  atexit(freeResources);

  return -1;
}

static void freeResources()
{
  for (int i = 0; i <= LastElementIndex; ++i)
    free(SequenceForFree[i]);
}

int addElementForFree(void *pointer)
{
  if (LastElementIndex < MAX_LENGTH)
    SequenceForFree[++LastElementIndex] = pointer;
  else
    return SEQUENCE_IS_FULL;

  return 0;
}
