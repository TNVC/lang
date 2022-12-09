#include "Logging.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>
#include "SystemLike.h"

#ifdef HTML_LOG

#define START_LOG

#define END_LOG

#define NEW_LOG_FILE

#else

#define SEPARATOR "============================================="

#define START_LOG                                               \
  do                                                            \
    {                                                           \
      fprintf(LOG_FILE, "<pre>\n");                             \
                                                                \
      fprintf(LOG_FILE, SEPARATOR SEPARATOR "\n");              \
                                                                \
      time_t LOG_TIME_TEMP = 0;                                 \
      time(&LOG_TIME_TEMP);                                     \
      fprintf(LOG_FILE, "%s", ctime(&LOG_TIME_TEMP));           \
                                                                \
      fprintf(LOG_FILE, SEPARATOR " START " SEPARATOR "\n\n");  \
                                                                \
      fprintf(LOG_FILE, SEPARATOR SEPARATOR "\n");              \
    } while (0)

#define END_LOG                                                 \
  do                                                            \
    {                                                           \
      fprintf(LOG_FILE, SEPARATOR SEPARATOR "\n\n");            \
                                                                \
      fprintf(LOG_FILE, SEPARATOR "  END  " SEPARATOR "\n\n");  \
                                                                \
      fprintf(LOG_FILE, SEPARATOR SEPARATOR "\n");              \
    } while (0)

#define NEW_LOG_FILE                                            \
  do                                                            \
    {                                                           \
      fprintf(LOG_FILE, SEPARATOR SEPARATOR "\n");              \
                                                                \
      time_t LOG_TIME_TEMP = 0;                                 \
      time(&LOG_TIME_TEMP);                                     \
      fprintf(LOG_FILE, "%s", ctime(&LOG_TIME_TEMP));           \
                                                                \
      fprintf(LOG_FILE, SEPARATOR "NEWFILE" SEPARATOR "\n\n");  \
                                                                \
      fprintf(LOG_FILE, SEPARATOR SEPARATOR "\n");              \
    } while(0)

#endif

/// Init logs
/// @note Autocallable
static unsigned initLog();

/// Destroy logs
/// @note Autocallable
static void destroyLog();

/// Try to open new file with name from getNewLogFileName()
/// @return Was file open
/// @note Don`t auto close files
static int openNewLogFile();

/// Return C-like string with data and time information
/// @return C-like string in static array
static const char *getDataString();

/// Generate new log file name using LOG_FILE_PREFIX and LOG_FILE_SUFFIX
/// @return C-like string in heap
static char *getNewLogFileName();

static FILE    *LOG_FILE          = nullptr;

static unsigned LOG_LEVEL         = initLog();

static char    *LOG_FILE_NAME     = nullptr;

static size_t   MAX_LOG_FILE_SIZE = 1024 * 1024 * 256;

static unsigned initLog()
{
  if (!isFileExists(LOG_DIRECTORY))
    mkdir(LOG_DIRECTORY, 0777);

  LOG_FILE_NAME = getNewLogFileName();

  LOG_FILE = fopen(LOG_FILE_NAME, "a");

  if (LOG_FILE == nullptr)
    return 0x00;

  setvbuf(LOG_FILE, nullptr, _IONBF, 0);

  atexit(destroyLog);

  START_LOG;

  unsigned level = 0;

#if   defined RELEASE_LOG_LEVEL_

  level |= FATAL;

#elif defined ERROR_LOG_LEVEL_

  level |= FATAL | ERROR;

#elif defined MESSAGE_LOG_LEVEL_

  level |= FATAL | ERROR | MESSAGE;

#elif defined VALUE_LOG_LEVEL_

  level |= FATAL | ERROR | MESSAGE | VALUE;

#endif

  return level;
}

static void destroyLog()
{
  if (isPointerCorrect(LOG_FILE))
    {
      END_LOG;

      fclose(LOG_FILE);
    }

  LOG_FILE = nullptr;

  if (isPointerCorrect(LOG_FILE_NAME))
    free(LOG_FILE_NAME);

  LOG_FILE_NAME = nullptr;

  LOG_LEVEL = 0;
}

