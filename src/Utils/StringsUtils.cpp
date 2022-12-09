#include "StringsUtils.h"

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "Assert.h"

static size_t splitBuff(char *buffer, size_t size);

String *parseToLines(char *buffer, size_t bufferSize, size_t *lineCount)
{
  assert(buffer);
  assert(lineCount);

  *lineCount = splitBuff(buffer, bufferSize);

  String *strings = (String *)calloc(*lineCount, sizeof(String));

  if (!strings)
    return nullptr;

  size_t currentLine = 0, lineSize = 0;

  for (size_t i = 0; currentLine < *lineCount; ++i)
    {
      strings[currentLine].buff = buffer + i;

      for ( ; buffer[i]; ++i, ++lineSize)
        {
          if (buffer[i] == ';')
            {
              buffer[i] = '\0';

              while(buffer[++i])
                continue;

              --i;
              --lineSize;
            }
        }

      strings[currentLine++].size = lineSize;

      lineSize = 0;
    }

  return strings;
}

static size_t splitBuff(char *buffer, size_t size)
{
  assert(buffer);

    size_t lines = 0;

    char *lastFindChar = strchr(buffer, '\n');

    for ( ; lastFindChar; lastFindChar = strchr(lastFindChar, '\n'))
    {
        ++lines;

        *lastFindChar = '\0';

        ++lastFindChar;

        if (lastFindChar >= buffer + size)
            break;
    }

    return lines;
}

int stricmp(const char *first, const char *second)
{
  assert(first);
  assert(second);

  int i = 0;

  for ( ; first[i] && second[i]; ++i)
    if (tolower(first[i]) != tolower(second[i]))
      return tolower(first[i]) - tolower(second[i]);

  return tolower(first[i]) - tolower(second[i]);
}

int strincmp(const char *first, const char *second, int maxSize)
{
  assert(first);
  assert(second);

  int i = 0;

  for ( ; first[i] && second[i] && i < maxSize; ++i)
    if (tolower(first[i]) != tolower(second[i]))
      return tolower(first[i]) - tolower(second[i]);

  if (i < maxSize)
    return tolower(first[i]) - tolower(second[i]);
  return 0;
}

int isStringEmpty(const char *string)
{
  for ( ; *string; ++string)
    if (!isspace(*string))
      return 0;

  return 1;
}

int strcmpto(const char *first, const char *second, char determinant)
{
  assert(first);
  assert(second);

  int i = 0;

  for ( ; first[i]                && second[i] &&
          first[i] != determinant && second[i] != determinant; ++i)
    if (first[i] != second[i])
      return first[i] - second[i];

  if ((first[i]  == '\0' || first[i]  == determinant) &&
      (second[i] == '\0' || second[i] == determinant))
    return 0;

  return first[i] - second[i];
}


int hasSpace(const char *start, const char *end)
{
  assert(start);
  assert(end);

  while (start != end)
    if (isspace(*start++))
      return 1;

  return 0;
}

int isCorrectName(const char *start, const char *end)
{
  assert(start);
  assert(end);

  for ( ; start != end; ++start)
    {
      if (!(isalnum(*start) || *start == '_' || *start == '$'))
        return 0;
    }

  return 1;
}

int isCorrectName(const char *string)
{
  assert(string);

  return isCorrectName(string, string + strlen(string));
}

int trimString(char *string)
{
  assert(string);

  size_t i = strlen(string) - 1;

  for ( ; i > 0; --i)
    if (!isspace(string[i]))
      break;
    else
      string[i] = '\0';

  if (isspace(string[i]))
    string[i] = '\0';

  return 0;
}

int isDigitString(const char *string)
{
  assert(string);

  bool hasDot = false;

  while (*string)
    if (!isdigit(*string++))
      {
        if (*(string - 1) != '.' || hasDot)
          return false;

        hasDot = true;
      }

  return true;
}

char *strndup(const char *string, int size)
{
  assert(string);

  size_t realSize = strlen(string);

  realSize = (realSize < (size_t)size ? realSize : (size_t)size) + 1;

  char *buffer = (char *)calloc(realSize, sizeof(char));

  if (!buffer) return nullptr;

  strncpy(buffer, string, (size_t)size);

  return buffer;
}

char *strnigDuplicate(const char *first, const char *second)
{
  assert(first);
  assert(second);

  size_t size = strlen(first) + strlen(second) + 1;

  char *buffer = (char *)calloc(size, sizeof(char));

  if (!buffer) return nullptr;

  strcat(buffer, first );
  strcat(buffer, second);

  return buffer;
}
