#include "Fiofunctions.h"

#include <stdio.h>
#include <stdlib.h>
#include "SystemLike.h"
#include "Assert.h"

size_t readFile(char **buffer, const char *filename)
{
  if (!isPointerCorrect(buffer) || !isPointerReadCorrect(filename))
    return (size_t)FIOFUNCTIONS_INCORRECT_ARGUMENTS;

  FILE *fileptr = fopen(filename, "r");

  if (!isPointerCorrect(fileptr))
    return (size_t)FIOFUNCTIONS_FAIL_TO_OPEN;


  size_t size = getFileSize(filename);

  *buffer = (char *)calloc(size + 1, sizeof(char));

  if (!isPointerCorrect(*buffer))
    {
      fclose(fileptr);

      return (size_t)FIOFUNCTIONS_OUT_OF_MEM;
    }

  if (fread(*buffer, sizeof(char), size, fileptr) != size)
    {
      fclose(fileptr);

      free(*buffer);

      return (size_t)FIOFUNCTIONS_OUT_OF_MEM;
    }

  fclose(fileptr);

  return size;
}

size_t readBin(void *buffer, size_t elementSize, size_t size, FILE *filePtr)
{
  assert(buffer);
  assert(filePtr);

  return fread(buffer, elementSize, size, filePtr);
}
