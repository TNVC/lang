#include "GenerateName.h"

#include <stdio.h>
#include <time.h>
#include <string.h>
#include <ctype.h>
#include <malloc.h>

const size_t MAX_HASH_SIZE = 16;

static char *getDataString();
static char *getHashString();

char *generateName(
                   const char *prefix,
                   const char *suffix,
                   const char * firstSeparator,
                   const char *secondSeparator
                  )
{
  if (!prefix) prefix = DEFAULT_PREFIX;
  if (!suffix) prefix = DEFAULT_SUFFIX;
  if (! firstSeparator)  firstSeparator = DEFAULT_SEPARATOR;
  if (!secondSeparator) secondSeparator = DEFAULT_SEPARATOR;

  char *dataString = getDataString();
  char *hashString = getHashString();

  size_t size =
    strlen(prefix)              +
    strlen( firstSeparator)     +
    strlen(suffix)              +
    strlen(secondSeparator)     +
    strlen(dataString)          +
    strlen(hashString)          + 2;

  char *newName = (char *)calloc(size, sizeof(char));

  if (!newName)
    return strdup("defaultName");

  strcat (newName, prefix);
  strcat (newName,  firstSeparator);
  strncat(newName, dataString, strlen(dataString) - 1);
  strcat (newName, "_");
  strcat (newName, hashString);
  strcat (newName, secondSeparator);
  strcat (newName, suffix);

  return newName;
}

static char *getDataString()
{
  time_t now = 0;
  time(&now);
  char *dataString = ctime(&now);

  for (int i = 0; dataString[i]; ++i)
    if (isspace(dataString[i]) || ispunct(dataString[i]))
      dataString[i] = '_';

  return dataString;
}

static char *getHashString()
{
  static char buffer[MAX_HASH_SIZE] = "";
  memset(buffer, 0,  MAX_HASH_SIZE);

  static int lastHash = 0;

  sprintf(buffer, "%010d", lastHash++);

  return buffer;
}
