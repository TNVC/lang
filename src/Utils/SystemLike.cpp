#include "SystemLike.h"

#include <malloc.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

#pragma GCC diagnostic ignored "-Wcast-qual"

//const int TEMP_FILE_DESCRIPTOR = mkstemp("/temp/TEMP_FILE_DESCRIPTOR_FOR_SYSTEMLIKE_H");

void *recalloc(void *pointer, size_t elements, size_t elementSize)
{
  size_t oldSize = malloc_usable_size(pointer);

  void *newPointer = realloc(pointer, elements * elementSize);

  if (isPointerCorrect(newPointer) && oldSize < elements * elementSize)
    memset((char *)newPointer + oldSize, 0, elements*elementSize - oldSize);

  return newPointer;
}

int isPointerWriteCorrect(const void *pointer)
{
  if (!pointer)
    return 0;

  //  char ch = *(const char *)pointer;

  //int result = read(TEMP_FILE_DESCRIPTOR, (void *)pointer, 1) != -1;

  //if (!result)
  //    *(char *)pointer = ch;

  return 1;
}

int isPointerReadCorrect(const void *pointer)
{
  if (!pointer)
    return 0;

  return 1;//write(TEMP_FILE_DESCRIPTOR, pointer, 0) != -1;
}

int isPointerCorrect(const void *pointer)
{
  return isPointerWriteCorrect(pointer) && isPointerReadCorrect(pointer);
}

size_t getFileSize(const char *fileName)
{
  if (!isPointerCorrect(fileName))
    return 0;

  struct stat temp = {};

  stat(fileName, &temp);

  return (size_t)temp.st_size;
}

int isFileExists(const char *fileName)
{
  if (!isPointerCorrect(fileName))
    return 0;

  struct stat temp = {};

  if (stat(fileName, &temp) == -1)
    return 0;

  return 1;
}
