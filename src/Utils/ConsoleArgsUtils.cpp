#include "ConsoleArgsUtils.h"

#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include "ResourceBundle.h"
#include "StringsUtils.h"
#include "SystemLike.h"
#include "ErrorHandler.h"
#include "GarbageCollector.h"
#include "StringsUtils.h"
#include "Assert.h"

#define HANDLE_IF(name, handler)                    \
  if (!strcmp(argv[i], FLAGS[name]))                \
    do                                              \
      {                                             \
        if (++i < argc && argv[i][0] != '-')        \
          {                                         \
            int error = handler(argv[i], settings); \
                                                    \
            if (error)                              \
              return error;                         \
          }                                         \
        else                                        \
          INCORRECT_ARGUMENT;                       \
      } while (0)

#define ELSE_HANDLE_IF(name, handler) else HANDLE_IF(name, handler)

#define INCORRECT_ARGUMENT                                              \
  do                                                                    \
    {                                                                   \
      handleIncorrectArgument(                                          \
                              argv[i - 1],                              \
                              i < argc ? argv[i] : "nothing"            \
                             );                                         \
                                                                        \
      return CONSOLE_INCORRECT_ARGUMENTS;                               \
    } while (0)

#define NO_INPUT_FILES                          \
  do                                            \
    {                                           \
      handleError("No input files");            \
                                                \
      return CONSOLE_NO_INPUT_FILES;            \
    } while (0)

#define HANDLE_FILE_NAME(name, type)                               \
  do                                                               \
    {                                                              \
      if (!settings->type)                                         \
        {                                                          \
          settings->type = addDirectory(name);                     \
                                                                   \
          /*addElementForFree(settings->type);*/                   \
        }                                                          \
      else                                                         \
        {                                                          \
          handleWarning("Too many "#type" files [%s]", name);      \
        }                                                          \
    } while (0)

/// Consts-indexes in FLAGS array
enum Flags {
  LOAD,
  SAVE,
  HELP,
};

/// Type of indefity console flags
const char *FLAGS[] = {
  "-load",
  "-save",
  "-help",
};

const int DEFAULT_GROWTH_FACTOR = 2;

static bool setDefaultSettings(Settings *settings);

/// Handle flag -in
/// @param [in] argument Argument for -in
/// @return Zero is all was Ok or NO_SUCH_FILE_FOUND if argument isn`t a file name
static int handleLoad(const char *argument, Settings *settings);

/// Handle flag -out
/// @param [in] argument Argument for -out
/// @return Error`s code
static int handleSave(const char *argument, Settings *settings);

static int handleHelp(Settings *settings);

/// Handle incorrect arguments for flags
/// @param [in] flag Name of flag wicth geted incorrect argument
/// @param [in] argument Geted argument
static void handleIncorrectArgument(const char *flag, const char *argument);

/// Handle unknown flags
/// @param [in] flag Name of unknown flag
static void handleUnknownFlag      (const char *flag);

#include <stdio.h>

int parseConsoleArgs(const int argc, const char * const argv[], Settings *settings)
{
  assert(argv);
  assert(argc > 0);
  assert(settings);

  if (setDefaultSettings(settings)) return 1;
  settings->programName = argv[0];

  setSettings(settings);

  for (int i = 1; i < argc; ++i)
    {
      if (!strcmp(argv[i], FLAGS[HELP]))
        {
          handleHelp(settings);

          return CONSOLE_HELP;
        }
      ELSE_HANDLE_IF(LOAD, handleLoad);
      ELSE_HANDLE_IF(SAVE, handleSave);
      else if (argv[i][0] == '-')
          handleUnknownFlag(argv[i]);
      else
        {
          if (handleLoad(argv[i], settings))
            return CONSOLE_NO_SUCH_FILE_FOUND;
        }
    }

  if (!settings->source)
    handleLoad(DEFAULT_SOURCE_FILE_NAME, settings);

  if (!settings->target)
    handleSave(DEFAULT_TARGET_FILE_NAME, settings);

  return 0;
}

static bool setDefaultSettings(Settings *settings)
{
  assert(settings);

  settings->programName  = nullptr;
  settings->source       = nullptr;
  settings->target       = nullptr;

  return 0;
}

static int handleLoad(const char *argument, Settings *settings)
{
  char *fileName = addDirectory(argument);

  if (!isFileExists(fileName))
    {
      handleError("No such file [%s]", fileName);

      free(fileName);

      return CONSOLE_NO_SUCH_FILE_FOUND;
    }

  free(fileName);

  HANDLE_FILE_NAME(argument, source);

  return 0;
}

static int handleSave(const char *argument, Settings *settings)
{
  HANDLE_FILE_NAME(argument, target);

  return 0;
}

static int handleHelp(Settings *settings)
{
  db::ResourceBundle bundle{};

  db::getBundle(&bundle, "help");

  const char *separator = db::getString(&bundle, "help.separator");
  
  const char *countString = db::getString(&bundle, "help.count");
  
  int count = 0;
  sscanf(countString, "%d", &count);

  printf("%s", settings->programName);

  char buffer[16] = "";
  for (int i = 0; i < count; ++i)
  {
    sprintf(buffer, "help.num%d", i+1);
    printf("%s%s", 
      db::getString(&bundle, buffer),
      separator);
  }

  db::destroyBundle(&bundle);

  return 0;
}

static void handleIncorrectArgument(const char *flag, const char *argument)
{
  handleError("%s expeced argument, but geted %s", flag, argument);
}

static void handleUnknownFlag(const char *flag)
{
  handleWarning("Unknow flag [%s]", flag);
}