FILE *getLogFile()
{
  int isNameCorrect = isPointerCorrect(LOG_FILE_NAME);

  int isFileFull    = getFileSize(LOG_FILE_NAME) >= MAX_LOG_FILE_SIZE;

  if (isNameCorrect && isFileFull)
    {
      if (isPointerCorrect(LOG_FILE))
        {
          NEW_LOG_FILE;

          fclose(LOG_FILE);
        }

      free(LOG_FILE_NAME);

      if (!openNewLogFile())
        return nullptr;

      NEW_LOG_FILE;
    }
  else if (!isNameCorrect && isFileFull)
    {
      if (!openNewLogFile())
        return nullptr;

      NEW_LOG_FILE;
    }

  return LOG_FILE;
}

char *getNewLogFileName()
{
  time_t now = 0;
  time(&now);
  char *dataString = ctime(&now);

  for (int i = 0; dataString[i]; ++i)
    if (isspace(dataString[i]) || ispunct(dataString[i]))
      dataString[i] = '_';

  size_t size =
    sizeof(LOG_DIRECTORY)   +
    sizeof(LOG_FILE_PREFIX) +
    sizeof(LOG_FILE_SUFFIX) +
    strlen(dataString)      + 3;

  char *newLogFileName = (char *) calloc(1, size);

  if (!isPointerCorrect(newLogFileName))
    return strdup(".log/defaultLogFile.txt");

  strcat (newLogFileName, LOG_DIRECTORY);
  strcat (newLogFileName, LOG_FILE_PREFIX);
  strcat (newLogFileName, "_");
  strncat(newLogFileName, dataString, strlen(dataString) - 1);
  strcat (newLogFileName, ".");
  strcat (newLogFileName, LOG_FILE_SUFFIX);

  return newLogFileName;
}

int loggingPrint(unsigned level, long long value, const char *name,
                 const char *fileName, const char *functionName, int line)
{
#ifdef RELEASE_BUILD_

  return 0;

#endif

  if (!isPointerCorrect(name))
    name         = "nullptr";
  if (!isPointerCorrect(fileName))
    fileName     = "nullptr";
  if (!isPointerCorrect(functionName))
    functionName = "nullptr";

  if (!(LOG_LEVEL & level))
    return 0;

  FILE *filePtr = getLogFile();

  if (!isPointerCorrect(filePtr))
    return 0;

  const char *dataString = getDataString();

  if (!isPointerCorrect(dataString))
    return 0;

  switch (level)
    {
    case VALUE:

      return fprintf(filePtr, "[%s] File: %30s, Function: %60s, Line: %5d. Decimal value of '%s': %lld.",
                     dataString, fileName, functionName, line, name, value);
    case MESSAGE:
    case WARNING:
    case ERROR:
    case FATAL:
    default:

      fprintf(filePtr, "Incorrect use of log functions!! File: %30s, Function: %60s, Line: %5d.",
              fileName, functionName, line);

      return 0;
    }
}

int loggingPrint(unsigned level, double value, const char *name,
                 const char *fileName, const char *functionName, int line)
{
#ifdef RELEASE_BUILD_

  return 0;

#endif

  if (!isPointerCorrect(name))
    name         = "nullptr";
  if (!isPointerCorrect(fileName))
    fileName     = "nullptr";
  if (!isPointerCorrect(functionName))
    functionName = "nullptr";

  if (!(LOG_LEVEL & level))
    return 0;

  FILE *filePtr = getLogFile();

  if (!isPointerCorrect(filePtr))
    return 0;

  const char *dataString = getDataString();

  if (!isPointerCorrect(dataString))
    return 0;

  switch (level)
    {
    case VALUE:
      return fprintf(filePtr, "[%s] File: %30s, Function: %60s, Line: %5d. Double value of '%s': %lf.",
                     dataString, fileName, functionName, line, name, value);

    case MESSAGE:
    case WARNING:
    case ERROR:
    case FATAL:
    default:
      fprintf(filePtr, "Incorrect use of log functions!! File: %30s, Function: %60s, Line: %5d.",
              fileName, functionName, line);

      return 0;
    }
}

