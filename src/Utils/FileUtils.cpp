#include "FileUtils.h"

#include <malloc.h>
#include <string.h>
#include "Assert.h"

FILE *openFile(const char *fileName, const char *fileMode)
{
  assert(fileName);
  assert(fileMode);

  size_t size = strlen(fileName) + strlen(DEFAULT_DIRECTORY) + 1;

  char *fullName = (char *)calloc(size, sizeof(char));

  if (!fullName)
    return nullptr;

  strcat(fullName, DEFAULT_DIRECTORY);
  strcat(fullName, fileName);

  FILE *file = fopen(fullName, fileMode);

  free(fullName);

  return file;
}