int loggingPrint(unsigned level, char value, const char *name,
                 const char *fileName, const char *functionName, int line)
{
#ifdef RELEASE_BUILD_

  return 0;

#endif
  value = isgraph(value) ? value : isspace(value) ? ' ' : '#';

  if (!isPointerCorrect(name))
    name         = "nullptr";
  if (!isPointerCorrect(fileName))
    fileName     = "nullptr";
  if (!isPointerCorrect(functionName))
    functionName = "nullptr";

  if (!(LOG_LEVEL & level))
    return 0;

  FILE *filePtr = getLogFile();

  if (!isPointerCorrect(filePtr))
    return 0;

  const char *dataString = getDataString();

  if (!isPointerCorrect(dataString))
    return 0;

  switch (level)
    {
    case VALUE:
      return fprintf(filePtr, "[%s] File: %30s, Function: %60s, Line: %5d. Char value of '%s': '%c'.",
                     dataString, fileName, functionName, line, name, value);

    case MESSAGE:
    case WARNING:
    case ERROR:
    case FATAL:
    default:
      fprintf(filePtr, "Incorrect use of log functions!! File: %30s, Function: %60s, Line: %5d.",
              fileName, functionName, line);

      return 0;
    }
}

int loggingPrint(unsigned level, const void *value, const char *name,
                 const char *fileName, const char *functionName, int line)
{
#ifdef RELEASE_BUILD_

  return 0;

#endif

  if (!isPointerCorrect(name))
    name         = "nullptr";
  if (!isPointerCorrect(fileName))
    fileName     = "nullptr";
  if (!isPointerCorrect(functionName))
    functionName = "nullptr";

  if (!(LOG_LEVEL & level))
    return 0;

  FILE *filePtr = getLogFile();

  if (!isPointerCorrect(filePtr))
    return 0;

  const char *dataString = getDataString();

  if (!isPointerCorrect(dataString))
    return 0;

  switch (level)
    {
    case VALUE:
      return fprintf(filePtr, "[%s] File: %30s, Function: %60s, Line: %5d. Pointer value of '%s': %p.",
                     dataString, fileName, functionName, line, name, value);

    case MESSAGE:
    case WARNING:
    case ERROR:
    case FATAL:
    default:
      fprintf(filePtr, "Incorrect use of log functions!! File: %30s, Function: %60s, Line: %5d.",
              fileName, functionName, line);

      return 0;
    }
}

int loggingPrint(unsigned level, const char *value, const char *name,
                 const char *fileName, const char *functionName, int line)
{
  if (!isPointerCorrect(value))
    value        = "nullptr";
  if (!isPointerCorrect(name))
    name         = "nullptr";
  if (!isPointerCorrect(fileName))
    fileName     = "nullptr";
  if (!isPointerCorrect(functionName))
    functionName = "nullptr";

  if (!(LOG_LEVEL & level))
    return 0;

  FILE *filePtr = getLogFile();

  if (!isPointerCorrect(filePtr))
    return 0;

  const char *dataString = getDataString();

  if (!isPointerCorrect(dataString))
    return 0;

  switch (level)
    {
    case VALUE:
      return fprintf(filePtr, "[%s] File: %30s, Function: %60s, Line: %5d. C-like string value of '%s': \"%s\".",
                     dataString, fileName, functionName, line, name, value ? value : "nullptr");

    case MESSAGE:
      return fprintf(filePtr, "[%s] File: %30s, Function: %60s, Line: %5d. Message: \"%s\".",
                     dataString, fileName, functionName, line, value);

    case WARNING:
      return fprintf(filePtr, "[%s] File: %30s, Function: %60s, Line: %5d. WARNING!!: \"%s\".",
                     dataString, fileName, functionName, line, value);

    case ERROR:
      return fprintf(filePtr, "[%s] File: %30s, Function: %60s, Line: %5d. ERROR!!: \"%s\".",
                     dataString, fileName, functionName, line, value);

    case FATAL:
      return fprintf(filePtr, "[%s] File: %30s, Function: %60s, Line: %5d. !!FATAL ERROR!!: \"%s\".",
                     dataString, fileName, functionName, line, value);

    default:
      return fprintf(filePtr, "Incorrect use of log functions!! File: %30s, Function: %60s, Line %5d.",
                     fileName, functionName, line);

      return 0;
    }
}

static int openNewLogFile()
{
  LOG_FILE_NAME = getNewLogFileName();

  LOG_FILE = fopen(LOG_FILE_NAME, "a");

  if (!isPointerCorrect(LOG_FILE))
    {
      LOG_LEVEL = 0x00;

      return 0;
    }

  setvbuf(LOG_FILE, nullptr, _IONBF, 0);

  return 1;
}

static const char *getDataString()
{
  time_t now = 0;

  time(&now);

  char *dataString = ctime(&now);

  char *newLine = strchr(dataString, '\n');
  if (isPointerCorrect(newLine))
    *newLine = '\0';

  return dataString;
}

